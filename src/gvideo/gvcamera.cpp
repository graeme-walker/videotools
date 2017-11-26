//
// Copyright (C) 2017 Graeme Walker
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
//
// gvcamera.cpp
//

#include "gdef.h"
#include "gvcamera.h"
#include "gvcapturefactory.h"
#include "gstr.h"
#include "grimagetype.h"
#include "grcolourspace.h"

Gv::Camera::Camera( Gr::ImageConverter & converter ,
	const std::string & dev_name , const std::string & dev_config , 
	bool one_shot , bool lazy_open , unsigned int open_timeout , 
	const std::string & caption , const Gv::Timezone & tz , 
	const std::string & image_input_source_name ) :
		ImageInputSource(converter,image_input_source_name.empty()?dev_name:image_input_source_name) ,
		m_dev_name(dev_name) ,
		m_dev_config(dev_config) ,
		m_one_shot(one_shot) ,
		m_lazy_open(lazy_open) ,
		m_open_timeout(std::max(1U,open_timeout)) ,
		m_caption(caption) ,
		m_tz(tz) ,
		m_dx(0) ,
		m_dy(0) ,
		m_open_timer(*this,&Gv::Camera::onOpenTimeout,*this) ,
		m_read_timer(*this,&Gv::Camera::onReadTimeout,*this) ,
		m_send_timer(*this,&Gv::Camera::onSendTimeout,*this)
{
	m_sleep_ms = G::Str::toUInt( G::Str::splitMatchTail(dev_config,"sleep=",";") , "0" ) ;
	G_DEBUG( "Gv::Camera::Camera: device [" << dev_name << "] sleep=" << m_sleep_ms << "ms" ) ;
	m_capture.reset( create(dev_name,dev_config,m_lazy_open) ) ;
	if( m_capture.get() == nullptr )
		m_open_timer.startTimer( m_open_timeout ) ;
	else
		onOpen() ;
}

Gv::Camera::~Camera()
{
	if( m_capture.get() != nullptr )
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(fd()) ) ;
}

bool Gv::Camera::isOpen() const
{
	return fd() != -1 ;
}

Gv::Capture * Gv::Camera::create( const std::string & dev_name , const std::string & dev_config , bool lazy_open )
{
	try
	{
		return Gv::CaptureFactory::create( dev_name , dev_config , m_tz ) ;
	}
	catch( Gv::Capture::NoDevice & )
	{
		if( lazy_open ) return nullptr ;
		throw ;
	}
}

void Gv::Camera::onOpenTimeout()
{
	m_capture.reset( create(m_dev_name,m_dev_config,true) ) ;
	if( m_capture.get() != nullptr )
		onOpen() ;
	else
		m_open_timer.startTimer( m_open_timeout ) ;
}

void Gv::Camera::onOpen()
{
	m_dx = static_cast<int>(m_capture->dx()) ;
	m_dy = static_cast<int>(m_capture->dy()) ;
	m_image_type = Gr::ImageType::raw( m_dx , m_dy , 3 ) ;

	G_LOG( "Gv::Camera::onOpen: camera device=[" << m_dev_name << "] " << m_capture.get()->info() ) ;
	GNet::EventLoop::instance().addRead( GNet::Descriptor(fd()) , *this ) ;
}

int Gv::Camera::fd() const
{
	return m_capture.get() ? m_capture->fd() : -1 ;
}

void Gv::Camera::onReadTimeout()
{
	G_DEBUG( "Gv::Camera::readEvent: delayed read" ) ;
	GNet::EventLoop::instance().addRead( GNet::Descriptor(fd()) , *this ) ;
	doRead() ;
}

void Gv::Camera::onException( std::exception & e )
{
	// exception thrown from readEvent() (ie. Gv::Capture or operator()())
	// out to the event loop and delivered back
	G_ERROR( "Gv::Camera::onException: exception: " << e.what() ) ;
	throw ; // rethrow to the event loop to terminate
}

void Gv::Camera::readEvent()
{
	if( m_sleep_ms )
	{
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(fd()) ) ;
		m_read_timer.startTimer( 0 , m_sleep_ms * 1000UL ) ;
	}
	else
	{
		doRead() ;
	}
}

void Gv::Camera::doRead()
{
	if( !doReadImp() ) // if no device
	{
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(fd()) ) ;
		blank() ;
		m_capture.reset() ;
		m_open_timer.startTimer( m_open_timeout ) ;
		m_send_timer.startTimer( 0 ) ;
	}
}

void Gv::Camera::blank()
{
	G_ASSERT( m_capture.get() != nullptr ) ;
	Gr::ImageBuffer * image_buffer_p = Gr::Image::blank( m_image , m_image_type , m_capture->simple() ) ;
	Gr::ImageData image_data( *image_buffer_p , m_image_type.dx() , m_image_type.dy() , m_image_type.channels() ) ;
	int dy = image_data.dy() ;
	int dx = image_data.dx() ;
	for( int y = 0 ; y < dy ; y++ )
	{
		for( int x = 0 ; x < dx ; x++ )
		{
			image_data.rgb( x , y , image_data.r(x,y)/8U , image_data.g(x,y)/8U , image_data.b(x,y)/8U ) ;
		}
	}
	Gv::Overlay overlay( image_data ) ;
	overlay.write( (dx-8)/2 , (dy-8)/2 , "X" , m_tz ) ; // could do better
}

namespace
{
	struct Fill : public Gv::CaptureCallback
	{
		Fill( Gr::ImageData & image_data ) :
			m_image_data(image_data)
		{
		}
		virtual void operator()( const Gv::CaptureBuffer & cb ) override
		{
			cb.copyTo( m_image_data ) ;
		}
		Gr::ImageData & m_image_data ;
	} ;
}

bool Gv::Camera::doReadImp()
{
	try
	{
		bool read_ok = false ;
		Gr::ImageBuffer * image_buffer_p = Gr::Image::blank( m_image , m_image_type , m_capture->simple() ) ;
		Gr::ImageData image_data( *image_buffer_p , m_image_type.dx() , m_image_type.dy() , m_image_type.channels() ) ;
		if( m_capture->simple() ) // RGB24 etc, typically libv4l
		{
			// read() straight into the contiguous image buffer
			char * p = Gr::imagebuffer::row_ptr( Gr::imagebuffer::row_begin(*image_buffer_p) ) ;
			size_t n = Gr::imagebuffer::size_of( *image_buffer_p ) ;
			read_ok = m_capture->read( reinterpret_cast<unsigned char*>(p) , n ) ;
		}
		else
		{
			// apply the capture buffer to the Fill object via operator()()
			Fill copy_to_image( image_data ) ;
			read_ok = m_capture->read( copy_to_image ) ;
		}
		if( read_ok ) 
		{
			if( !m_caption.empty() )
			{
				// add a timestamp overlay
				Gv::Overlay overlay( image_data ) ;
				overlay.write( 10 , 10 , m_caption , m_tz ) ;
			}
			m_send_timer.startTimer( 0 ) ; // send asynchronously (for historical reasons)
		}
		else
		{
			G_DEBUG( "Gv::Camera::doRead: failed to acquire an image from the capture device" ) ; // eg. EAGAIN
		}
		return true ;
	}
	catch( Gv::Capture::NoDevice & )
	{
		G_LOG( "Gv::Camera::doRead: capture device has disappeared" ) ;
		if( m_one_shot ) 
			throw ;
		return false ;
	}
}

void Gv::Camera::onSendTimeout()
{
	if( !m_image.empty() )
		sendImageInput( m_image ) ; // Gv::ImageInputSource
}

void Gv::Camera::resend( ImageInputHandler & )
{
	m_send_timer.startTimer( 0 ) ;
}

/// \file gvcamera.cpp

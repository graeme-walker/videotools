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
// grimage.cpp
//

#include "gdef.h"
#include "grdef.h"
#include "grimage.h"
#include "grpnm.h"
#include "glimits.h"
#include "ghexdump.h"
#include "gdebug.h"

namespace
{
	size_t size_of( shared_ptr<const Gr::ImageBuffer> ptr )
	{
		return ptr.get() == nullptr ? 0U : Gr::imagebuffer::size_of(*ptr) ;
	}
}

Gr::Image::Image()
{
}

Gr::Image::Image( shared_ptr<const ImageBuffer> ptr , ImageType type ) :
	m_ptr(ptr) ,
	m_type(type)
{
	if( type.isRaw() && size_of(ptr) != type.size() )
	{
		G_ASSERT( !"invalid raw image buffer size" ) ;
		type = ImageType() ;
	}
	if( ptr.get() == nullptr )
		type = ImageType() ;
}

Gr::ImageBuffer * Gr::Image::blank( Image & image , ImageType type , bool contiguous )
{
	G_ASSERT( type.isRaw() ) ;
	if( !type.isRaw() )
		throw std::runtime_error( "invalid type for blank image" ) ;

	shared_ptr<ImageBuffer> ptr = image.recycle() ;

	if( contiguous )
	{
		ptr->resize( 1U ) ;
		ptr->at(0).resize( type.size() ) ;
	}
	else
	{
		size_t rowsize = type.rowsize() ;
		const int dy = type.dy() ;
		ptr->resize( dy ) ;
		for( int y = 0 ; y < dy ; y++ )
			ptr->at(y).resize( rowsize ) ;
	}

	image = Image( ptr , type ) ;
	return ptr.get() ;
}

bool Gr::Image::valid() const
{
	return !empty() && m_type.valid() ;
}

Gr::ImageType Gr::Image::type() const
{
	return m_type ;
}

bool Gr::Image::empty() const
{
	return m_ptr.get() == nullptr ;
}

void Gr::Image::clear()
{
	m_ptr.reset() ;
	m_type = ImageType() ;
}

size_t Gr::Image::size() const
{
	if( empty() ) return 0U ;
	return size_of(m_ptr) ;
}

shared_ptr<const Gr::ImageBuffer> Gr::Image::ptr() const
{
	return m_ptr ;
}

const Gr::ImageBuffer & Gr::Image::data() const
{
	return *m_ptr ;
}

shared_ptr<Gr::ImageBuffer> Gr::Image::recycle()
{
	if( m_ptr.get() != nullptr && m_ptr.unique() )
		return const_pointer_cast<ImageBuffer>(m_ptr) ;
	else
		return shared_ptr<ImageBuffer>( new ImageBuffer ) ;
}

void Gr::Image::read( std::istream & in , Image & image , const std::string & help_text )
{
	if( !in.good() )
		throw ReadError( help_text ) ;

	ImageType image_type_in( in ) ;
	G_DEBUG( "Gr::Image::read: type=[" << image_type_in << "]" ) ;

	shared_ptr<ImageBuffer> ptr = image.recycle() ;

	if( image_type_in.isPnm() ) // read pnm files as raw 
	{
		ImageBuffer & image_buffer = *ptr ;
		imagebuffer::resize( image_buffer , image_type_in.dx() , image_type_in.dy() , image_type_in.channels() ) ;
		ImageData image_data( image_buffer , image_type_in.dx() , image_type_in.dy() , image_type_in.channels() ) ;
		PnmReader reader ;
		reader.decode( image_data , in ) ;
		image_type_in = ImageType::raw( image_type_in.dx() , image_type_in.dy() , image_type_in.channels() ) ;
	}
	else
	{
		// raw files cannot be stored on disk because the the image size is 
		// not intrinsic to the format, so treat the file as jpeg or png
		in >> *ptr ;
		if( in.fail() )
			throw ReadError( help_text ) ;
	}
	image = Image( ptr , image_type_in ) ;
}

bool Gr::Image::receive( G::PublisherSubscription & channel , Image & image , std::string & type_str )
{
	return receiveImp( false , channel , image , type_str ) ;
}

bool Gr::Image::peek( G::PublisherSubscription & channel , Image & image , std::string & type_str )
{
	return receiveImp( true , channel , image , type_str ) ;
}

bool Gr::Image::receiveImp( bool peek , G::PublisherSubscription & channel , Image & image , std::string & type_str )
{
	shared_ptr<ImageBuffer> ptr = image.recycle() ;
	ptr->resize( 1U ) ;
	std::vector<char> & buffer = ptr->at(0) ;

	bool ok = peek ? channel.peek(buffer,&type_str) : channel.receive(buffer,&type_str) ;
	if( !ok )
		return false ;

	ImageType image_type( type_str ) ;
	image = Image( ptr , image_type ) ; // even if !image_type.valid()

	return true ;
}

/// \file grimage.cpp

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
// gvcapture_test.cpp
//

#include "gdef.h"
#include "gvcapture.h"
#include "gvcapture_test.h"
#include "gmsg.h"
#include "gassert.h"
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

namespace
{
	int dx_() { return 640 ; }
	int dy_() { return 480 ; }
	size_t size( size_t channels ) { return channels * 640U * 480U ; }
	size_t rowsize( size_t channels ) { return channels * 640U ; }
}

Gv::CaptureTest::CaptureTest( const std::string & config , ImageGenerator * generator ) :
	m_generator(generator) ,
	m_capturing(false) ,
	m_fd_read(-1) ,
	m_fd_write(-1) ,
	m_format(99U) ,
	m_config(config) ,
	m_timer(*this,&Gv::CaptureTest::onTimeout,*this)
{
	bool rgb = generator->init() ;
	if( rgb )
	{
		// RGB24
		m_buffer.reset( new CaptureBuffer(size(3U)) ) ;
		m_scale = CaptureBufferScale( size(3U) , rowsize(3U) , dx_() , dy_() ) ;
		m_format = CaptureBufferFormat( 0 , "" , CaptureBufferFormat::rgb ,
			// offset,x-shift,y-shift,step
			CaptureBufferComponent( 0 , 0 , 0 , 3 ) ,
			CaptureBufferComponent( 1 , 0 , 0 , 3 ) ,
			CaptureBufferComponent( 2 , 0 , 0 , 3 ) ) ;
	}
	else
	{
		// YUYV (YUV422)
		m_buffer.reset( new CaptureBuffer(size(2U)) ) ;
		m_scale = CaptureBufferScale( size(2U) , rowsize(2U) , dx_() , dy_() ) ;
		m_format = CaptureBufferFormat( 0 , "" , CaptureBufferFormat::yuv ,
			// offset,x-shift,y-shift,step
			CaptureBufferComponent( 0 , 0 , 0 , 2 ) ,
			CaptureBufferComponent( 1 , 1 , 0 , 4 ) ,
			CaptureBufferComponent( 3 , 1 , 0 , 4 ) ) ;
	}

	createSockets() ;
	start() ;
}

Gv::CaptureTest::~CaptureTest()
{
	delete m_generator ;
	::close( m_fd_read ) ;
	::close( m_fd_write ) ;
}

int Gv::CaptureTest::dx( int )
{
	return dx_() ;
}

int Gv::CaptureTest::dy( int )
{
	return dy_() ;
}

unsigned int Gv::CaptureTest::dx() const
{
	return static_cast<unsigned int>(dx_()) ;
}

unsigned int Gv::CaptureTest::dy() const
{
	return static_cast<unsigned int>(dy_()) ;
}

void Gv::CaptureTest::onTimeout()
{
	if( m_capturing )
		raiseEvent() ;
}

void Gv::CaptureTest::onException( std::exception & )
{
	throw ;
}

bool Gv::CaptureTest::createSockets()
{
	int fds[2] ;
	int rc = ::socketpair( AF_LOCAL , SOCK_DGRAM , 0 , fds ) ;
	if( rc != 0 ) throw std::runtime_error( "socketpair error" ) ;
	m_fd_read = fds[0] ;
	m_fd_write = fds[1] ;
	::fcntl( m_fd_read , F_SETFL , ::fcntl(m_fd_read,F_GETFL) | O_NONBLOCK ) ;
	::fcntl( m_fd_write , F_SETFL , ::fcntl(m_fd_write,F_GETFL) | O_NONBLOCK ) ;
	return true ;
}

void Gv::CaptureTest::raiseEvent()
{
	if( m_fd_write != -1 )
		G::Msg::send( m_fd_write , "" , 1U , 0 ) ;
}

void Gv::CaptureTest::start()
{
	if( !m_capturing )
	{
		m_capturing = true ;
		raiseEvent() ;
	}
}

void Gv::CaptureTest::flushEvents()
{
	char c ; 
	ssize_t nread ;
	while( ( nread = G::Msg::recv( m_fd_read , &c , 1U , 0 ) ) == 1 ) ;
}

void Gv::CaptureTest::stop()
{
	if( m_capturing )
	{
		flushEvents() ;
		m_capturing = false ;
	}
}

bool Gv::CaptureTest::active() const
{
	return m_capturing ;
}

std::string Gv::CaptureTest::info() const
{
	return std::string() ;
}

void Gv::CaptureTest::readSocket()
{
	char c ; 
	ssize_t nread = G::Msg::recv( m_fd_read , &c , 1U , 0 ) ;
	if( nread != 1 ) 
		throw std::runtime_error( "recv error" ) ;
	if( m_capturing )
		m_timer.startTimer( 0 , 30000U ) ; // keep going -- set a maximum frame rate
}

bool Gv::CaptureTest::simple() const
{
	return false ;
}

bool Gv::CaptureTest::read( unsigned char * , size_t )
{
	return false ;
}

bool Gv::CaptureTest::read( CaptureCallback & callback )
{
	// do the event handling
	readSocket() ;

	// create the image
	m_buffer->setFormat( m_format , m_scale ) ;
	m_generator->fillBuffer( *m_buffer.get() , m_scale ) ;

	// callback op()()
	callback( *m_buffer.get() ) ;

	return true ;
}

int Gv::CaptureTest::fd() const
{
	return m_fd_read ;
}

/// \file gvcapture_test.cpp

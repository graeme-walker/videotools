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
// gvviewerinput.cpp
// 

#include "gdef.h"
#include "gnet.h"
#include "gvviewerinput.h"
#include "geventloop.h"
#include "gassert.h"
#include "glog.h"
#include <string>
#include <stdexcept>

Gv::ViewerInput::ViewerInput( ViewerInputHandler & input_handler , const ViewerInputConfig & input_config ,
	const std::string & channel , int shmem_fd , int pipe_fd ) :
		m_input_handler(input_handler) ,
		m_scale(input_config.m_scale) ,
		m_static(input_config.m_static) ,
		m_first(true),
		m_rate_limit(input_config.m_rate_limit) ,
		m_timer(*this,&ViewerInput::onTimeout,*this) ,
		m_decoder_tmp(Gr::ImageData::Contiguous) ,
		m_data_out(nullptr)
{
	if( channel.empty() )
	{
		m_pipe.reset( new G::FatPipeReceiver(shmem_fd,pipe_fd) ) ;
		m_fd = pipe_fd ;
	}
	else
	{
		m_channel.reset( new G::PublisherSubscription(channel) ) ;
		m_fd = m_channel->fd() ;
	}
	GNet::EventLoop::instance().addRead( GNet::Descriptor(m_fd) , *this ) ;
}

Gv::ViewerInput::~ViewerInput()
{
	GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_fd) ) ;
}

void Gv::ViewerInput::onException( std::exception & )
{
	throw ;
}

void Gv::ViewerInput::readEvent()
{
	if( m_static && !m_first ) 
	{
		read( m_tmp ) ; // client only wants the first image -- clear the event and discard the data
	}
	else if( read(m_buffer) && decode() )
	{
		m_first = false ;
		m_input_handler.onInput( m_type_out.dx() , m_type_out.dy() , m_type_out.channels() , m_data_out , m_type_out.size() ) ;
	}
}

bool Gv::ViewerInput::read( std::vector<char> & buffer )
{
	G::EpochTime time( 0 ) ;
	if( m_pipe.get() )
	{
		if( !m_pipe->receive( buffer , &m_type_in_str , &time ) )
			throw Closed( "parent has gone away" ) ;
	}
	else
	{
		if( !m_channel->receive( buffer , &m_type_in_str , &time ) )
			throw std::runtime_error( "publisher has gone away" ) ;
	}

	m_type_in = Gr::ImageType( m_type_in_str ) ;

	if( buffer.empty() ) 
		G_DEBUG( "Gv::ViewerInput::read: empty read" ) ; // eg. FatPipe::ping(), or select() race

	if( !m_type_in.valid() ) 
		G_DEBUG( "Gv::ViewerInput::read: invalid type: [" << m_type_in_str << "]" ) ;

	bool ok = !buffer.empty() && m_type_in.valid() ;
	if( ok ) 
		G_DEBUG( "Gv::ViewerInput::read: got image: size=" << buffer.size() << " type=[" << m_type_in << "]" ) ;

	if( ok && m_rate_limit != G::EpochTime(0) )
	{
		G_DEBUG( "Gv::ViewerInput::read: rate limiting: dropping fd " << m_fd ) ;
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_fd) ) ;
		m_timer.startTimer( m_rate_limit.s , m_rate_limit.us ) ;
	}
	return ok ;
}

void Gv::ViewerInput::onTimeout()
{
	G_DEBUG( "Gv::ViewerInput::onTimeout: rate limiting: resuming fd " << m_fd ) ;
	GNet::EventLoop::instance().addRead( GNet::Descriptor(m_fd) , *this ) ;
}

bool Gv::ViewerInput::decode()
{
	try
	{
		m_decoder.setup( m_scale , false ) ;
		m_data_out = &m_buffer[0] ;
		m_type_out = m_decoder.decodeInPlace( m_type_in , m_data_out/*in-out*/ , m_buffer.size() , m_decoder_tmp ) ;
		G_DEBUG( "Gv::ViewerInput::decode: type=[" << m_type_out << "]" ) ;
		return true ;
	}
	catch( std::exception & e )
	{
		G_WARNING_ONCE( "Gv::ViewerInput::decode: viewer failed to decode image: "
			"type=[" << m_type_in << "] size=" << m_buffer.size() << ": " << e.what() ) ;
		G_LOG( "Gv::ViewerInput::decode: failed image decode: "
			"type=[" << m_type_in << "] size=" << m_buffer.size() << ": " << e.what() ) ;
		return false ;
	}
}

const char * Gv::ViewerInput::data() const
{
	return &m_buffer[0] ;
}

size_t Gv::ViewerInput::size() const
{
	return m_buffer.size() ;
}

int Gv::ViewerInput::dx() const
{
	return m_type_out.dx() ;
}

int Gv::ViewerInput::dy() const
{
	return m_type_out.dy() ;
}

int Gv::ViewerInput::channels() const
{
	return m_type_out.channels() ;
}

Gr::ImageType Gv::ViewerInput::type() const
{
	return m_type_out ;
}

// ==

Gv::ViewerInputHandler::~ViewerInputHandler()
{
}

/// \file gvviewerinput.cpp

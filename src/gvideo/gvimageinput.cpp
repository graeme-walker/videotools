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
// gvimageinput.cpp
//

#include "gdef.h"
#include "gvimageinput.h"
#include "grjpeg.h"
#include "gassert.h"
#include <algorithm>

namespace Gv
{
	bool operator==( ImageInputConversion a , ImageInputConversion b )
	{
		return a.type == b.type && a.scale == b.scale && a.monochrome == b.monochrome ;
	}
	bool operator==( const ImageInputTask & a , const ImageInputTask & b )
	{
		return a.m_conversion == b.m_conversion ;
	}
}

// ==

Gv::ImageInputSource::ImageInputSource( Gr::ImageConverter & converter , const std::string & name ) :
	m_name(name) ,
	m_converter(converter)
{
}

Gv::ImageInputSource::~ImageInputSource()
{
	G_ASSERT( handlers() == 0U ) ;
}

std::string Gv::ImageInputSource::name() const
{
	return m_name ;
}

size_t Gv::ImageInputSource::handlers() const
{
	size_t n = 0U ;
	for( TaskList::const_iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( (*task_p).m_handler != nullptr )
			n++ ;
	}
	return n ;
}

void Gv::ImageInputSource::addImageInputHandler( ImageInputHandler & handler )
{
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( (*task_p).m_handler == &handler )
			return ;
	}

	m_tasks.push_back( ImageInputTask() ) ;
	m_tasks.back().m_handler = &handler ;
}

void Gv::ImageInputSource::removeImageInputHandler( ImageInputHandler & handler )
{
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( (*task_p).m_handler == &handler )
			(*task_p).m_handler = nullptr ;
	}
}

void Gv::ImageInputSource::sendNonImageInput( Gr::Image non_image_in , const std::string & type_str , 
	ImageInputHandler * one_handler_p )
{
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( (*task_p).m_handler != nullptr && (one_handler_p==nullptr || (*task_p).m_handler==one_handler_p) )
			(*task_p).m_handler->onNonImageInput( *this , non_image_in , type_str ) ;
	}
	collectGarbage() ;
}

bool Gv::ImageInputSource::sendImageInput( Gr::Image image_in , ImageInputHandler * one_handler_p )
{
	G_DEBUG( "Gv::ImageInputSource::sendImageInput: type=[" << image_in.type() << "](" << image_in.type() << ")" ) ;

	// update the conversion types
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( (*task_p).m_handler != nullptr && (one_handler_p==nullptr || (*task_p).m_handler==one_handler_p) )
		{
			(*task_p).m_conversion = (*task_p).m_handler->imageInputConversion(*this) ;
		}
	}

	// do the conversions
	bool all_ok = true ;
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( one_handler_p != nullptr && (*task_p).m_handler == one_handler_p )
		{
			if( ! (*task_p).run(m_converter,image_in) )
				all_ok = false ;
		}
		else if( (*task_p).m_handler != nullptr )
		{
			TaskList::iterator previous = std::find( m_tasks.begin() , task_p , *task_p ) ;
			if( previous != task_p )
				(*task_p).m_image = (*previous).m_image ; // we've already done it, so just take a copy
			else if( ! (*task_p).run(m_converter,image_in) )
				all_ok = false ;
		}
	}

	// call the handlers
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; ++task_p )
	{
		if( (*task_p).m_handler != nullptr && (one_handler_p==nullptr || (*task_p).m_handler==one_handler_p) )
		{
			if( (*task_p).m_image.empty() ) // ie. no conversion -- use the input image
				(*task_p).m_handler->onImageInput( *this , image_in ) ;
			else if( (*task_p).m_image.valid() )
				(*task_p).m_handler->onImageInput( *this , (*task_p).m_image ) ;
		}
	}

	collectGarbage() ;
	return all_ok ;
}

void Gv::ImageInputSource::collectGarbage()
{
	for( TaskList::iterator task_p = m_tasks.begin() ; task_p != m_tasks.end() ; )
	{
		if( (*task_p).m_handler == nullptr )
			task_p = m_tasks.erase( task_p ) ;
		else
			++task_p ;
	}
}

// ==

Gv::ImageInput::ImageInput( Gr::ImageConverter & converter , const std::string & channel_name , bool lazy_open ) :
	Gv::ImageInputSource(converter,channel_name) , // source name
	m_channel(channel_name,lazy_open) , // channel subscription
	m_resend_timer(*this,&ImageInput::onResendTimeout,*this) ,
	m_resend_to(nullptr)
{
}

Gv::ImageInput::~ImageInput()
{
}

bool Gv::ImageInput::open()
{
	G_ASSERT( m_channel.fd() == -1 ) ;
	G_ASSERT( m_image.empty() ) ;
	return m_channel.open() ;
}

std::string Gv::ImageInput::info() const
{
	return m_channel.info() ;
}

int Gv::ImageInput::fd() const
{
	return m_channel.fd() ;
}

bool Gv::ImageInput::handleReadEvent()
{
	return receive( false ) ;
}

bool Gv::ImageInput::receive( bool peek )
{
	if( peek )
	{
		if( ! Gr::Image::peek(m_channel,m_image,m_type_str) )
			return false ; // publisher has gone away
	}
	else
	{
		if( ! Gr::Image::receive(m_channel,m_image,m_type_str) )
			return false ; // publisher has gone away
	}

	if( m_image.valid() )
		sendImageInput( m_image ) ;
	else if( !m_image.empty() )
		sendNonImageInput( m_image , m_type_str ) ;

	return true ;
}

void Gv::ImageInput::close()
{
	if( m_channel.fd() != -1 )
	{
		m_channel.close() ;
		m_image.clear() ;
	}
	G_ASSERT( m_channel.fd() == -1 ) ;
}

void Gv::ImageInput::resend( ImageInputHandler & handler )
{
	m_resend_timer.startTimer( 0U ) ;
	m_resend_to = &handler ;
}

void Gv::ImageInput::onResendTimeout()
{
	if( m_channel.fd() != -1 && m_resend_to != nullptr &&
		Gr::Image::peek(m_channel,m_image,m_type_str) )
	{
		ImageInputHandler * one_handler = m_resend_to ;
		m_resend_to = nullptr ;

		if( m_image.valid() )
			sendImageInput( m_image , one_handler ) ;
		else if( !m_image.empty() )
			sendNonImageInput( m_image , m_type_str , one_handler ) ;
	}
}

void Gv::ImageInput::onException( std::exception & )
{
	throw ;
}

// ==

Gv::ImageInputHandler::~ImageInputHandler()
{
}

void Gv::ImageInputHandler::onNonImageInput( ImageInputSource & , Gr::Image , const std::string & )
{
}

// ==

Gv::ImageInputConversion::ImageInputConversion() :
	type(none) ,
	scale(1) ,
	monochrome(false)
{
}

Gv::ImageInputConversion::ImageInputConversion( ImageInputConversion::Type type_ , int scale_ , bool monochrome_ ) :
	type(type_) ,
	scale(scale_) ,
	monochrome(monochrome_)
{
}

// ==

Gv::ImageInputTask::ImageInputTask() :
	m_handler(nullptr)
{
}

bool Gv::ImageInputTask::run( Gr::ImageConverter & converter , Gr::Image image_in )
{
	const bool to_raw = m_conversion.type == ImageInputConversion::to_raw ;
	const bool to_jpeg = m_conversion.type == ImageInputConversion::to_jpeg ;
	const bool keep_type = m_conversion.type == ImageInputConversion::none ;
	const bool is_jpeg = image_in.type().isJpeg() ;
	const bool is_raw = image_in.type().isRaw() ;
	const int channels = image_in.type().channels() ;

	G_DEBUG( "Gv::ImageInputTask::run: conversion=[t=" << m_conversion.type << ",m=" << m_conversion.monochrome << ",s=" << m_conversion.scale << "]" ) ;
	G_DEBUG( "Gv::ImageInputTask::run: input=[" << image_in.type() << "]" ) ;

	if( ( keep_type || (to_jpeg && is_jpeg) || (to_raw && is_raw) ) && 
		m_conversion.scale == 1 && (m_conversion.monochrome?1:3) == channels )
	{
		G_DEBUG( "Gv::ImageInputTask::run: no-op" ) ;
		m_image.clear() ; // no conversion
		return true ;
	}
	else if( to_jpeg || (keep_type && is_jpeg) )
	{
		G_DEBUG( "Gv::ImageInputTask::run: to-jpeg: s=" << m_conversion.scale << " m=" << m_conversion.monochrome ) ;
		return converter.toJpeg( image_in , m_image , m_conversion.scale , m_conversion.monochrome ) ;
	}
	else if( to_raw || (keep_type && is_raw) )
	{
		G_DEBUG( "Gv::ImageInputTask::run: to-raw: s=" << m_conversion.scale << " m=" << m_conversion.monochrome ) ;
		return converter.toRaw( image_in , m_image , m_conversion.scale , m_conversion.monochrome ) ;
	}
	else
	{
		G_DEBUG( "Gv::ImageInputTask::run: unsupported conversion" ) ;
		return false ;
	}
}

/// \file gvimageinput.cpp

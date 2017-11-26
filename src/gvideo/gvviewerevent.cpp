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
// gvviewerevent.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gtimer.h"
#include "gvviewerevent.h"
#include "geventloop.h"
#include "gstr.h"
#include "gassert.h"
#include <string>
#include <sstream>

Gv::ViewerEvent::ViewerEvent( const std::string &channel_name ) :
	m_channel(channel_name,true) ,
	m_button_down(false)
{
}

std::string Gv::ViewerEvent::channelName() const
{
	return m_channel.name() ;
}

bool Gv::ViewerEvent::init()
{
	return m_channel.open() ;
}

int Gv::ViewerEvent::fd() const
{
	return m_channel.fd() ;
}

bool Gv::ViewerEvent::receive( std::vector<char> & buffer , std::string * type_p )
{
	return m_channel.receive( buffer , type_p ) ;
}

int Gv::ViewerEvent::parse( const std::string & event , const std::string & key , int default_ )
{
	typedef std::string::size_type pos_t ;
	const pos_t npos = std::string::npos ;

	pos_t pos1 = event.find( "'"+key+"':" ) ;
	if( pos1 == npos ) return default_ ;
	pos_t pos2 = pos1 + key.length() + 3U ;
	pos_t pos3 = event.find( "," , pos2 ) ;
	if( pos3 == npos ) return default_ ;
	std::string value = event.substr( pos2 , pos3-pos2 ) ;
	G::Str::trim( value , G::Str::ws() ) ;
	if( !G::Str::isInt(value) ) return default_ ;
	return G::Str::toInt(value) ;
}

Gv::ViewerEvent::Info Gv::ViewerEvent::apply( const std::vector<char> & event )
{
	return apply( std::string(&event[0],event.size()) ) ;
}

Gv::ViewerEvent::Info Gv::ViewerEvent::apply( const std::string & event )
{
	Info info ;
	info.type = Invalid ;
	info.x = parse( event , "x" ) ;
	info.y = parse( event , "y" ) ;
	info.x_down = parse( event , "x0" ) ;
	info.y_down = parse( event , "y0" ) ;
	info.dx = parse( event , "dx" ) ;
	info.dy = parse( event , "dy" ) ;
	info.shift = parse( event , "shift" ) ;
	info.control = parse( event , "control" ) ;

	// messaging is not reliable, so button-down messages especially
	// can get lost -- therefore we synthesise our own button-up 
	// and button-down events depending on the button state 
	// information that appears in all three message types

	if( event.find("'button': 'down'") != std::string::npos )
	{
		if( !m_button_down )
		{
			m_button_down = true ;
			info.type = Down ;
		}
	}
	if( event.find("'event': 'move'") != std::string::npos )
	{
		info.type = m_button_down ? Drag : Move ;
	}
	if( event.find("'button': 'up'") != std::string::npos )
	{
		if( m_button_down )
		{
			m_button_down = false ;
			info.type = Up ;
		}
	}
	return info ;
}

// ==

/// \class Gv::ViewerEventMixinImp
/// A pimple-pattern implementation class for Gv::ViewerEventMixin.
///
class Gv::ViewerEventMixinImp : private GNet::EventHandler
{
public:
	ViewerEventMixinImp( ViewerEventMixin & outer , const std::string & channel ) ;
	virtual ~ViewerEventMixinImp() ;

private:
	virtual void readEvent() override ;
	virtual void onException( std::exception & ) override ;
	void onTimeout() ;

private:
	ViewerEventMixin & m_outer ;
	ViewerEvent m_viewer_event ;
	GNet::Timer<ViewerEventMixinImp> m_timer ;
	int m_fd ;
} ;

Gv::ViewerEventMixinImp::ViewerEventMixinImp( ViewerEventMixin & outer , const std::string & channel ) :
	m_outer(outer) ,
	m_viewer_event(channel) ,
	m_timer(*this,&ViewerEventMixinImp::onTimeout,*this) ,
	m_fd(-1)
{
	if( !channel.empty() )
		m_timer.startTimer( 1 ) ;
}

Gv::ViewerEventMixinImp::~ViewerEventMixinImp()
{
	if( m_fd != -1 )
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_viewer_event.fd()) ) ;
}

void Gv::ViewerEventMixinImp::onTimeout()
{
	if( m_viewer_event.init() )
	{
		G_ASSERT( m_fd == -1 ) ;
		if( m_fd == -1 )
		{
			m_fd = m_viewer_event.fd() ;
			GNet::EventLoop::instance().addRead( GNet::Descriptor(m_fd) , *this ) ;
		}
	}
	else
	{
		G_DEBUG( "Gv::ViewerEventMixinImp::onTimeout: viewer event channel not ready: " << m_viewer_event.channelName() ) ;
		m_timer.startTimer( 1 ) ;
	}
}

void Gv::ViewerEventMixinImp::readEvent()
{
	std::vector<char> buffer ;
	if( !m_viewer_event.receive( buffer ) )
		throw std::runtime_error( "viewer has gone away" ) ;

	m_outer.onViewerEvent( m_viewer_event.apply(buffer) ) ;
}

void Gv::ViewerEventMixinImp::onException( std::exception & )
{
	throw ;
}

// ==

Gv::ViewerEventMixin::ViewerEventMixin( const std::string & channel_name ) :
	m_imp(new ViewerEventMixinImp(*this,channel_name))
{
}

Gv::ViewerEventMixin::~ViewerEventMixin()
{
	delete m_imp ;
}

// ==

Gv::ViewerEvent::Info::Info() :
	type(Invalid) ,
	x(-1) ,
	y(-1) ,
	dx(0) ,
	dy(0) ,
	x_down(-1) ,
	y_down(-1) ,
	shift(false) ,
	control(false)
{
}

std::string Gv::ViewerEvent::Info::str() const
{
	std::ostringstream ss ;
	ss 
		<< type << "," 
		<< x << "," << y << "," 
		<< dx << "," << dy << "," 
		<< x_down << "," << y_down << "," 
		<< shift << "," << control ;
	return ss.str() ;
}


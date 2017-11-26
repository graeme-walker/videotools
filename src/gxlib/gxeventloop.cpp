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
// gxeventloop.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "glog.h"
#include "gxeventloop.h"
#include "gxdisplay.h"
#include "gxerror.h"
#include "gassert.h"
#include <stdexcept>

static Bool fn_always_true( ::Display * , XEvent * , XPointer )
{
	return 1 ;
}

// ==

GX::EventLoop::EventLoop( GX::Display & display ) :
	m_display(display) ,
	m_timeout(0)
{
}

GX::Display & GX::EventLoop::display()
{
	return m_display ;
}

void GX::EventLoop::run()
{
	for(;;)
	{
		runOnce() ;
	}
}


void GX::EventLoop::runToEmpty()
{
	for(;;)
	{
		XEvent event ;
		Bool rc = XCheckIfEvent( display().x() , &event , &fn_always_true , 0 ) ;
		if( !rc ) break ;
		handle( event ) ;
	}
}

void GX::EventLoop::runToTimeout()
{
	for(;;)
	{
		// break if timeout time is reached
		struct Timeval now ;
		if( m_timeout.is_set() && now > m_timeout )
		{
			m_timeout.reset() ;
			break ;
		}

		// prepare an interval for select()
		Timeval t ; 
		struct timeval * tp = nullptr ;
		if( m_timeout.is_set() )
		{
			t = m_timeout - now ;
			tp = &t.m_timeval ;
		}

		// fdset containing just the x-window display fd
		fd_set fds ;
		FD_ZERO( &fds ) ;
		int fd = ConnectionNumber(display().x()) ;
		FD_SET( fd , &fds ) ;

		// wait for an x-window event or timeout
		int rc = select( fd+1 , &fds , nullptr , &fds , tp ) ;
		if( rc < 0 ) 
			throw Error( "select failed" ) ;

		// process the event(s)
		handlePendingEvents() ;
	}
}

void GX::EventLoop::handlePendingEvents()
{
	XEvent event ;
	for(;;)
	{
		Bool rc = XCheckIfEvent( display().x() , &event , &fn_always_true , 0 ) ;
		if( !rc ) break ;
		handle( event ) ;
	}
}

void GX::EventLoop::startTimer( unsigned int usec )
{
	m_timeout = Timeval() + usec ;
}

void GX::EventLoop::runOnce()
{
	XEvent event ;
	XNextEvent( display().x() , &event ) ;
	handle( event ) ;
}

void GX::EventLoop::runUntil( int type )
{
	for(;;)
	{
		XEvent event ;
		XNextEvent( display().x() , &event ) ;
		handle( event ) ;
		if( event.type == type ) break ;
	}
}

GX::Window & GX::EventLoop::w( ::Window w )
{
	if( WindowMap::instance() == nullptr ) throw Error( "no window map" ) ;
	return WindowMap::instance()->find(w) ;
}

void GX::EventLoop::handle( XEvent & e )
{
	try
	{
		handleImp( e ) ;
	}
	catch( WindowMap::NotFound & )
	{
		// ignore it
		G_WARNING( "warning: event received for unknown window: type " << e.type ) ;
	}
}

void GX::EventLoop::handleImp( XEvent & e )
{
	if( e.type == Expose ) 
	{
		w(e.xexpose.window).onExposure( e.xexpose.x , e.xexpose.y , e.xexpose.width , e.xexpose.height ) ;
		w(e.xexpose.window).onPaint() ;
		w(e.xexpose.window).onExpose( e.xexpose ) ;
	}
	else if( e.type == KeyPress )
	{
		w(e.xkey.window).onKeyPress( e.xkey ) ;
	}
	else if( e.type == KeyRelease ) 
	{
		w(e.xkey.window).onKeyRelease( e.xkey ) ;
	}
	else if( e.type == ButtonPress )
	{
		bool shift = e.xbutton.state & ShiftMask ;
		bool control = e.xbutton.state & ControlMask ;
		if( e.xbutton.button == Button1 )
			w(e.xbutton.window).onLeftMouseButtonDown( e.xbutton.x , e.xbutton.y , shift , control ) ;
	}
	else if( e.type == ButtonRelease ) 
	{
		bool shift = e.xbutton.state & ShiftMask ;
		bool control = e.xbutton.state & ControlMask ;
		if( e.xbutton.button == Button1 )
			w(e.xbutton.window).onLeftMouseButtonUp( e.xbutton.x , e.xbutton.y , shift , control ) ;
	}
	else if( e.type == MapNotify ) 
	{
		if( ! w(e.xmap.window).onCreate() )
			throw Error( "window not created" ) ; // for now
		w(e.xmap.window).onShow() ;
		w(e.xmap.window).onMap( e.xmap ) ;
	}
	else if( e.type == ClientMessage )
	{
		w(e.xclient.window).onUser() ;
	}
	else if( e.type == MotionNotify )
	{
		w(e.xmotion.window).onMouseMove( e.xmotion.x , e.xmotion.y ) ;
	}
}

/// \file gxeventloop.cpp

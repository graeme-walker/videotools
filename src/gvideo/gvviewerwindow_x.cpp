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
// gvviewerwindow_x.cpp
// 

#include "gdef.h"
#include "gvviewerwindow_x.h"
#include "geventloop.h"
#include "gassert.h"
#include "glog.h"
#include <exception>

Gv::ViewerWindowX::ViewerWindowX( ViewerWindowHandler & window_handler , ViewerWindow::Config config , int dx , int dy ) :
	GX::Window(m_base_display,dx,dy,m_base_display.white(),config.m_title) ,
	m_xloop(m_base_xloop) ,
	m_window_handler(window_handler) ,
	m_canvas(*this) ,
	m_config(config) ,
	m_dx(dx) ,
	m_dy(dy) ,
	m_mask(dx,dy,m_config.m_mask_file)
{
	G_DEBUG( "Gv::ViewerWindowX::ctor: window-fd=" << windowDisplay().fd() ) ;
	GNet::EventLoop::instance().addRead( GNet::Descriptor(windowDisplay().fd()) , *this ) ;
}

Gv::ViewerWindowX::~ViewerWindowX()
{
	GNet::EventLoop::instance().dropRead( GNet::Descriptor(windowDisplay().fd()) ) ;
}

void Gv::ViewerWindowX::init()
{
	enableEvents( GX::Window::events(m_config.m_mouse_moves) ) ;
	show() ;
	m_xloop.runUntil( MapNotify ) ;
}

void Gv::ViewerWindowX::onChar( char c )
{
	m_window_handler.onChar( c ) ;
}

void Gv::ViewerWindowX::onLeftMouseButtonDown( int x , int y , bool shift , bool control )
{
	m_window_handler.onMouseButtonDown( x , y , shift , control ) ;
}

void Gv::ViewerWindowX::onLeftMouseButtonUp( int x , int y , bool shift , bool control )
{
	m_window_handler.onMouseButtonUp( x , y , shift , control ) ;
}

void Gv::ViewerWindowX::onMouseMove( int x , int y )
{
	m_window_handler.onMouseMove( x , y ) ;
}

void Gv::ViewerWindowX::display( int data_dx , int data_dy , int data_channels , const char * data_p , size_t data_n )
{
	const unsigned char * p = reinterpret_cast<const unsigned char*>(data_p) ;
	if( m_mask.empty() && data_channels == 3 )
		update( m_canvas.dx() , m_canvas.dy() , data_dx , data_dy , p , data_n ) ;
	else
		update( m_canvas.dx() , m_canvas.dy() , data_dx , data_dy , data_channels , p , data_n ) ;
	m_canvas.blit() ;

	// take this opportunity to poll the x event queue -- this 
	// improves reliability if the process is stressed
	int n = XPending(m_base_display.x()) ;
	if( n )
		m_xloop.handlePendingEvents() ;
}

void Gv::ViewerWindowX::update( int x_max , int y_max , int data_dx , int data_dy , 
	const unsigned char * p , size_t data_n )
{
	// optimised overload for rgb image and no mask

	if( !m_canvas.fastable() )
	{
		update( x_max , y_max , data_dx , data_dy , 3 , p , data_n ) ;
		return ;
	}

	G_ASSERT( data_n == static_cast<size_t>(data_dx*data_dy*3) ) ;
	int y_out = y_max - 1 ;
	for( int y = 0 ; y < data_dy && y < y_max ; y++ , y_out-- )
	{
		for( int x = 0 ; x < data_dx ; x++ )
		{
			const unsigned int r = *p++ ;
			const unsigned int g = *p++ ;
			const unsigned int b = *p++ ;
			if( x < x_max )
			{
				m_canvas.fastpoint( x , y_out , Gr::Colour(r,g,b) ) ;
			}
		}
	}
}

void Gv::ViewerWindowX::update( int x_max , int y_max , int data_dx , int data_dy , int data_channels , 
	const unsigned char * p , size_t data_n )
{
	size_t mask_offset = 0U ;
	int y_out = y_max - 1 ;
	for( int y = 0 ; y < data_dy && y < y_max ; y++ , y_out-- )
	{
		for( int x = 0 ; x < data_dx ; x++ )
		{
			const unsigned int r = *p++ ;
			const unsigned int g = data_channels > 1 ? *p++ : r ;
			const unsigned int b = data_channels > 2 ? *p++ : r ;
			if( x < x_max )
			{
				if( m_mask.masked( mask_offset++ ) )
					m_canvas.point( x , y_out , Gr::Colour(r>>1,g>>3,b>>3) ) ;
				else
					m_canvas.point( x , y_out , Gr::Colour(r,g,b) ) ;
			}
		}
	}
}

void Gv::ViewerWindowX::onExpose( XExposeEvent & event )
{
	G_DEBUG( "Gv::ViewerWindowX::onExpose: exposed" ) ;
	if( event.count == 0 )
		m_canvas.blit() ;
}

void Gv::ViewerWindowX::readEvent()
{
	int n = XEventsQueued( m_base_display.x() , QueuedAlready ) ;
	G_DEBUG( "Gv::ViewerWindowX::readEvent: event on xwindow fd: " << n ) ;
	if( n > 3 )
		G_WARNING( "Gv::ViewerWindowX::readEvent: backlog: " << n ) ;

	m_xloop.handlePendingEvents() ;
}

void Gv::ViewerWindowX::onException( std::exception & e )
{
	G_WARNING( "Gv::ViewerWindowX::onException: " << e.what() ) ;
	throw ;
}

int Gv::ViewerWindowX::dx() const
{
	return m_dx ;
}

int Gv::ViewerWindowX::dy() const
{
	return m_dy ;
}

/// \file gvviewerwindow_x.cpp

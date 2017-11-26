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
// gximage.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gximage.h"
#include "gxwindow.h"
#include "gxdisplay.h"
#include "gxvisual.h"
#include "gxerror.h"
#include "grlinedraw.h"
#include "gassert.h"
#include <cstdlib> // malloc
#include <cstring> // memset
#include <new> // std::bad_alloc

GX::Image::Image( Display & display , const GX::Window & window , int dx_in , int dy_in ) :
	m_display(display) ,
	m_image(nullptr)
{
	m_dx = dx_in ? dx_in : window.dx() ;
	m_dy = dy_in ? dy_in : window.dy() ;
	G_ASSERT( m_dx > 0 && m_dy > 0 ) ;

	char * p = nullptr ;
	Visual visual( m_display ) ;
	m_image = XCreateImage( display.x() , visual.x() , static_cast<unsigned int>(visual.depth()) , 
		ZPixmap , 0 , p , static_cast<unsigned int>(m_dx) , static_cast<unsigned int>(m_dy) , 8 , 0 ) ;
	if( m_image == nullptr )
		throw Error( "XCreateImage failed" ) ;

	int size = m_dy * m_image->bytes_per_line ;
	G_ASSERT( size >= 0 ) ;
	p = static_cast<char*>(std::malloc(static_cast<size_t>(size))) ;
	if( p == nullptr )
		throw std::bad_alloc() ; // malloc() failed
	m_image->data = p ;
}

GX::Image::Image( Display & display , XImage * p , int dx , int dy ) :
	m_display(display) ,
	m_image(p) ,
	m_dx(dx) ,
	m_dy(dy)
{
}

GX::Image::~Image()
{
	try
	{
		if( m_image )
			XDestroyImage( m_image ) ;
	}
	catch(...)
	{
	}
}

void GX::Image::blit( GX::Window & window , int src_x , int src_y , int dx , int dy , int dst_x , int dst_y )
{
	Context gc = m_display.defaultContext() ;
	blit( window , gc , src_x , src_y , dx , dy , dst_x , dst_y ) ;
}

void GX::Image::blit( GX::Window & window , Context & gc , 
	int src_x , int src_y , int dx , int dy , int dst_x , int dst_y )
{
	XPutImage( m_display.x() , window.x() , gc.x() , m_image , src_x , src_y , dst_x , dst_y , 
		static_cast<unsigned int>(dx) , static_cast<unsigned int>(dy) ) ;
}

void GX::Image::clear( bool white )
{
	std::memset( m_image->data , white?255:0 , m_image->bytes_per_line * m_image->height ) ;
}

unsigned long GX::Image::readPoint( int x , int y ) const
{
	return XGetPixel( m_image , x , y ) ;
}

void GX::Image::drawLine( int x1 , int y1 , int x2 , int y2 , unsigned long p , bool or_ )
{ 
	Gr::LineDraw<int> line_draw( x1 , y1 , x2 , y2 ) ;
	while( line_draw.more() )
	{
		for( int x = line_draw.x1() ; x <= line_draw.x2() ; x++ )
			drawPoint( x , line_draw.y() , p , or_ ) ;
	}
}

void GX::Image::drawLineAcross( int x1 , int x2 , int y , unsigned long p )
{ 
	if( y >= 0 && y < m_dy )
	{
		x1 = x1 < 0 ? 0 : ( x1 >= m_dx ? (m_dx-1) : x1 ) ;
		x2 = x2 < 0 ? 0 : ( x2 >= m_dx ? (m_dx-1) : x2 ) ;
		for( int x = x1 ; x <= x2 ; x++ )
			XPutPixel( m_image , x , y , p ) ;
	}
}

void GX::Image::drawLineDown( int x , int y1 , int y2 , unsigned long p )
{ 
	if( x >= 0 && x < m_dx )
	{
		y1 = y1 < 0 ? 0 : ( y1 >= m_dy ? (m_dy-1) : y1 ) ;
		y2 = y2 < 0 ? 0 : ( y2 >= m_dy ? (m_dy-1) : y2 ) ;
		for( int y = y1 ; y <= y2 ; y++ )
			XPutPixel( m_image , x , y , p ) ;
	}
}

/// \file gximage.cpp

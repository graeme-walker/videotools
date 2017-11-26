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
// gxpixmap.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxpixmap.h"
#include "gxwindow.h"
#include "gxdisplay.h"
#include "gxcontext.h"
#include "gxcolourmap.h"
#include "gximage.h"
#include "gxerror.h"
#include "gassert.h"

GX::Pixmap::Pixmap( Display & display , GX::Window & window , int dx , int dy ) :
	Drawable(display) ,
	m_display(display)
{
	m_dx = dx ? dx : window.dx() ;
	m_dy = dy ? dy : window.dy() ;
	G_ASSERT( m_dx >= 0 && m_dy >= 0 ) ;
	m_pixmap = XCreatePixmap( display.x() , window.x() , 
		static_cast<unsigned int>(m_dx) , static_cast<unsigned int>(m_dy) , 
		static_cast<unsigned int>(display.depth()) ) ;
}

GX::Pixmap::~Pixmap()
{
	try
	{
		XFreePixmap( m_display.x() , m_pixmap ) ;
	}
	catch(...)
	{
	}
}

::Pixmap GX::Pixmap::x()
{
	return m_pixmap ;
}

::Drawable GX::Pixmap::xd()
{
	return m_pixmap ;
}

int GX::Pixmap::dx() const
{
	return m_dx ;
}

int GX::Pixmap::dy() const
{
	return m_dy ;
}

void GX::Pixmap::blit( GX::Window & window , int src_x , int src_y , int dx , int dy , int dst_x , int dst_y )
{
	Context gc = m_display.defaultContext() ;
	blit( window , gc , src_x , src_y , dx , dy , dst_x , dst_y ) ;
}

void GX::Pixmap::blit( GX::Window & window , Context & gc , 
	int src_x , int src_y , int dx , int dy , int dst_x , int dst_y )
{
	G_ASSERT( dx >= 0 && dy >= 0 ) ;
	XCopyArea( m_display.x() , m_pixmap , window.x() , gc.x() , src_x , src_y , 
		static_cast<unsigned int>(dx) , static_cast<unsigned int>(dy) , dst_x , dst_y ) ;
}

void GX::Pixmap::readImage( unique_ptr<GX::Image> & image ) const
{
	unsigned int mask = static_cast<unsigned int>(~0) ; // callee ignores extraneous bits, so set them all
	XImage * p = XGetImage( m_display.x() , m_pixmap , 0 , 0 , 
		static_cast<unsigned int>(m_dx) , static_cast<unsigned int>(m_dy) , 
		mask , XYPixmap ) ;
	if( p == nullptr )
		throw Error( "XGetImage" ) ;
	image.reset( new Image( m_display , p , m_dx , m_dy ) ) ;
}

void GX::Pixmap::clear()
{
	Context gc = m_display.defaultContext() ;
	drawRectangle( gc , 0 , 0 , m_dx , m_dy ) ; // Drawable base-class
}

/// \file gxpixmap.cpp

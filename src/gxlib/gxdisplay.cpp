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
// gxdisplay.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxdisplay.h"
#include "gxerror.h"
#include <sstream>

GX::Display::Display()
{
	m_display = XOpenDisplay(0) ;
	if( m_display == nullptr )
		throw Error( "XOpenDisplay" ) ;

	m_black = XBlackPixel( m_display , DefaultScreen(m_display) ) ;
	m_white = XWhitePixel( m_display , DefaultScreen(m_display) ) ;

	int screen = XDefaultScreen( m_display ) ;
	m_depth = XDefaultDepth( m_display , screen ) ;
	m_dx = XDisplayWidth( m_display , screen ) ;
	m_dy = XDisplayHeight( m_display , screen ) ;
}

GX::Display::~Display()
{
	try
	{
		XCloseDisplay( m_display ) ;
	}
	catch(...)
	{
	}
}

::Display * GX::Display::x()
{
	return m_display ;
}

int GX::Display::fd() const
{
	return ConnectionNumber(m_display) ; // ie. m_display->fd ;
}

::Colormap GX::Display::defaultColourmap()
{
	int screen = XDefaultScreen( m_display ) ;
	return XDefaultColormap( m_display , screen ) ;
}

GX::Context GX::Display::defaultContext()
{
	int screen = XDefaultScreen( m_display ) ;
	return Context( *this , XDefaultGC(m_display,screen) ) ;
}

int GX::Display::white() const
{
	return m_white ;
}

int GX::Display::black() const
{
	return m_black ;
}

int GX::Display::depth() const
{
	return m_depth ;
}

int GX::Display::dx() const
{
	return m_dx ;
}

int GX::Display::dy() const
{
	return m_dy ;
}

std::list<GX::Display::PixmapFormat> GX::Display::pixmapFormats() const
{
	int n = 0 ;
	XPixmapFormatValues * p = XListPixmapFormats( m_display , &n ) ;
	if( p == nullptr )
		throw Error( "XListPixmapFormats" ) ;

	std::list<PixmapFormat> result ;
	for( int i = 0 ; i < n ; i++ )
	{
		PixmapFormat item = { p[i].depth , p[i].bits_per_pixel , p[i].scanline_pad } ;
		result.push_back( item ) ;
	}
	XFree( p ) ;
	return result ;
}

/// \file gxdisplay.cpp

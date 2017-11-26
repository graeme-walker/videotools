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
// gxcolourmap.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxcolourmap.h"
#include "gxdisplay.h"
#include "gxvisual.h"
#include "gxwindow.h"
#include "gxerror.h"
#include "grcolour16.h"
#include <sstream>

GX::ColourMap::ColourMap( Display & display , bool greyscale256 ) :
	m_display(display) ,
	m_size(greyscale256?256U:16U)
{
	m_map.reserve( m_size ) ;
	GX::Window window( display , 1 , 1 ) ;
	Visual default_visual( display ) ;
	create( display , window.x() , default_visual.x() , greyscale256 ) ;
}

GX::ColourMap::ColourMap( Display & display , GX::Window & window , ::Visual * visual ) :
	m_display(display)
{
	create( display , window.x() , visual , false ) ;
}

void GX::ColourMap::create( Display & display , ::Window window , ::Visual * visual , bool greyscale256 )
{
	m_x = XCreateColormap( display.x() , window , visual , AllocNone ) ;
	if( greyscale256 )
		setGrey256() ;
	else
		setColour16() ;
}

unsigned int GX::ColourMap::size() const
{
	return m_size ;
}

void GX::ColourMap::setGrey256()
{
	int n = 0 ;
	for( int i = 0 ; i < 256 ; i++ , n += 257 )
	{
		XColor xcolour ;
		xcolour.pixel = 0 ;
		xcolour.flags = 0 ;
		xcolour.red = n ;
		xcolour.green = n ;
		xcolour.blue = n ;
		Status rc = XAllocColor( m_display.x() , m_x , &xcolour ) ; 
		if( rc == 0 ) throw Error( "XAllocColor" ) ;
		m_map.push_back(xcolour.pixel) ;
	}
}

void GX::ColourMap::setColour16()
{
	for( unsigned i = 0U ; i < 16U ; i++ )
		addColour16( Gr::colour16::r(i) , Gr::colour16::g(i) , Gr::colour16::b(i) ) ;
}

void GX::ColourMap::addColour16( int r , int g , int b )
{
	XColor xcolour ;
	xcolour.pixel = 0 ;
	xcolour.flags = 0 ;
	xcolour.red = 32761 * r ;
	xcolour.green = 32761 * g ;
	xcolour.blue = 32761 * b ;
	Status rc = XAllocColor( m_display.x() , m_x , &xcolour ) ; 
	if( rc == 0 ) throw Error( "XAllocColor" ) ;
	m_map.push_back(xcolour.pixel) ;
}

GX::ColourMap::~ColourMap()
{
	try
	{
		XFreeColormap( m_display.x() , m_x ) ;
	}
	catch(...)
	{
	}
}

::Colormap GX::ColourMap::x()
{
	return m_x ;
}

int GX::ColourMap::find( unsigned long p ) const
{
	int n = static_cast<int>(size()) ;
	for( int i = 0 ; i < n ; i++ )
	{
		if( get(i) == p )
			return i ;
	}
	std::ostringstream ss ;
	ss << p << " not in" ;
	for( int i = 0U ; i < n ; i++ )
	{
		ss << " " << get(i) ;
	}
	throw Error( "GX::ColourMap::find: invalid pixel value: " + ss.str() ) ;
	return 0 ;
}

/// \file gxcolourmap.cpp

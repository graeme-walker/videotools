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
// gxdrawable.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxdrawable.h"
#include "gxdisplay.h"
#include "gxcontext.h"
#include "gassert.h"

GX::Drawable::Drawable( Display & display ) :
	m_display(display)
{
}

GX::Drawable::~Drawable()
{
}

void GX::Drawable::drawPoint( Context & gc , int x , int y )
{
	XDrawPoint( m_display.x() , xd() , gc.x() , x , y ) ;
}

void GX::Drawable::drawLine( Context & gc , int x1 , int y1 , int x2 , int y2 )
{
	XDrawLine( m_display.x() , xd() , gc.x() , x1 , y1 , x2 , y2 ) ;
}

void GX::Drawable::drawRectangle( Context & gc , int x , int y , int dx , int dy )
{
	G_ASSERT( dx >= 0 && dy >= 0 ) ;
	XFillRectangle( m_display.x() , xd() , gc.x() , x , y , 
		static_cast<unsigned int>(dx) , static_cast<unsigned int>(dy) ) ;
}

void GX::Drawable::drawArc( Context & gc , int x , int y , unsigned int width , 
	unsigned int height , int angle1 , int angle2 )
{
	XDrawArc( m_display.x() , xd() , gc.x() , x , y , width , height , angle1 , angle2 ) ;
}

/// \file gxdrawable.cpp

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
// gxcontext.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxcontext.h"
#include "gxdisplay.h"
#include "gxdrawable.h"
#include "gxerror.h"

GX::Context::Context( Display & display , Drawable & drawable ) :
	m_display(display) ,
	m_owned(true)
{
	unsigned long value_mask = 0UL ;
	XGCValues * values = nullptr ;
	m_context = XCreateGC( display.x() , drawable.xd() , value_mask , values ) ;
}

GX::Context::Context( Display & display , ::GC gc ) :
	m_display(display) ,
	m_context(gc) ,
	m_owned(false)
{
}

GX::Context::Context( const Context & other ) :
	m_display(other.m_display) ,
	m_context(other.m_context) ,
	m_owned(other.m_owned)
{
}

GX::Context::~Context()
{
	try
	{
		if( m_owned )
			XFreeGC( m_display.x() , m_context ) ;
	}
	catch(...)
	{
	}
}

::GC GX::Context::x()
{
	return m_context ;
}

void GX::Context::setForeground( unsigned long c )
{
	XSetForeground( m_display.x() , m_context , c ) ;
}

void GX::Context::setLineWidth( unsigned long n )
{
	XSetLineAttributes( m_display.x() , m_context , n ,
		LineSolid , CapRound , JoinRound ) ;
}


/// \file gxcontext.cpp

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
// gxvisual.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxdisplay.h"
#include "gxvisual.h"
#include "gxerror.h"
#include <sstream>

GX::Visual::Visual( Display & display )
{
	int screen = XDefaultScreen( display.x() ) ;
	::Visual * vp = XDefaultVisual( display.x() , screen ) ;
	if( vp == nullptr ) throw Error( "XDefaultVisual" ) ;
	init( display , vp->visualid ) ;
}

GX::Visual::Visual( Display & display , const Info & info )
{
	init( display , info.visual->visualid ) ;
}

void GX::Visual::init( Display & display , ::VisualID id )
{
	std::list<Info> info_list = infoList( display ) ;
	std::list<Info>::iterator p = info_list.begin() ;
	for( ; p != info_list.end() ; ++p )
	{
		if( (*p).visualid == id )
		{
			m_info = *p ;
			break ;
		}
	}
	if( p == info_list.end() )
		throw Error( "unknown visual" ) ;
}

GX::Visual::~Visual()
{
}

::Visual * GX::Visual::x()
{
	return m_info.visual ;
}

std::list<GX::Visual::Info> GX::Visual::infoList( Display & display )
{
	Info template_ ;
	template_.screen = XDefaultScreen( display.x() ) ;

	int n = 0 ;
	XVisualInfo * vp = XGetVisualInfo( display.x() , VisualScreenMask , &template_ , &n ) ;
	if( vp == nullptr )
		throw Error( "XGetVisualInfo" ) ;

	std::list<Info> result ;
	for( int i = 0 ; i < n ; i++ )
	{
		Info copy( vp[i] ) ;
		result.push_back( copy ) ;
	}
	XFree( vp ) ;
	return result ;
}

::VisualID GX::Visual::id() const
{
	check( m_info.visualid == XVisualIDFromVisual(m_info.visual) ) ;
	return m_info.visualid ;
}

void GX::Visual::check( bool b )
{
	if( ! b )
		throw Error( "internal error" ) ;
}

int GX::Visual::type() const
{
	return m_info.c_class ;
}

int GX::Visual::depth() const
{
	return m_info.depth ;
}

std::string GX::Visual::typeName() const
{
	if( type() == PseudoColor ) return "PseudoColour" ;
	if( type() == GrayScale ) return "GreyScale" ;
	if( type() == DirectColor ) return "DirectColour" ;
	if( type() == TrueColor ) return "TrueColour" ;
	if( type() == StaticColor ) return "StaticColour" ;
	if( type() == StaticGray ) return "StaticGrey" ;
	std::stringstream ss ;
	ss << "unrecognised visual class [" << static_cast<int>(type()) << "]" ;
	throw Error( ss.str() ) ;
}

/// \file gxvisual.cpp

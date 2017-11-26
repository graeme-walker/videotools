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
// gvoverlay.cpp
//

#include "gdef.h"
#include "gvoverlay.h"
#include "grglyph.h"
#include "grcolourspace.h"
#include "gdatetime.h"
#include "gstr.h"
#include "gdate.h"
#include "gtime.h"
#include <sstream>

namespace
{
	void make_dim( bool monochrome , unsigned char & r , unsigned char & g , unsigned char & b )
	{
		unsigned int y = monochrome ? r : Gr::ColourSpace::y_int(r,g,b) ;
		unsigned int u = monochrome ? 0 : Gr::ColourSpace::u_int(r,g,b) ;
		unsigned int v = monochrome ? 0 : Gr::ColourSpace::v_int(r,g,b) ;
		y /= 2 ;
		r = Gr::ColourSpace::r_int( y , u , v ) ;
		g = Gr::ColourSpace::g_int( y , u , v ) ;
		b = Gr::ColourSpace::b_int( y , u , v ) ;
	}
}

void Gv::Overlay::operator()( int offset_x , int offset_y , bool on )
{
	int x = m_x0 + offset_x ;
	int y = m_y0 + offset_y ;
	if( x >= 0 && y >= 0 && x < m_data.dx() && y < m_data.dy() )
	{
		unsigned char r = m_data.r( x , y ) ;
		unsigned char g = m_data.g( x , y ) ;
		unsigned char b = m_data.b( x , y ) ;

		if( on ) 
			r = g = b = 255U ;
		else
			make_dim( false , r , g , b ) ;

		m_data.rgb( x , y , r , g , b ) ;
	}
}

Gv::Overlay::Overlay( Gr::ImageData & data ) :
	m_data(data)
{
}

void Gv::Overlay::write( int x0 , int y0 , const std::string & format , const Gv::Timezone & tz )
{
	std::string text = format ;

	G::EpochTime t = G::DateTime::now() + tz.seconds() ;
	G::Str::replaceAll( text , "%t" , timestamp(t,tz) ) ;
	G::Str::replaceAll( text , "%z" , zone(tz) ) ;
	G::Str::replaceAll( text , "%D" , date(t,tz) ) ;
	G::Str::replaceAll( text , "%T" , time(t,tz) ) ;

	m_x0 = x0 ;
	m_y0 = y0 ;
	Gr::Glyph::output( text , *this ) ;

	int w = static_cast<int>(text.length()) * 9 ;
	for( int x = 0 ; x < (w+1) ; x++ )
	{
		operator()( x , -1 , false ) ;
		operator()( x , +8 , false ) ;
	}
	for( int y = -1 ; y < 8 ; y++ )
	{
		operator()( w , y , false ) ;
	}
}

void Gv::Overlay::timestamp( int x0 , int y0 , const Gv::Timezone & tz )
{
	write( x0 , y0 , "%t" , tz ) ;
}

std::string Gv::Overlay::timestamp( G::EpochTime t , const Gv::Timezone & tz )
{
	return datetime(t,tz) + " " + zone(tz) ;
}

std::string Gv::Overlay::zone( const Gv::Timezone & tz )
{
	return tz.zero() ? std::string("UTC") : tz.str() ;
}

std::string Gv::Overlay::date( G::EpochTime t , const Gv::Timezone & tz )
{
	G::DateTime::BrokenDownTime tm = G::DateTime::utc( t ) ;
	return G::Date(tm).string(G::Date::yyyy_mm_dd_slash) ;
}

std::string Gv::Overlay::time( G::EpochTime t , const Gv::Timezone & tz )
{
	G::DateTime::BrokenDownTime tm = G::DateTime::utc( t ) ;
	return G::Time(tm).hhmmss(":") ;
}

std::string Gv::Overlay::datetime( G::EpochTime t , const Gv::Timezone & tz )
{
	return date(t,tz) + " " + time(t,tz) ;
}

/// \file gvoverlay.cpp

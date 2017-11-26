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
// gvviewerwindow_ansi.cpp
// 

#include "gdef.h"
#include "gvviewerwindow_ansi.h"
#include "gstr.h"
#include "genvironment.h"
#include "grcolourspace.h"
#include "grscaler.h"
#include "gassert.h"
#include <exception>

namespace
{
	void output( const char * p )
	{
		std::cout << p << std::flush ;
	}
	int env( const std::string & name )
	{
		std::string s = G::Environment::get( name , "" ) ;
		if( s.empty() )
			throw std::runtime_error( "no environment variable '"+name+"'; try \"export "+name+"\"" ) ;
		if( !G::Str::isUInt(s) )
			throw std::runtime_error( "invalid value of envionment variable '"+name+"'" ) ;
		int result = static_cast<int>( G::Str::toUInt(s) ) ;
		if( result <= 0 )
			throw std::runtime_error( "invalid value of envionment variable '"+name+"'" ) ;
		return result ;
	}
}

Gv::ViewerWindowAnsi::ViewerWindowAnsi( ViewerWindowHandler & handler , ViewerWindowConfig config , int dx , int dy ) :
	m_handler(handler) ,
	m_config(config) ,
	m_mask(dx,dy,m_config.m_mask_file)
{
	output( "\033[2J\033[1;1H" ) ;
}

Gv::ViewerWindowAnsi::~ViewerWindowAnsi()
{
}

void Gv::ViewerWindowAnsi::init()
{
}

void Gv::ViewerWindowAnsi::display( int data_dx , int data_dy , int data_channels , const char * data_p , size_t data_n )
{
	output( "\033[2J\033[1;1H" ) ;
	int dx = this->dx() ;
	int dy = this->dy() ;

	std::string line ;
	line.reserve( dx + 2 ) ;
	int x = -1 ;
	int y = -1 ;
	for( Gr::Scaler y_scaler(dy,data_dy) ; !!y_scaler ; ++y_scaler )
	{
		int display_y = y_scaler.first() ;
		if( display_y == y )
			continue ;
		y = display_y ;

		if( !line.empty() ) 
			std::cout << line << std::flush ;

		line.clear() ;
		for( Gr::Scaler x_scaler(dx,data_dx) ; !!x_scaler ; ++x_scaler )
		{
			int display_x = x_scaler.first() ;
			if( display_x == x )
				continue ;
			x = display_x ;

			int data_x = x_scaler.second() ;
			int data_y = y_scaler.second() ;

			int data_offset = data_y * data_dx + data_x ;
			if( data_channels != 1 )
				data_offset *= data_channels ;
			const unsigned char * p = reinterpret_cast<const unsigned char *>(data_p+data_offset) ;

			unsigned int r = *p++ ;
			unsigned int g = data_channels > 1 ? *p++ : r ;
			unsigned int b = data_channels > 2 ? *p++ : r ;

			unsigned int luma = Gr::ColourSpace::yuv_int(Gr::ColourSpace::triple<unsigned char>(r,g,b)).y() ;
			static const char * luma_map = " .,:;|(){}%%$@@##" ;

			bool masked = m_mask.masked(data_x,data_y) ;
			char c = masked ? '_' : luma_map[(luma/16U)%16U] ;

			if( y == 0 && x < int(m_config.m_title.length()) )
				line.append( 1U , m_config.m_title.at(x) ) ;
			else
				line.append( 1U , c ) ;
		}
	}
	if( !line.empty() )
		line.erase( line.begin() + (line.size()-1U) ) ;
	std::cout << line << std::flush ;
}

int Gv::ViewerWindowAnsi::dx() const
{
	return env( "COLUMNS" ) ;
}

int Gv::ViewerWindowAnsi::dy() const
{
	return env( "LINES" ) ;
}

/// \file gvviewerwindow_ansi.cpp

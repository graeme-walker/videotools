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
// gvviewerwindow_curses.cpp
// 

#include "gdef.h"
#include "gvviewerwindow_curses.h"
#include "grscaler.h"
#include "grcolourspace.h"
#include "gcleanup.h"
#include "gassert.h"
#include <exception>
#include <cstdlib> // putenv
#include <cstring> // strdup
#include <curses.h>

namespace
{
	void end_curses( G::SignalSafe , const char * )
	{
		static bool ended = false ;
		if( !ended ) endwin() ;
		ended = true ;
	}
}

Gv::ViewerWindowCurses::ViewerWindowCurses( ViewerWindowHandler & handler , ViewerWindowConfig config , int dx , int dy ) :
	m_handler(handler) ,
	m_config(config) ,
	m_mask(dx,dy,m_config.m_mask_file)
{
	initscr() ;

	// unset lines and columns so that xterm resizing works 
	putenv( strdup( "COLUMNS=" ) ) ; // leak
	putenv( strdup( "LINES=" ) ) ; // leak

	G::Cleanup::add( end_curses , 0 ) ;
	cbreak() ;
	if( has_colors() ) 
	{
		start_color() ;
		init_pair( 1, COLOR_RED, COLOR_BLACK ) ;
		init_pair( 2, COLOR_GREEN, COLOR_BLACK ) ;
		init_pair( 3, COLOR_YELLOW, COLOR_BLACK ) ;
		init_pair( 4, COLOR_BLUE, COLOR_BLACK ) ;
		init_pair( 5, COLOR_MAGENTA, COLOR_BLACK ) ;
		init_pair( 6, COLOR_CYAN, COLOR_BLACK ) ;
		init_pair( 7, COLOR_WHITE, COLOR_BLACK ) ;
	}
}

Gv::ViewerWindowCurses::~ViewerWindowCurses()
{
	end_curses( G::SignalSafe() , 0 ) ;
}

void Gv::ViewerWindowCurses::init()
{
}

void Gv::ViewerWindowCurses::display( int data_dx , int data_dy , int data_channels , const char * data_p , size_t data_n )
{
	int x = -1 ;
	int y = -1 ;
	for( Gr::Scaler y_scaler(LINES,data_dy) ; !!y_scaler ; ++y_scaler )
	{
		int display_y = y_scaler.first() ;
		G_ASSERT( display_y < LINES ) ;
		if( display_y == y )
			continue ;
		y = display_y ;

		for( Gr::Scaler x_scaler(COLS,data_dx) ; !!x_scaler ; ++x_scaler )
		{
			int display_x = x_scaler.first() ;
			G_ASSERT( display_x < COLS ) ;
			if( display_x == x )
				continue ;
			x = display_x ;

			int data_x = x_scaler.second() ;
			int data_y = y_scaler.second() ;
			G_ASSERT( data_x < data_dx ) ;
			G_ASSERT( data_y < data_dy ) ;

			int data_offset = data_y * data_dx + data_x ;
			if( data_channels != 1 )
				data_offset *= data_channels ;
			const unsigned char * p = reinterpret_cast<const unsigned char *>(data_p+data_offset) ;

			const unsigned int r = *p++ ;
			const unsigned int g = data_channels > 1 ? *p++ : r ;
			const unsigned int b = data_channels > 2 ? *p++ : r ;

			const unsigned int range = std::max(r,std::max(g,b)) ;
			const unsigned int threshold = range/2U + range/4U ;
			const unsigned int index = ((r>threshold)?1U:0U) | ((g>threshold)?2U:0U) | ((b>threshold)?4U:0U) ;
			const chtype colour = m_mask.masked(data_x,data_y) ? COLOR_PAIR(1) : COLOR_PAIR(index) ;

			const unsigned int luma = Gr::ColourSpace::yuv_int(Gr::ColourSpace::triple<unsigned char>(r,g,b)).y() ;
			static const char * luma_map = " .,:;|(){}%%$@@##" ;
			const chtype c = luma_map[(luma/16U)%16U] ;

			move( display_y , display_x ) ;
			addch( c | colour ) ;
		}
	}

	if( !m_config.m_title.empty() )
	{
		move( 0 , 1 ) ;
		for( std::string::iterator p = m_config.m_title.begin() ; p != m_config.m_title.end() ; ++p )
			addch( *p ) ;
		move( 0 , 0 ) ;
	}

	refresh() ;
}

int Gv::ViewerWindowCurses::dx() const
{
	return COLS ;
}

int Gv::ViewerWindowCurses::dy() const
{
	return LINES ;
}

/// \file gvviewerwindow_curses.cpp

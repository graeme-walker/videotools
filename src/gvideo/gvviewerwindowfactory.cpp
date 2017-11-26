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
// gvviewerwindowfactory.cpp
// 

#include "gdef.h"
#include "gtest.h"
#include "gvviewerwindow.h"
#include "gvviewerwindow_ansi.h"
#include "genvironment.h"

#if GCONFIG_HAVE_X11
#include "gvviewerwindow_x.h"
#endif

#if GCONFIG_HAVE_CURSES
#include "gvviewerwindow_curses.h"
#endif

Gv::ViewerWindow * Gv::ViewerWindow::create( ViewerWindowHandler & window_handler , ViewerWindowConfig config , int dx , int dy )
{
	std::string display = G::Environment::get( "DISPLAY" , "" ) ;

	if( display == "tty" || G::Test::enabled("viewer-window-tty") ) 
		return new ViewerWindowAnsi( window_handler , config , dx , dy ) ;

	#if GCONFIG_HAVE_CURSES
	if( display == "curses" || G::Test::enabled("viewer-window-curses") ) 
		return new ViewerWindowCurses( window_handler , config , dx , dy ) ;
	#endif

	#if GCONFIG_HAVE_X11
	if( !display.empty() )
		return new ViewerWindowX( window_handler , config , dx , dy ) ;
	#endif

	#if GCONFIG_HAVE_CURSES
	return new ViewerWindowCurses( window_handler , config , dx , dy ) ;
	#else
	return new ViewerWindowAnsi( window_handler , config , dx , dy ) ;
	#endif
}

/// \file gvviewerwindowfactory.cpp

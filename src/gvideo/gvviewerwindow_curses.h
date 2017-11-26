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
///
/// \file gvviewerwindow_curses.h
///

#ifndef GV_VIEWERWINDOW_CURSES__H
#define GV_VIEWERWINDOW_CURSES__H

#include "gdef.h"
#include "gvviewerwindow.h"
#include "gvmask.h"

namespace Gv
{
	class ViewerWindowCurses ;
}

/// \class Gv::ViewerWindowCurses
/// A pimple-pattern implementation class for ViewerWindow that uses curses.
/// 
class Gv::ViewerWindowCurses : public ViewerWindow
{
public:
	ViewerWindowCurses( ViewerWindowHandler & , ViewerWindowConfig , int dx , int dy ) ;
		///< Constructor.

	virtual ~ViewerWindowCurses() ;
		///< Destructor.

	virtual void init() override ;
		///< Override from ViewerWindow.

	virtual void display( int , int , int , const char * , size_t ) override ;
		///< Override from ViewerWindow.

	virtual int dx() const override ;
		///< Override from ViewerWindow.

	virtual int dy() const override ;
		///< Override from ViewerWindow.

private:
	ViewerWindowCurses( const ViewerWindowCurses & ) ;
	void operator=( const ViewerWindowCurses & ) ;

private:
	ViewerWindowHandler & m_handler ;
	ViewerWindowConfig m_config ;
	Gv::Mask m_mask ;
} ;

#endif

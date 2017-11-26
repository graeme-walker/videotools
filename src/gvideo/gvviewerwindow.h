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
/// \file gvviewerwindow.h
///

#ifndef GV_VIEWERWINDOW__H
#define GV_VIEWERWINDOW__H

#include "gdef.h"
#include <exception>

namespace Gv
{
	class ViewerWindowConfig ;
	class ViewerWindowHandler ;
	class ViewerWindow ;
}

/// \class Gv::ViewerWindowConfig
/// A configuration structure for Gv::ViewerWindow.
/// 
struct Gv::ViewerWindowConfig
{
	std::string m_title ;
	std::string m_mask_file ;
	bool m_static ;
	bool m_mouse_moves ;
} ;

/// \class Gv::ViewerWindowHandler
/// A callback interface for Gv::ViewerWindow.
/// 
class Gv::ViewerWindowHandler
{
public:
	virtual void onChar( char ) = 0 ;
		///< Called when a key is pressed.

	virtual void onMouseButtonDown( int x , int y , bool shift , bool control ) = 0 ;
		///< Called when the left mouse button is pressed.

	virtual void onMouseButtonUp( int x , int y , bool shift , bool control ) = 0 ;
		///< Called when the left mouse button is released.

	virtual void onMouseMove( int x , int y ) = 0 ;
		///< Called when the mouse moves (typically also depending on whether
		///< ViewerWindowConfig::m_mouse_moves is set). Coordinates can be negative 
		///< if the mouse is moved outside the window with the button pressed.

	virtual void onInvalid() = 0 ;
		///< Called when the window needs to be re-display()ed.

	virtual ~ViewerWindowHandler() {}
		///< Destructor.
} ;

/// \class Gv::ViewerWindow
/// An abstract base class for a viewer window. The concrete derived class 
/// is chosen by the factory class.
/// 
class Gv::ViewerWindow
{
public:
	typedef ViewerWindowHandler Handler ;
	typedef ViewerWindowConfig Config ;

	static ViewerWindow * create( Handler & , ViewerWindow::Config , int image_dx , int image_dy ) ;
		///< A factory function that returns a new'ed ViewerWindow.
		///< 
		///< The dx and dy parameters are the size of the image
		///< that triggered the creation of the window, not necessarily
		///< the size of the window.
		///< 
		///< The new window should have its init() method called immediately
		///< after construction.

	virtual void init() = 0 ;
		///< An initialisation function that is called after contstruction.
		///< Keeping it as a separate function makes cleanup simpler in the 
		///< face of exceptions, for some derived classes.

	virtual void display( int dx , int dy , int channels , const char * , size_t ) = 0 ;
		///< Displays the given image.

	virtual int dx() const = 0 ;
		///< Returns the width of the window. This may not be the same as what was
		///< passed to the constructor. The mouse coordinates relate to this value.

	virtual int dy() const = 0 ;
		///< Returns the height of the window. This may not be the same as what was
		///< passed to the constructor. The mouse coordinates relate to this value.

	virtual ~ViewerWindow() ;
		///< Destructor.
} ;

#endif

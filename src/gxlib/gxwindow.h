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
/// \file gxwindow.h
///

#ifndef GX_WINDOW_H
#define GX_WINDOW_H

#include "gdef.h"
#include "gxdef.h"
#include "gxdrawable.h"
#include "gxcapture.h"
#include "gxeventhandler.h"
#include <string>

namespace GX
{
	class Window ;
	class Display ;
	class ColourMap ;
	class Capture ;
}

/// \class GX::Window
/// A window class that is-a GX::Drawable and a GX::EventHandler.
/// 
/// The window is empty and inherently drawable, and this class does not create
/// any sub-windows or embedded drawables. However, if a separate drawable is
/// used then it is typically blitted into this window in response to this
/// window's expose events.
/// 
class GX::Window : public GX::Drawable , public GX::EventHandler
{
public:
	Window( GX::Display & , int dx , int dy ) ;
		///< Constructor. The display reference is kept.

	Window( GX::Display & , int dx , int dy , int background , const std::string & title = std::string() ) ;
		///< Constructor for a window with a solid background. The display 
		///< reference is kept.

	virtual ~Window() ;
		///< Destructor.

	void clear() ;
		///< Clears the window.

	void show() ;
		///< Shows ("maps") the window.

	::Window x() ;
		///< Returns the X object.

	int dx() const ;
		///< Returns the current width.

	int dy() const ;
		///< Returns the current hieght.

	virtual ::Drawable xd() ;
		///< From Drawable.

	void enableEvents( long mask ) ;
		///< Enables the specified event types.

	static long events( bool with_mouse_moves ) ;
		///< Returns a default event mask for enableEvents().

	void install( GX::ColourMap & ) ;
		///< Installs a colourmap.

	GX::Display & windowDisplay() ;
		///< Returns a reference to the display
		///< as passed in to the ctor.

	const GX::Display & windowDisplay() const ;
		///< Const overload.

	GX::Capture * capture() ;
		///< Starts capturing the mouse. Returns a new-ed Capture pointer 
		///< that should be put into a unique_ptr.

	virtual void onKeyPress( ::XKeyEvent & ) override ;
		///< An override from EventHandler that calls onKey() doing the
		///< translation from opaque keycode to meaningful keysym.

	virtual void onKey( const std::string & key ) ;
		///< Called from this class's override of EventHandler::onKeyPress(). 
		///< The parameter is something like "a", "X", "F1", "END", "^C", etc.

	virtual void onChar( char c ) ;
		///< Called from this class's override of EventHandler::onKeyPress()
		///< in the case of simple character keys.

	void sendUserEvent() ;
		///< Posts a message back to itself so that onUser() gets called.

	std::string key() const ;
		///< Returns the last onKey() string.

	void invalidate() ;
		///< Invalidates the window by posting an expose event (see
		///< GX::EventHandler::onExpose()). This is not normally needed 
		///< because the X server will generate all the necessary expose 
		///< events without the help of user code.

private:
	Window( const Window & ) ;
	void operator=( const Window & ) ;
	void create( GX::Display & , int , int , unsigned long , const ::XSetWindowAttributes & ) ;
	static std::string keysymToString( ::KeySym , bool ) ;

private:
	GX::Display & m_display ;
	::Window m_window ;
	int m_dx ;
	int m_dy ;
	std::string m_key ;
} ;

#endif

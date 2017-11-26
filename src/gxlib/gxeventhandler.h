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
/// \file gxeventhandler.h
///

#ifndef GX_EVENT_HANDLER_H
#define GX_EVENT_HANDLER_H

#include "gdef.h"
#include "gxdef.h"

namespace GX
{
	class EventHandler ;
}

/// \class GX::EventHandler
/// An interface for delivering GX::EventLoop events, with do-nothing default method implementations.
/// 
/// One Xlib event can trigger several different callbacks methods at this interface in order to 
/// resemble other windowing systems.
/// 
class GX::EventHandler
{
public:
	virtual ~EventHandler() ;
		///< Destructor.

	virtual bool onIdle() ;
		///< Called by GX::Application::run() once the event queue is empty.
		///< Returns false when idle work is complete.

	virtual void onLeftMouseButtonDown( int x , int y , bool shift , bool control ) ;
		///< Called for a left-mouse-button-down event.

	virtual void onLeftMouseButtonUp( int x , int y , bool shift , bool control ) ;
		///< Called for a left-mouse-button-up event.

	virtual void onMouseMove( int x , int y ) ;
		///< Called for a mouse-move event.

	virtual void onExposure( int x , int y , int dx , int dy ) ;
		///< Called first for a window-expose event.

	virtual void onPaint() ;
		///< Called second for a window-expose event.

	virtual void onExpose( ::XExposeEvent & ) ;
		///< Called third for a window-expose event.

	virtual bool onCreate() ;
		///< Called first for a window-map event. Returns false to abort
		///< window creation.

	virtual void onShow() ;
		///< Called second for a window-map event.

	virtual void onMap( ::XMapEvent & ) ;
		///< Called third for a window-map event.

	virtual void onKeyPress( ::XKeyEvent & ) ;
		///< Called for a key-press event.

	virtual void onKeyRelease( ::XKeyEvent & ) ;
		///< Called for a key-release event.

	virtual void onUser() ;
		///< Called for a client-message event.

private:
	void operator=( const EventHandler & ) ;
} ;

#endif

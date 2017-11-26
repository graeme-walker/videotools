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
/// \file gxcapture.h
///

#ifndef GX_CAPTURE__H
#define GX_CAPTURE__H

#include "gdef.h"
#include "gxdef.h"

namespace GX
{
	class Window ;
	class Capture ;
}

/// \class GX::Capture
/// RAII class to capture Xlib mouse events.
/// See also GX::Window::capture().
/// 
class GX::Capture
{
public:
	~Capture() ;
		///< Destructor. Releases mouse capture.
	
private:
	friend class GX::Window ;
	Capture( ::Display * , ::Window ) ;
		///< Constructor. Starts mouse capture.

private:
	Capture( const Capture & ) ;
	void operator=( const Capture & ) ;

private:
	bool m_ok ;
	::Display * m_display ;
} ;

inline
GX::Capture::Capture( ::Display * display , ::Window window ) :
	m_ok(false) ,
	m_display(display)
{
	m_ok = GrabSuccess == ::XGrabPointer( display ,
		window , // grab_window
		True , // owner_events
		ButtonReleaseMask | PointerMotionMask , // event_mask
		GrabModeAsync , // pointer_mode
		GrabModeAsync , // keyboard_mode
		window , // confine_to
		None , // cursor
		CurrentTime ) ;
}

inline
GX::Capture::~Capture()
{
	if( m_ok )
		::XUngrabPointer( m_display , CurrentTime ) ;
}

#endif

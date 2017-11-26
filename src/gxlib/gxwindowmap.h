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
/// \file gxwindowmap.h
///

#ifndef GX_WINDOW_MAP_H
#define GX_WINDOW_MAP_H

#include "gdef.h"
#include "gxdef.h"
#include "gxwindow.h"
#include "gxerror.h"
#include <map>
#include <exception>

namespace GX
{
	class WindowMap ;
}

/// \class GX::WindowMap
/// A class that can locate a GX::Window object based on a Xlib window handle.
/// 
class GX::WindowMap
{
public:
	struct NotFound : public GX::Error /// Exception class for GX::WindowMap.
	{ 
		virtual const char * what() const g__noexcept override { return "window not found" ; }
	} ;

	WindowMap() ;
		///< Constructor.

	~WindowMap() ;
		///< Destructor.

	static WindowMap * instance() ;
		///< Singleton access. Returns nullptr if none.

	void add( GX::Window & w ) ;
		///< Adds a window.

	void remove( GX::Window & w ) ;
		///< Removes a window.

	GX::Window & find( ::Window w ) const ;
		///< Finds a window. Throws if not found.

private:
	WindowMap( const WindowMap & ) ;
	void operator=( const WindowMap & ) ;

private:
	typedef ::Window XlibWindow ;
	typedef std::map<XlibWindow,GX::Window*> Map ;
	Map m_map ;
	static WindowMap * m_this ;
} ;

#endif

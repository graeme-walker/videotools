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
/// \file gxvisual.h
///

#ifndef GX_VISUAL_H
#define GX_VISUAL_H

#include "gdef.h"
#include "gxdef.h"
#include <list>
#include <string>

namespace GX
{
	class Visual ;
	class Display ;
}

/// \class GX::Visual
/// An Xlib XVisual wrapper.
/// 
class GX::Visual
{
public:
	typedef ::XVisualInfo Info ;

	static std::list<Info> infoList( Display & ) ;
		///< Returns a list of supported "visuals" for
		///< the default screen.

	explicit Visual( Display & ) ;
		///< Constructor for the display's default visual.

	Visual( Display & , const Info & ) ;
		///< Constructor.

	~Visual() ;
		///< Destructor.

	::Visual * x() ;
		///< Returns the X object.

	::VisualID id() const ;
		///< Returns the id.

	int depth() const ;
		///< Returns the depth.

	int type() const ;
		///< Returns the type.

	std::string typeName() const ;
		///< Returns the type name.

private:
	void init( Display & , VisualID ) ;
	static void check( bool ) ;
	Visual( const Visual & ) ;
	void operator=( const Visual & ) ;

private:
	::XVisualInfo m_info ;
} ;

#endif

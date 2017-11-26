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
/// \file gxcontext.h
///

#ifndef GX_CONTEXT_H
#define GX_CONTEXT_H

#include "gdef.h"
#include "gxdef.h"

namespace GX
{
	class Context ;
	class Display ;
	class Drawable ;
}

/// \class GX::Context
/// An Xlib GC wrapper.
/// 
class GX::Context
{
public:
	Context( Display & , Drawable & ) ;
		///< Constructor for a new graphics context. The display reference 
		///< is kept.

	Context( Display & , ::GC gc ) ;
		///< Constructor for an existing graphics context. The display 
		///< reference is kept. The context is _not_ freed in the destructor.

	~Context() ;
		///< Destructor.

	void setForeground( unsigned long cc ) ;
		///< Sets the foreground drawing colour.

	void setLineWidth( unsigned long ) ;
		///< Sets the line width.

	::GC x() ;
		///< Returns the X object.

	Context( const Context & ) ;
		///< Copy constructor. The new object refers to the same underlying context.

private:
	void operator=( const Context & ) ;

private:
	Display & m_display ;
	::GC m_context ;
	bool m_owned ;
} ;

#endif

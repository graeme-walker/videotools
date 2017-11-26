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
/// \file gxdrawable.h
///

#ifndef GX_DRAWABLE_H
#define GX_DRAWABLE_H

#include "gdef.h"
#include "gxdef.h"

namespace GX
{
	class Drawable ;
	class Display ;
	class Context ;
}

/// \class GX::Drawable
/// An abstract base class for xserver-side drawables (windows, pixmaps, etc) 
/// with methods for drawing points, lines and blocks.
/// 
class GX::Drawable
{
public:
	explicit Drawable( Display & ) ;
		///< Constructor. The display reference is kept.

	virtual ~Drawable() ;
		///< Destructor.

	virtual ::Drawable xd() = 0 ;
		///< Returns the X object.

	void drawPoint( Context & , int x , int y ) ;
		///< Draws a point.

	void drawLine( Context & , int x1 , int y1 , int x2 , int y2 ) ;
		///< Draws a line.

	void drawRectangle( Context & , int x , int y , int dx , int dy ) ;
		///< Draws a rectangle.

	void drawArc( Context & , int x , int y , unsigned int width , 
		unsigned int height , int angle1 , int angle2 ) ;
			///< Draws an arc.

private:
	Drawable( const Drawable & ) ;
	void operator=( const Drawable & ) ;

private:
	Display & m_display ;
} ;

#endif

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
/// \file gxpixmap.h
///

#ifndef GX_PIXMAP_H
#define GX_PIXMAP_H

#include "gdef.h"
#include "gxdef.h"
#include "gxdrawable.h"
#include <memory>

namespace GX
{
	class Pixmap ;
	class Display ;
	class Image ;
	class Window ;
}

/// \class GX::Pixmap
/// A pixmap class for xserver-side images. Inherits line-drawing
/// from its Drawable base class.
/// 
class GX::Pixmap : public GX::Drawable
{
public:
	Pixmap( Display & , GX::Window & window , int dx = 0 , int dy = 0 ) ;
		///< Constructor. The display reference is kept.

	~Pixmap() ;
		///< Destructor.

	::Pixmap x() ;
		///< Returns the X object.

	virtual ::Drawable xd() ;
		///< From Drawable.

	void blit( GX::Window & , int src_x , int src_y , int dx , int dy , int dst_x , int dst_y ) ;
		///< Blits to a window. Uses the display's default 
		///< graphics context.

	int dx() const ;
		///< Returns the width.

	int dy() const ;
		///< Returns the height.

	void clear() ;
		///< Draws a big rectangle to clear the pixmap.

	void readImage( unique_ptr<Image> & ) const ;
		///< Reads the image from the xserver. (This is not in
		///< the base class because reading window images is
		///< less reliable wrt. occlusion etc.)

private:
	Pixmap( const Pixmap & ) ;
	void operator=( const Pixmap & ) ;
	void blit( GX::Window & , Context & , int src_x , int src_y , int dx , int dy , int dst_x , int dst_y ) ;

private:
	Display & m_display ;
	::Pixmap m_pixmap ;
	int m_dx ;
	int m_dy ;
} ;

#endif

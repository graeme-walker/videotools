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
/// \file gximage.h
///

#ifndef GX_IMAGE_H
#define GX_IMAGE_H

#include "gdef.h"
#include "gxdef.h"
#include "gxvisual.h"

namespace GX
{
	class Image ;
	class Display ;
	class Context ;
	class Visual ;
	class Window ;
}

/// \class GX::Image
/// A class for xclient-side images that are drawn locally and then blitted 
/// to the xserver.
/// 
class GX::Image
{
public:
	Image( Display & , const GX::Window & window , int dx = 0 , int dy = 0 ) ;
		///< Constructor. The display reference is kept.

	Image( Display & display , XImage * p , int dx , int dy ) ;
		///< A private constructor used by Pixmap::getImage().

	~Image() ;
		///< Destructor.

	::XImage * x() ;
		///< Returns the X object.

	void blit( GX::Window & , int src_x , int src_y , int dx , int dy , int dst_x , int dst_y ) ;
		///< Blits to a window. Uses the display's default 
		///< graphics context.

	int dx() const ;
		///< Returns the width.

	int dy() const ;
		///< Returns the height.

	void drawPoint( int x , int y , unsigned long p , bool or_ = false ) ;
		///< Draws a point. The pixel value typically comes from GX::ColourMap::get().

	bool fastable() const ;
		///< Returns true if drawFastPoint() can be used.

	void drawFastPoint( int x , int y , unsigned long p ) ;
		///< Draws a point fast.

	unsigned long readPoint( int x , int y ) const ;
		///< Reads a pixel value at a point.

	void drawLine( int x1 , int y1 , int x2 , int y2 , unsigned long p , bool or_ = false ) ;
		///< Draws a line. The pixel value typically comes from GX::ColourMap::get().

	void drawLineAcross( int x1 , int x2 , int y , unsigned long p ) ;
		///< Draws a horizontal line.

	void drawLineDown( int x , int y1 , int y2 , unsigned long p ) ;
		///< Draws a vertical line.

	void clear( bool white = false ) ;
		///< Clears the image.

private:
	Image( const Image & ) ;
	void operator=( const Image & ) ;
	void blit( GX::Window & , Context & , int src_x , int src_y , int dx , int dy , int dst_x , int dst_y ) ;

private:
	Display & m_display ;
	::XImage * m_image ;
	int m_dx ;
	int m_dy ;
} ;

inline
::XImage * GX::Image::x()
{
	return m_image ;
}

inline
void GX::Image::drawPoint( int x , int y , unsigned long p , bool or_ )
{
	if( x >= 0 && x < m_dx && y >= 0 && y < m_dy )
	{
		if( or_ )
			XPutPixel( m_image , x , y , p | XGetPixel(m_image,x,y) ) ;
		else
			XPutPixel( m_image , x , y , p ) ;
	}
}

inline
bool GX::Image::fastable() const
{
	return true ;
}

inline
void GX::Image::drawFastPoint( int x , int y , unsigned long p )
{
	XPutPixel( m_image , x , y , p ) ;
}

inline
int GX::Image::dx() const
{
	return m_dx ;
}

inline
int GX::Image::dy() const
{
	return m_dy ;
}

#endif

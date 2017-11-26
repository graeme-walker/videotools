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
/// \file gxcanvas.h
///

#ifndef GX_CANVAS_H
#define GX_CANVAS_H

#include "gdef.h"
#include "gxdef.h"
#include "gxcolourmap.h"
#include "gxwindow.h"
#include "gximage.h"
#include "gxpixmap.h"
#include "gxcontext.h"
#include "grcolour.h"
#include <string>

namespace GX
{
	class Canvas ;
	class CanvasImp ;
	class Window ;
}

/// \class GX::Canvas
/// A drawing surface that is embedded in a window.
/// 
/// Supports server-side and client-side implementations chosen at run-time.
/// Client-side uses a local image that is transferred to the X-server by the
/// blit() method; server-side uses a pixmap that lives on the X-server and 
/// blit() is a copy performed within the X-server. Client-side should be 
/// preferred for efficient reading of pixels and/or for dense images; 
/// server-side might be better for sparse images.
/// 
/// Optionally uses a palette for ancient display hardware; either 16-value
/// bios colours, or 256 greyscale shades. If using a palette then all
/// colours passed to point(), line() etc. should be from colour().
/// 
/// The point (0,0) is in the bottom left hand corner and (dx()-1,dy()-1) is 
/// the top right.
/// 
/// A stereo mode can be set so that colours are modified to red or green,
/// with (if client-side) yellow pixels on intersection.
/// 
class GX::Canvas
{
public:
	explicit Canvas( GX::Window & , int dx = 0 , int dy = 0 , int colours = 0 , bool server_side = false ) ;
		///< Constructor. By default the size of the canvas is the size 
		///< of the window. The colours parameter can be zero, 16 or 256.

	~Canvas() ;
		///< Destructor.

	int dx() const ;
		///< Returns the canvas width in pixels.

	int dy() const ;
		///< Returns the canvas height in pixels.

	std::pair<unsigned int,unsigned int> aspect() const ;
		///< Returns the canvas aspect ratio as a fraction (normally about 0.7).

	int colours() const ;
		///< Returns the number of colours, as passed in to the ctor.

	Gr::Colour colour( unsigned int n ) const ;
		///< Returns a colour from the palette.
		///< Precondition: n < colours

	Gr::Colour black() const ;
		///< Returns black.

	Gr::Colour white() const ;
		///< Returns white.

	void text( const std::string & s , int x , int y , Gr::Colour c ) ;
		///< Draws a single-line text string at the given position.

	void point( int x , int y , Gr::Colour c ) ;
		///< Draws a single pixel.

	bool fastable() const ;
		///< Returns true if fastpoint() can be used rather than point().

	void fastpoint( int x , int y , Gr::Colour c ) ;
		///< Draws a pixel quickly, assuming a client-side image and no stereo.
		///< Precondition: fastable()

	void line( int x0 , int y0 , int x1 , int y1 , Gr::Colour c ) ;
		///< Draws a line.

	void lineAcross( int y , int x_first , int x_last , const Gr::Colour & c ) ;
		///< Draws a horizontal line.

	void lineDown( int x , int y_first , int y_last , const Gr::Colour & c ) ;
		///< Draws a vertical line.

	Gr::Colour readPoint( int x , int y ) const ;
		///< Reads a pixel. This method might be very slow and inefficient for
		///< a server-side canvas. If using a palette then the palette index
		///< will be in the red component.

	void clear( bool white = false ) ;
		///< Clears the canvas to black (or white).

	void blit() ;
		///< Blits the canvas contents to the window.

	enum Stereo { left , right , normal } ;
	void stereo( Stereo mode = normal ) ;
		///< Sets the left/right stereo mode for subsequent line() and 
		///< point() drawing operations.

private:
	Canvas( const Canvas & ) ;
	void operator=( const Canvas & ) ;
	unsigned long pixel( const Gr::Colour & c_in ) const ;
	unsigned long pixel( const Gr::Colour & c_in , Stereo stereo_mode ) const ;

private:
	GX::Window & m_window ;
	int m_dx ;
	int m_dy ;
	int m_colours ; // 0=>rgb, 16=>biospalette, 256=>greyscale
	bool m_client_side ;
	unique_ptr<GX::ColourMap> m_colour_map ;
	unsigned int m_aspect_top ;
	unsigned int m_aspect_bottom ;
	GX::Pixmap m_pixmap ;
	GX::Image m_image ;
	GX::Context m_context ;
	Canvas::Stereo m_stereo_mode ;
} ;

inline
void GX::Canvas::fastpoint( int x , int y , Gr::Colour c )
{
	m_image.drawFastPoint( x , m_dy-1-y , c.cc() ) ;
}

#endif

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
/// \file gxdisplay.h
///

#ifndef GX_DISPLAY_H
#define GX_DISPLAY_H

#include "gdef.h"
#include "gxdef.h"
#include "gxcontext.h"
#include <list>

namespace GX
{
	class Display ;
	class Context ;
}

/// \class GX::Display
/// An Xlib Display wrapper.
/// 
class GX::Display
{
public:

	struct PixmapFormat /// A summary of one item in an Xlib XPixmapFormatValues list.
	{
		int depth ; 
		int bits_per_pixel ; 
		int scanline_pad ; 
	} ;

	Display() ;
		///< Constructor.

	~Display() ;
		///< Destructor.

	::Display * x() ;
		///< Returns the X object.

	int fd() const ;
		///< Returns a select()able file descriptor.

	int depth() const ;
		///< Returns the color depth (in bits).

	int white() const ;
		///< Returns the white colour value.

	int black() const ;
		///< Returns the black colour value.

	int dx() const ;
		///< Returns the default screen's width.

	int dy() const ;
		///< Returns the default screen's height.

	::Colormap defaultColourmap() ;
		///< Returns the default colour map of the default
		///< screen.

	Context defaultContext() ;
		///< Returns the default graphics context
		///< for the default screen.

	std::list<PixmapFormat> pixmapFormats() const ;
		///< Returns a list of supported pixmap formats.

private:
	Display( const Display & ) ;
	void operator=( const Display & ) ;

private:
	::Display * m_display ;
	int m_black ;
	int m_white ;
	int m_depth ;
	int m_dx ;
	int m_dy ;
} ;

#endif

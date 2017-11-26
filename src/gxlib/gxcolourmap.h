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
/// \file gxcolourmap.h
///

#ifndef GX_COLOURMAP_H
#define GX_COLOURMAP_H

#include "gdef.h"
#include "gxdef.h"
#include "gxdrawable.h"
#include <vector>

namespace GX
{
	class ColourMap ;
	class Display ;
	class Window ;
}

/// \class GX::ColourMap
/// A colourmap class that provides pixel values for a set of mapped colours. 
/// The map can be for 16 colours or 256 greys. The first colour (0) is black, 
/// and the last (15/255) is white.
/// 
class GX::ColourMap
{
public:
	explicit ColourMap( Display & , bool greyscale256 = false ) ;
		///< Constructor for a colourmap for the default visual
		///< providing 16 colours or 256 grey levels. The 
		///< implementation creates a temporary window.

	ColourMap( Display & , GX::Window & w , ::Visual * v ) ;
		///< Constructor for a colourmap for a specific visual. 
		///< The display reference is kept. The window is only 
		///< used to determine the screen.

	~ColourMap() ;
		///< Destructor.

	::Colormap x() ;
		///< Returns the X colourmap id. See XCreateColormap(3).

	unsigned long get( int index ) const ;
		///< Gets the pixel value for the given index value.

	unsigned int size() const ;
		///< Returns 16 for colours or 256 for greyscale.

	int find( unsigned long ) const ;
		///< Does a reverse lookup to return the index value
		///< for the given pixel value.

private:
	ColourMap( const ColourMap & ) ;
	void operator=( const ColourMap & ) ;
	void create( Display & , ::Window , ::Visual * v , bool ) ;
	void setGrey256() ;
	void setColour16() ;
	void addColour16(int,int,int) ;

private:
	Display & m_display ;
	::Colormap m_x ;
	std::vector<unsigned long> m_map ;
	unsigned int m_size ;
} ;

inline
unsigned long GX::ColourMap::get( int index ) const
{
	//index = index % m_size ;
	return m_map[static_cast<size_t>(index)] ;
}

#endif

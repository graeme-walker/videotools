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
/// \file grglyph.h
///

#ifndef GR_GLYPH__H
#define GR_GLYPH__H

#include "gdef.h"
#include <vector>
#include <string>

namespace Gr
{
	class Glyph ;
}

/// \class Gr::Glyph
/// A class for mapping characters to 8x8 glyphs.
/// 
class Gr::Glyph
{
public:
	typedef std::vector<bool> row_type ;
	typedef std::vector<row_type> matrix_type ;

	static matrix_type matrix( char c ) ;
		///< Returns a glyph matrix for the given character.

	template <typename Tout>
	static void output( const std::string & s , Tout & out_functor ) ;
		///< Calls an (x,y,bool) functor for all the glyph points corresponding 
		///< to the given line of text, with no support for line-wrapping. 
		///< 
		///< The functor call order is suitable for a raster scan, ie. the 
		///< x parameter varies fastest and traverses the whole string before 
		///< the y parameter is incremented. 
		///< 
		///< An 'off' pixel is added at the end of each characer raster scan 
		///< (including the last character), so there are nine horizontal 
		///< pixels per character and a total of eight vertical pixels.

private:
	Glyph() ;
	static bool point( char , unsigned int , unsigned int ) ;
	static unsigned int * row( char ) ;
} ;

namespace Gr
{
	template <typename Tout>
	void Glyph::output( const std::string & s , Tout & out )
	{
		for( unsigned int dy = 0U ; dy < 8U ; dy++ )
		{
			unsigned int n = 0U ;
			std::string::const_iterator const end = s.end() ;
			for( std::string::const_iterator p = s.begin() ; p != end ; ++p , n++ )
			{
				for( unsigned int dx = 0U ; dx < 8U ; dx++ )
					out( (n*9U) + dx , dy , point(*p,dx,dy) ) ;
				out( (n*9U) + 8U , dy , false ) ;
			}
		}
	} 
}

#endif

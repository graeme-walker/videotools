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
/// \file grcolour16.h
///

#ifndef GR_COLOUR16__H
#define GR_COLOUR16__H

#include "gdef.h"

namespace Gr
{
	namespace colour16
	{
		// generators for the standard sixteen-colour bios palette -- see wikipedia 'bios_color_attributes'
		//
		// 0 (0,0,0) black
		// 1 (0,0,1) dark blue
		// 2 (0,1,0) dark green
		// 3 (0,1,1) dark cyan
		// 4 (1,0,0) dark red
		// 5 (1,0,1) dark magenta
		// 6 (1,1,0) dark yellow (brown)
		// 7 (1,2,2) bright grey-ish
		// 8 (1,1,1) dark grey
		// 9 (0,0,2) bright blue
		// 10 (0,2,0) bright green
		// 11 (0,2,2) bright cyan
		// 12 (2,0,0) bright red
		// 13 (2,0,2) bright magenta
		// 14 (2,2,0) bright yellow
		// 15 (2,2,2) white
		//
        inline unsigned r( unsigned n ) { return (n==8U) ? 1U : ( (n&4U) ? ((n/8U)+1U) : 0U ) ; }
        inline unsigned g( unsigned n ) { return (n==7U) ? 2U : ( (n==8U) ? 1U : ( (n&2U) ? ((n/8U)+1U) : 0U ) ) ; }
        inline unsigned b( unsigned n ) { return ((n&1U) << ((n>8U)?1:0) ) + (n==7U||n==8U) ; }
		inline unsigned _256( unsigned n ) { return n == 0U ? 0U : ( n == 1U ? 127U : 255U ) ; }
        inline unsigned r256( unsigned n ) { return _256(r(n)) ; }
        inline unsigned g256( unsigned n ) { return _256(g(n)) ; }
        inline unsigned b256( unsigned n ) { return _256(b(n)) ; }
	} ;
}

#endif

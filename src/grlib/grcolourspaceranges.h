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
/// \file grcolourspaceranges.h
///

#ifndef GR_COLOURSPACE_RANGES__H
#define GR_COLOURSPACE_RANGES__H

#include "gdef.h"

namespace Gr
{
namespace ColourSpace
{

/// \class Gr::ColourSpace::range_y
/// Defines the "digital" range for y component. This is [16..235],
/// so having a size of 219.
/// 
struct range_y
{
	typedef unsigned char value_type ;
	static constexpr value_type min = 16 ; // black
	static constexpr value_type max = 235 ; // white
	static constexpr bool is_signed = 0 ;
} ;

/// \class Gr::ColourSpace::range_uv
/// Defines the "digital" range for u/v components. This is [16..240],
/// so having a size of 224.
/// 
struct range_uv
{
	typedef unsigned char value_type ;
	static constexpr value_type min = 16 ;
	static constexpr value_type max = 240 ;
	static constexpr bool is_signed = 1 ;
} ;

/// \class Gr::ColourSpace::range_rgb
/// Defines the "digital" range for r/g/b components.
/// This is [0..255], so having a size of 255.
/// 
/// range for rgb components (0..255)
struct range_rgb
{
	typedef unsigned char value_type ;
	static constexpr value_type min = 0 ;
	static constexpr value_type max = 255 ;
	static constexpr bool is_signed = 0 ;
} ;

}
}

#endif

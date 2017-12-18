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
/// \file grcolourspacematrix.h
///
/// This file has been machine generated -- do not edit
/// 

#ifndef GR_COLOURSPACE_MATRIX__H
#define GR_COLOURSPACE_MATRIX__H

#include "gdef.h"

namespace Gr
{
namespace ColourSpace
{

/// \class Gr::ColourSpace::matrix_256_inverse
/// Rgb-to-yuv matrix using scaled integers where 256 is 1.
/// 
struct matrix_256_inverse
{
	static g__constexpr int scale = 256 ;

	static g__constexpr int ry = 256 ;
	static g__constexpr int ru = 0 ;
	static g__constexpr int rv = 359 ; // 2*(kg+kb)

	static g__constexpr int gy = 256 ;
	static g__constexpr int gu = -88 ; // -2*(kr+kg)*kb/kg
	static g__constexpr int gv = -183 ; // -2*(kg+kb)*kr/kg

	static g__constexpr int by = 256 ;
	static g__constexpr int bu = 454 ; // 2*(kr+kg)
	static g__constexpr int bv = 0 ;
} ;

/// \class Gr::ColourSpace::matrix_256
/// Yuv-to-rgb matrix using scaled integers where 256 is 1.
/// 
struct matrix_256
{
	typedef matrix_256_inverse inverse_type ;
	static g__constexpr int scale = 256 ;

	static g__constexpr int yr = 77 ; // kr
	static g__constexpr int yg = 150 ; // kg=1-kr-kb
	static g__constexpr int yb = 29 ; // kb=1-kr-kg

	static g__constexpr int ur = -43 ; // -kr/2(1-kb)
	static g__constexpr int ug = -85 ; // -kg/2(1-kb)
	static g__constexpr int ub = 128 ;

	static g__constexpr int vr = 128 ;
	static g__constexpr int vg = -107 ; // -kg/2(1-kr)
	static g__constexpr int vb = -21 ; // -kb/2(1-kr)
} ;

/// \class Gr::ColourSpace::matrix_1000_inverse
/// Rgb-to-yuv matrix using scaled integers where 1000 is 1.
/// 
struct matrix_1000_inverse
{
	static g__constexpr int scale = 1000 ;

	static g__constexpr int ry = 1000 ;
	static g__constexpr int ru = 0 ;
	static g__constexpr int rv = 1402 ; // 2*(kg+kb)

	static g__constexpr int gy = 1000 ;
	static g__constexpr int gu = -344 ; // -2*(kr+kg)*kb/kg
	static g__constexpr int gv = -714 ; // -2*(kg+kb)*kr/kg

	static g__constexpr int by = 1000 ;
	static g__constexpr int bu = 1772 ; // 2*(kr+kg)
	static g__constexpr int bv = 0 ;
} ;

/// \class Gr::ColourSpace::matrix_1000
/// Yuv-to-rgb matrix using scaled integers where 1000 is 1.
/// 
struct matrix_1000
{
	typedef matrix_1000_inverse inverse_type ;
	static g__constexpr int scale = 1000 ;

	static g__constexpr int yr = 299 ; // kr
	static g__constexpr int yg = 587 ; // kg=1-kr-kb
	static g__constexpr int yb = 114 ; // kb=1-kr-kg

	static g__constexpr int ur = -169 ; // -kr/2(1-kb)
	static g__constexpr int ug = -331 ; // -kg/2(1-kb)
	static g__constexpr int ub = 500 ;

	static g__constexpr int vr = 500 ;
	static g__constexpr int vg = -419 ; // -kg/2(1-kr)
	static g__constexpr int vb = -81 ; // -kb/2(1-kr)
} ;

}
}

#endif

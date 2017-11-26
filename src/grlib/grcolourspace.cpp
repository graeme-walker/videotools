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
//
// grcolourspace.cpp
//

#include "gdef.h"
#include "grcolourspace.h"

static unsigned int to_int( double d ) ;

// these are transliterations of the colourspace standards docs used
// to check the c++ implementation -- do not "improve" them without
// reference to the original specifications

unsigned int g_colourspace_y( unsigned int r , unsigned int g , unsigned int b )
{
	return to_int( 16. +
		((double)r)/255.*219.*0.299 + // 65.738
		((double)g)/255.*219.*0.587 + // 129.057
		((double)b)/255.*219.*0.114 ) ; // 25.064
}

unsigned int g_colourspace_u( unsigned int r , unsigned int g , unsigned int b )
{
	return to_int( 128. +
		-((double)r)/255.*224.*0.169 // 37.945
		-((double)g)/255.*224.*0.331 + // 74.494
		((double)b)/255.*224.*0.5 ) ; // 112.439
}

unsigned int g_colourspace_v( unsigned int r , unsigned int g , unsigned int b )
{
	return to_int( 128. +
		((double)r)/255.*224.*0.5 + // 112.439
		-((double)g)/255.*224.*0.419 + // 94.154
		-((double)b)/255.*224.*0.081 ) ; // 18.285
}

unsigned int g_colourspace_r( unsigned int y , unsigned int , unsigned int v )
{
	return to_int(
		((double)(y)-16.)*255./219. +
		((double)(v)-128.)*255./112.*0.701 ) ;
}

unsigned int g_colourspace_g( unsigned int y , unsigned int u , unsigned int v )
{
	return to_int(
		((double)(y)-16.)*255./219. +
		-((double)(u)-128.)*255./112.*0.886*0.114/0.587 +
		-((double)(v)-128.)*255./112.*0.701*0.299/0.587 ) ;
}

unsigned int g_colourspace_b( unsigned int y , unsigned int u , unsigned int )
{
	return to_int(
		((double)(y)-16.)*255./219. +
		((double)(u)-128.)*255./112.*0.886 ) ;
}

static unsigned int to_int( double d )
{
	int i = (int)(d+0.5) ; // moot rounding
	return i < 0 ? 0U : ( i > 255 ? 255U : (unsigned int)i ) ;
}

/// \file grcolourspace.cpp

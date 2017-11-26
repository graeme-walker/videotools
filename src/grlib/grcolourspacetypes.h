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
/// \file grcolourspacetypes.h
///

#ifndef GR_COLOURSPACE_TYPES__H
#define GR_COLOURSPACE_TYPES__H

#include "gdef.h"

namespace Gr
{
namespace ColourSpace
{

/// \class Gr::ColourSpace::fp_1000
/// A simple fixed-point number class with a scale factor of 1000.
/// 
struct fp_1000
{
	typedef int int_t ;
	static constexpr int scale__ = 1000 ;
	explicit fp_1000( int i ) g__noexcept : m_i(int_t(i)*scale__) {}
	fp_1000( int i_scaled , int /*private*/ ) g__noexcept : m_i(i_scaled) {}
	static fp_1000 value__( int i_scaled ) g__noexcept { return fp_1000(i_scaled,0) ; }
	int to_builtin() const g__noexcept { return int( m_i / scale__ ) ; }
	int_t m_i ;
} ;
inline fp_1000 operator*( const fp_1000 & a , const fp_1000 & b ) g__noexcept
{
	return fp_1000::value__( ( a.m_i * b.m_i ) / fp_1000::scale__ ) ;
}
inline fp_1000 operator/( const fp_1000 & a , const fp_1000 & b ) g__noexcept
{
	return fp_1000::value__( ( a.m_i * fp_1000::scale__ ) / b.m_i ) ;
}
inline fp_1000 operator+( const fp_1000 & a , const fp_1000 & b ) g__noexcept
{
	return fp_1000::value__( a.m_i + b.m_i ) ;
}
inline fp_1000 operator-( const fp_1000 & a , const fp_1000 & b ) g__noexcept
{
	return fp_1000::value__( a.m_i - b.m_i ) ;
}
inline bool operator<( const fp_1000 & a , const fp_1000 & b ) g__noexcept
{
	return a.m_i < b.m_i ;
}
inline bool operator>( const fp_1000 & a , const fp_1000 & b ) g__noexcept
{
	return a.m_i > b.m_i ;
}


/// \class Gr::ColourSpace::fp_256
/// A simple fixed-point number class with a scale factor of 256
/// 
struct fp_256
{
	typedef int int_t ;
	static constexpr int scale__ = 256 ;
	explicit fp_256( int i ) g__noexcept : m_i(i>0?(int_t(i)<<8):-(int_t(-i)<<8)) {}
	fp_256( int i_scaled , int /*private*/ ) g__noexcept : m_i(i_scaled) {}
	static fp_256 value__( int i_scaled ) g__noexcept { return fp_256(i_scaled,0) ; }
	int to_builtin() const g__noexcept { return int(m_i>0?(m_i>>8):-((-m_i)>>8)) ; }
	int_t m_i ;
} ;
inline fp_256 operator*( fp_256 a , fp_256 b ) g__noexcept
{
	return fp_256::value__( ( a.m_i * b.m_i ) >> 8 ) ;
}
inline fp_256 operator/( fp_256 a , fp_256 b ) g__noexcept
{
	return fp_256::value__( ( a.m_i << 8 ) / b.m_i ) ;
}
inline fp_256 operator+( fp_256 a , fp_256 b ) g__noexcept
{
	return fp_256::value__( a.m_i + b.m_i ) ;
}
inline fp_256 operator-( fp_256 a , fp_256 b ) g__noexcept
{
	return fp_256::value__( a.m_i - b.m_i ) ;
}
inline bool operator<( fp_256 a , fp_256 b ) g__noexcept
{
	return a.m_i < b.m_i ;
}
inline bool operator>( fp_256 a , fp_256 b ) g__noexcept
{
	return a.m_i > b.m_i ;
}

/// \class Gr::ColourSpace::converter_double
/// An adaptor for double.
/// 
struct converter_double
{
	typedef double fp_type ;
	static double make_fp( int i , int s ) g__noexcept
	{
		return double(i) / double(s) ;
	}
	static double make_half() g__noexcept
	{
		return 0.5 ;
	}
	static unsigned char to_range( double d ) g__noexcept
	{
		return static_cast<unsigned char>(d) ;
	}
} ;

/// \class Gr::ColourSpace::converter_fp
/// An adaptor for fp_#.
/// 
template <typename T>
struct converter_fp
{
	typedef T fp_type ;
	static fp_type make_fp( int i , int ) g__noexcept
	{
		return fp_type::value__( i ) ; // assume scale factors match
	}
	static constexpr fp_type make_half() g__noexcept
	{
		return fp_type::value__( fp_type::scale__ >> 1 ) ;
	}
	static unsigned char to_range( fp_type n ) g__noexcept
	{
		return static_cast<unsigned char>( n.to_builtin() ) ;
	}
} ;

}
}

#endif

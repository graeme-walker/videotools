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
/// \file grcolourspace.h
///

#ifndef G_COLOURSPACE__H
#define G_COLOURSPACE__H

#include "gdef.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
unsigned int g_colourspace_r( unsigned int y , unsigned int u , unsigned int v ) ;
unsigned int g_colourspace_g( unsigned int y , unsigned int u , unsigned int v ) ;
unsigned int g_colourspace_b( unsigned int y , unsigned int u , unsigned int v ) ;
unsigned int g_colourspace_y( unsigned int r , unsigned int g , unsigned int b ) ;
unsigned int g_colourspace_u( unsigned int r , unsigned int g , unsigned int b ) ;
unsigned int g_colourspace_v( unsigned int r , unsigned int g , unsigned int b ) ;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace Gr
{
	/// \namespace Gr::ColourSpace
	/// Provides rgb/yuv colourspace mapping functions.
	/// 
	/// Synopsis.
	/// \code
	///
	///   typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
	///   triple_t rgb = Gr::ColourSpace::rgb_int( triple_t(y,u,v) ) ;
	///   triple_t yuv = Gr::ColourSpace::yuv_int( triple_t(r,g,b) ) ;
	///   triple_t rgb = Gr::ColourSpace::rgb( triple_t(y,u,v) ) ;
	///   triple_t yuv = Gr::ColourSpace::yuv( triple_t(r,g,b) ) ;
	///
	/// \endcode
	/// Most code will be content to use just the two lookup functions rgb_int()
	/// and yuv_int(), and in that case the bulk of the code here is only used 
	/// at built-time to generate the lookup tables that underlie those functions.
	/// 
	/// Otherwise, there are two top-level functions rgb_imp() and yuv_imp()
	/// with lots of template parameters providing maximum flexibility, and
	/// two convenience functions rgb() and yuv() that use rgb_imp()
	/// and yuv_imp() with sensible template defaults.
	/// 
	/// The implementation recognises that the core matrix multiplication
	/// has to be performed with a higher-precision numeric datatype than
	/// is used at the interface, and that the values used at the interface
	/// are sometimes constrained to a range that is not a convenient 
	/// power of two (ie. head-room and toe-room for YUV values). The 
	/// internal datatype can be considered to be "analogue" and the 
	/// external datatype "digital", alluding to their different levels 
	/// of precision.
	/// 
	/// The internal datatype is most naturally "double", but for speed on
	/// some machines it can be better to used a fixed-point type. Two 
	/// fixed-point classes are provided, "fp_256" and "fp_1000", together
	/// with adaptors that are used to convert between internal and external 
	/// values. If any internal datatype other than "double", "fp_256" or
	/// "fp_1000" is used then it will need to have a corresponding 
	/// adaptor class (as per "converter_double").
	/// 
	namespace ColourSpace
	{
		template <typename T> struct triple ;
	}
}

/// \class Gr::ColourSpace::triple
/// A 3-tuple for holding rgb or yuv triples.
/// 
template <typename T>
struct Gr::ColourSpace::triple
{
	typedef T value_type ;
	T m_1 ;
	T m_2 ;
	T m_3 ;
	triple( T a , T b , T c ) : m_1(a) , m_2(b) , m_3(c) {}
	T r() const g__noexcept { return m_1 ; }
	T g() const g__noexcept { return m_2 ; }
	T b() const g__noexcept { return m_3 ; }
	T y() const g__noexcept { return m_1 ; }
	T u() const g__noexcept { return m_2 ; }
	T v() const g__noexcept { return m_3 ; }
	T operator[]( int c ) const { return c == 0 ? m_1 : ( c == 1 ? m_2 : m_3 ) ; }
} ;

#include "grcolourspacematrix.h"
#include "grcolourspacetypes.h"
#include "grcolourspaceranges.h"
#include "grcolourspacemap.h"
#include "grcolourspacetables.h"

namespace Gr
{
namespace ColourSpace
{

/// Does a cross-product using fp types.
/// 
template <typename Tmatrix, typename Tconverter, typename Ta, typename Ttriple>
constexpr
typename Tconverter::fp_type product( Ta a1 , Ta a2 , Ta a3 , Ttriple tfp ) g__noexcept
{
	return 
		tfp.m_1 * Tconverter::make_fp(a1,Tmatrix::scale) +
		tfp.m_2 * Tconverter::make_fp(a2,Tmatrix::scale) +
		tfp.m_3 * Tconverter::make_fp(a3,Tmatrix::scale) ;
}

/// Calculates y from rgb using fp types.
/// Uses the matrix given by the template parameter.
/// 
template <typename Tmatrix, typename Tconverter, typename Ttriple>
constexpr
typename Tconverter::fp_type y_imp( Ttriple rgb ) g__noexcept
{
	return product<Tmatrix,Tconverter>( Tmatrix::yr , Tmatrix::yg , Tmatrix::yb , rgb ) ;
}

/// Calculates u (cb) from rgb using fp types.
/// Uses the matrix given by the template parameter.
/// 
template <typename Tmatrix, typename Tconverter, typename Ttriple>
constexpr
typename Tconverter::fp_type u_imp( Ttriple rgb ) g__noexcept
{
	return product<Tmatrix,Tconverter>( Tmatrix::ur , Tmatrix::ug , Tmatrix::ub , rgb ) ;
}

/// Calculates v (cr) from rgb using fp types.
/// Uses the matrix given by the template parameter.
/// 
template <typename Tmatrix, typename Tconverter, typename Ttriple>
constexpr
typename Tconverter::fp_type v_imp( Ttriple rgb ) g__noexcept
{
	return product<Tmatrix,Tconverter>( Tmatrix::vr , Tmatrix::vg , Tmatrix::vb , rgb ) ;
}

/// Calculates r from yuv using fp types.
/// Uses the matrix given by the template parameter.
/// 
template <typename Tmatrix, typename Tconverter, typename Ttriple>
constexpr
typename Tconverter::fp_type r_imp( Ttriple yuv ) g__noexcept
{
	return product<Tmatrix,Tconverter>( Tmatrix::ry , Tmatrix::ru , Tmatrix::rv , yuv ) ;
}

/// Calculates g from yuv using fp types.
/// Uses the matrix given by the template parameter.
/// 
template <typename Tmatrix, typename Tconverter, typename Ttriple>
constexpr
typename Tconverter::fp_type g_imp( Ttriple yuv ) g__noexcept
{
	return product<Tmatrix,Tconverter>( Tmatrix::gy , Tmatrix::gu , Tmatrix::gv , yuv ) ;
}

/// Calculates b from yuv using fp types.
/// Uses the matrix given by the template parameter.
/// 
template <typename Tmatrix, typename Tconverter, typename Ttriple>
constexpr
typename Tconverter::fp_type b_imp( Ttriple yuv ) g__noexcept
{
	return product<Tmatrix,Tconverter>( Tmatrix::by , Tmatrix::bu , Tmatrix::bv , yuv ) ;
}

/// A top-level function that calculates yuv from rgb
/// with all the implementation options exposed as template parameters.
/// 
template <typename Tmatrix, typename Tconverter,
	typename Trange_rgb , typename Trange_y, typename Trange_uv,
	typename Ttriple>
Ttriple yuv_imp( Ttriple rgb ) g__noexcept
{
	return Ttriple( 
		map_to_range<Trange_y,Tconverter> ( y_imp<Tmatrix,Tconverter>( map_to_space<Tconverter,Trange_rgb,Trange_rgb,Trange_rgb>(rgb) ) ) ,
		map_to_range<Trange_uv,Tconverter>( u_imp<Tmatrix,Tconverter>( map_to_space<Tconverter,Trange_rgb,Trange_rgb,Trange_rgb>(rgb) ) ) ,
		map_to_range<Trange_uv,Tconverter>( v_imp<Tmatrix,Tconverter>( map_to_space<Tconverter,Trange_rgb,Trange_rgb,Trange_rgb>(rgb) ) ) ) ;
}

/// A top-level function that calculates rgb from yuv
/// with all the implementation options exposed as template parameters.
/// 
template <typename Tmatrix, typename Tconverter,
	typename Trange_rgb , typename Trange_y, typename Trange_uv,
	typename Ttriple>
Ttriple rgb_imp( Ttriple yuv ) g__noexcept
{
	typedef typename Tmatrix::inverse_type matrix_type ;
	return Ttriple( 
		map_to_range<Trange_rgb,Tconverter>( r_imp<matrix_type,Tconverter>( map_to_space<Tconverter,Trange_y,Trange_uv,Trange_uv>(yuv) ) ) ,
		map_to_range<Trange_rgb,Tconverter>( g_imp<matrix_type,Tconverter>( map_to_space<Tconverter,Trange_y,Trange_uv,Trange_uv>(yuv) ) ) ,
		map_to_range<Trange_rgb,Tconverter>( b_imp<matrix_type,Tconverter>( map_to_space<Tconverter,Trange_y,Trange_uv,Trange_uv>(yuv) ) ) ) ;
}

/// A top-level function that calculates yuv from rgb
/// with default implementation options.
/// 
inline triple<unsigned char> yuv( triple<unsigned char> rgb ) g__noexcept
{
	return yuv_imp<matrix_256,converter_fp<fp_256>,range_rgb,range_y,range_uv>( rgb ) ;
}

/// A top-level function that calculates rgb from yuv
/// with default implementation options.
/// 
inline triple<unsigned char> rgb( triple<unsigned char> yuv ) g__noexcept
{
	return rgb_imp<matrix_256,converter_fp<fp_256>,range_rgb,range_y,range_uv>( yuv ) ;
}

/// A fast conversion from yuv to r.
/// 
inline unsigned char r_int( unsigned char y , unsigned char /*u*/ , unsigned char v ) g__noexcept
{
	const int r = table_r_from_y[y] + table_r_from_v[v] ;
	return clamp<range_rgb,unsigned char>(r) ;
}

/// A fast conversion from yuv to g.
/// 
inline unsigned char g_int( unsigned char y , unsigned char u , unsigned char v ) g__noexcept
{
	const int g = table_g_from_y[y] + table_g_from_u[u] + table_g_from_v[v] ;
	return clamp<range_rgb,unsigned char>(g) ;
}

/// A fast conversion from yuv to b.
/// 
inline unsigned char b_int( unsigned char y , unsigned char u , unsigned char /*v*/ ) g__noexcept
{
	const int b = table_b_from_y[y] + table_b_from_u[u] ;
	return clamp<range_rgb,unsigned char>(b) ;
}

/// A fast conversion from yuv to rgb.
/// 
inline triple<unsigned char> rgb_int( triple<unsigned char> yuv ) g__noexcept
{
	return triple<unsigned char>( r_int(yuv.y(),yuv.u(),yuv.v()) , g_int(yuv.y(),yuv.u(),yuv.v()) , b_int(yuv.y(),yuv.u(),yuv.v()) ) ;
}

/// A fast conversion from rgb to y.
/// 
inline unsigned char y_int( unsigned char r , unsigned char g , unsigned char b )
{
	const int y = table_y_from_r[r] + table_y_from_g[g] + table_y_from_b[b] ;
	return clamp<range_y,unsigned char>( y + range_y::min ) ;
}

/// A fast conversion from rgb to u.
/// 
inline unsigned char u_int( unsigned char r , unsigned char g , unsigned char b )
{
	static constexpr int bias = ( static_cast<int>(range_uv::max) + static_cast<int>(range_uv::min) ) / 2 ;
	const int u = table_u_from_r[r] + table_u_from_g[g] + table_u_from_b[b] ;
	return clamp<range_uv,unsigned char>( u + bias ) ;
}

/// A fast conversion from rgb to v.
/// 
inline unsigned char v_int( unsigned char r , unsigned char g , unsigned char b )
{
	static constexpr int bias = ( static_cast<int>(range_uv::max) + static_cast<int>(range_uv::min) ) / 2 ;
	const int v = table_v_from_r[r] + table_v_from_g[g] + table_v_from_b[b] ;
	return clamp<range_uv,unsigned char>( v + bias ) ;
}

/// A fast conversion from rgb to yuv.
/// 
inline triple<unsigned char> yuv_int( triple<unsigned char> rgb ) g__noexcept
{
	return triple<unsigned char>( y_int(rgb.r(),rgb.g(),rgb.b()) , u_int(rgb.r(),rgb.g(),rgb.b()) , v_int(rgb.r(),rgb.g(),rgb.b()) ) ;
}


/// \class Gr::ColourSpace::cache
/// A cache for a complete set of rgb/yuv mappings, amounting
/// to about 50MB of data.
/// Eg:
/// \code
///   Gr::ColourSpace::cache<unsigned char> cache( Gr::ColourSpace::rgb ) ;
///   unsigned int rgb = cache.value<unsigned int>( y , u , v ) ;
/// \endcode
/// 
template <typename T>
class cache
{
public:
	typedef triple<T> triple_t ;
	typedef std::vector<triple_t> table_t ;
	table_t table ;
	cache() {}
	cache( triple_t (*fn)(triple_t) ) : 
		table(256*256*256,triple_t(0,0,0))
	{
		fill( fn ) ;
	}
	void fill( triple_t (*fn)(triple_t) )
	{
		if( table.empty() ) table.resize( 256*256*256 , triple_t(0,0,0) ) ;
		typename table_t::iterator p = table.begin() ;
		for( unsigned int a = 0 ; a <= 255 ; ++a )
			for( unsigned int b = 0 ; b <= 255 ; ++b )
				for( unsigned int c = 0 ; c <= 255 ; ++c )
					*p++ = fn( triple_t(a,b,c) ) ;
	}
	triple_t operator()( triple_t abc ) const
	{
		typedef g_uint32_t index_t ;
		return table[
			(static_cast<index_t>(abc.m_1)<<16) |
			(static_cast<index_t>(abc.m_2)<<8) |
			(static_cast<index_t>(abc.m_3)) ] ;
	}
	template <typename Tout>
	Tout value( T a , T b , T c ) const
	{
		typedef g_uint32_t index_t ;
		const index_t index = 
			(static_cast<index_t>(a)<<16) |
			(static_cast<index_t>(b)<<8) |
			(static_cast<index_t>(c)) ;
		return
			(static_cast<Tout>(table[index].m_1)<<16) |
			(static_cast<Tout>(table[index].m_2)<<8) |
			(static_cast<Tout>(table[index].m_3)) ;
	}
} ;

}
}

#endif

#endif

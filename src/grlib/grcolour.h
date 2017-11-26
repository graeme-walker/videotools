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
/// \file grcolour.h
///

#ifndef GR_COLOUR__H
#define GR_COLOUR__H

#include "gdef.h"
#include <utility> // std::swap
#include <iostream>

namespace Gr
{
	class Colour ;
}

/// \class Gr::Colour
/// A simple rgb colour structure.
/// 
class Gr::Colour
{
public:
	typedef unsigned char value_t ;
	typedef g_uint32_t cc_t ;

	Colour() ;
		///< Default constructor for black.

	Colour( value_t r , value_t g , value_t b ) ;
		///< Constructor taking red, green and blue values.

	value_t r() const ;
		///< Returns the red value.

	value_t g() const ;
		///< Returns the green value.

	value_t b() const ;
		///< Returns the blue value.

	cc_t cc() const ;
		///< Returns a combined-component value that incorporates
		///< the r(), g() and b() values. This is what Xlib uses 
		///< for a pixel value.

	static Colour from( cc_t cc ) ;
		///< Creates a colour from a combined-component value.

public:
	value_t m_r ;
	value_t m_g ;
	value_t m_b ;
} ;

inline
Gr::Colour::Colour() :
	m_r(0U) , 
	m_g(0U) , 
	m_b(0U)
{
}

inline
Gr::Colour::Colour( value_t r_ , value_t g_ , value_t b_ ) :
	m_r(r_) , 
	m_g(g_) , 
	m_b(b_)
{
}

inline
Gr::Colour::value_t Gr::Colour::r() const
{
	return m_r ;
}

inline
Gr::Colour::value_t Gr::Colour::g() const
{
	return m_g ;
}

inline
Gr::Colour::value_t Gr::Colour::b() const
{
	return m_b ;
}

inline
Gr::Colour::cc_t Gr::Colour::cc() const
{
	const cc_t r = m_r ;
	const cc_t g = m_g ;
	const cc_t b = m_b ;
	return (r<<16) | (g<<8) | (b<<0) ;
}

inline
Gr::Colour Gr::Colour::from( cc_t cc )
{
	return Colour( (cc>>16) & 255U , (cc>>8) & 255U , (cc>>0) & 255U ) ;
}

namespace Gr
{
	inline bool operator==( const Colour & p , const Colour & q )
	{
		return p.m_r == q.m_r && p.m_g == q.m_g && p.m_b == q.m_b ;
	}
	inline bool operator!=( const Colour & p , const Colour & q )
	{
		return !(p==q) ;
	}
	inline void swap( Colour & p , Colour & q )
	{
		std::swap( p.m_r , q.m_r ) ;
		std::swap( p.m_g , q.m_g ) ;
		std::swap( p.m_b , q.m_b ) ;
	}
	inline std::ostream & operator<<( std::ostream & stream , const Colour & colour )
	{
		stream 
			<< "{Colour=[" 
			<< static_cast<unsigned int>(colour.r()) << "," 
			<< static_cast<unsigned int>(colour.g()) << "," 
			<< static_cast<unsigned int>(colour.b()) << "]}" ;
		return stream ;
	}
}

#endif

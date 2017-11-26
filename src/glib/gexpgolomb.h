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
/// \file gexpgolomb.h
///

#ifndef G_EXP_GOLOMB__H
#define G_EXP_GOLOMB__H

#include "gdef.h"
#include "gbititerator.h"
#include "gexception.h"
#include <string>
#include <stdexcept>
#include <limits>

namespace G
{
	class ExpGolomb ;
}

/// \class G::ExpGolomb
/// Does exp-golomb decoding, as used in H.264 "ue(v)" syntax elements. The 
/// encoding is bit-oriented, so byte boundaries have no significance. The 
/// number of leading zero bits indicates the number of relevant trailing bits 
/// following the first '1' (eg. 0 0 0 0 1 x x x x), starting with zero encoded 
/// as '1' on its own, so 010 maps to 1, 011 to 2, 00100 to 3, 00101 to 4, 
/// 00110 to 5, 00111 to 6 etc.
/// 
/// \see H.264 spec (ISO/IEC 14496 part 10) section 9.1
/// 
class G::ExpGolomb
{
public:
	G_EXCEPTION( Error , "exp-golomb bitstream underflow" ) ;
	template <typename V> struct value /// Syntactic sugar for extracting ExpGolomb-encoded values.
		{ V n ; } ;

	template <typename U, typename T>
	static U decode( T & begin , T end , U underflow_value ) ;
		///< Decodes the bitstream data provided by the bit iterator. 
		///< Returns the underflow value on underflow.

	template <typename U, typename T>
	static U decode( T & begin , T end , bool * underflow_p ) ;
		///< Overload that sets a flag on underflow.

	template <typename U,typename T>
	static U decode( T & begin , T end ) ;
		///< Overload that throws on underflow.

	template <typename S, typename U>
	static S make_signed( U ) ;
		///< Makes a signed value from the result of decode()
		///< so 0 maps to 0, 1 maps to 1, 2 maps to -1, 
		///< 3 maps to 2, 4 maps to -2 etc.

private:
	template <typename U,typename T>
	static U decode_imp( T & begin , T end , bool * , U ) ;
} ;

template <typename U,typename T>
inline
U G::ExpGolomb::decode_imp( T & p , T end , bool * underflow_p , U underflow_value )
{
	U result = 0U ;
	int scale = 0 ;
	bool counting = true ;
	while( p != end )
	{
		int b = *p++ ;
		if( counting && b == 0 )
			scale++ ;
		else if( counting )
			result = 1 , counting = false ;
		else if( b == 0 )
			scale-- , result <<= 1 ;
		else
			scale-- , result <<= 1 , result++ ;

		if( !counting && scale == 0 )
			return result-1U ;
	}
	if( underflow_p ) *underflow_p = true ;
	return underflow_value ;
}

template <typename U,typename T>
inline
U G::ExpGolomb::decode( T & p , T end , U underflow_value )
{
	return decode_imp<U,T>( p , end , nullptr , underflow_value ) ;
}

template <typename U,typename T>
inline
U G::ExpGolomb::decode( T & p , T end , bool * underflow_p )
{
	return decode_imp<U,T>( p , end , underflow_p , 0 ) ;
}

template <typename U,typename T>
inline
U G::ExpGolomb::decode( T & p , T end )
{
	bool underflow = false ;
	U result = decode_imp<U,T>( p , end , &underflow , 0 ) ;
	if( underflow ) throw Error() ;
	return result ;
}

template <typename S, typename U>
inline
S G::ExpGolomb::make_signed( U u )
{
	return u == 0U ? S(0) : ( (u & 1U) ? S((u+1U)/2U) : -S(u/2U) ) ;
}

#endif

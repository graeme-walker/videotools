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
/// \file gbitstream.h
///

#ifndef G_BIT_STREAM__H
#define G_BIT_STREAM__H

#include "gdef.h"
#include "gbititerator.h"
#include "gexpgolomb.h"
#include <stdexcept>

namespace G
{

/// \class G::bit_stream
/// A class for pulling integer values of various widths out of a bit stream. 
/// Also supports exp-golomb encoding.
/// 
/// The template parameter must be G::bit_iterator<X>, for some underlying 
/// byte-wise forward iterator type X.
/// 
/// Eg:
/// \code
/// typedef G::ExpGolomb::value<unsigned> uev_t ;
/// typedef G::ExpGolomb::value<signed> sev_t ;
/// bit_stream<bit_iterator<char*>> bs( data.begin() , data.end() ) ;
/// g_uint16_t w ;
/// g_uint32_t dw ;
/// uev_t uev ;
/// sev_t uev ;
/// bs >> w >> dw >> uev >> sev ;
/// bs.check() ; // throw on underflow
/// \endcode
/// 
/// \see G::bit_iterator
/// 
template <typename T>
class bit_stream
{
public:
	typedef T bit_iterator_type ;
	typedef typename T::byte_iterator_type static_assert_T_is_bit_iterator ;

	bit_stream( T begin , T end ) ;
		///< Constructor taking a G::bit_iterator<X> pair where the underlying 
		///< byte source has a well-defined end.
		///< 
		///< Prefer the other overload if the bit iterator is based on an underlying 
		///< stream, because the underlying byte stream will be left at a poorly-defined 
		///< read position after the bit_stream goes away.

	explicit bit_stream( T begin ) ;
		///< Constructor overload for an G::bit_iterator that has no well-defined 
		///< end; typically bit iterators that are already based on a stream,
		///< such as G::bit_stream<std::istream_iterator<char>>.

	bool get_bool() ;
		///< Extracts a boolean. Sets the fail state on underflow.

	unsigned char get( int bits ) ;
		///< Gets a number of bits, returned in a byte. Sets the fail state on underflow.
		///< Precondition: bits <= 8

	unsigned char get() ;
		///< Gets a byte. Sets the fail state on underflow.
		///< This is matches std::istream::get().

	unsigned char get_byte() ;
		///< Gets a byte. Sets the fail state on underflow.

	g_uint16_t get_word() ;
		///< Gets a word. Sets the fail state on underflow.

	g_uint32_t get_dword() ;
		///< Gets a dword. Sets the fail state on underflow.

	int get_signed_golomb() ;
		///< Gets an exp-golomb encoded signed value. Sets the fail state on underflow.

	unsigned int get_unsigned_golomb() ;
		///< Gets an exp-golomb encoded unsigned value. Sets the fail state on underflow.

	bool at_end( bool default_ = false ) const ;
		///< Returns true if the bit iterator is equal to the end iterator.
		///< Returns the default if the end iterator was not supplied in the ctor.

	bool fail() const ;
		///< Returns the fail state.

	bool good() const ;
		///< Returns !fail().

	void check() ;
		///< Checks for a failed state and throws if failed.

	size_t tellg() const ;
		///< Returns the current bit offset.

private:
	template <typename U> U get_imp( int nbits = sizeof(U) * 8 ) ;

private:
	T m_begin ;
	T m_p ;
	T m_end ;
	bool m_end_defined ;
	bool m_failbit ;
} ;

template <typename T>
bit_stream<T>::bit_stream( T begin , T end ) :
	m_begin(begin) ,
	m_p(begin) ,
	m_end(end) ,
	m_end_defined(true) ,
	m_failbit(false)
{
}

template <typename T>
bit_stream<T>::bit_stream( T begin ) :
	m_begin(begin) ,
	m_p(begin) ,
	m_end(begin) ,
	m_end_defined(false) ,
	m_failbit(false)
{
}

template <typename T>
template <typename U> U bit_stream<T>::get_imp( int nbits )
{
	U result = 0U ;
	for( int i = 0 ; i < nbits ; i++ )
	{
		if( m_end_defined && m_p == m_end ) 
		{ 
			m_failbit = true ; 
			return 0U ; 
		}
		result <<= 1 ;
		result += *m_p++ ;
	}
	return result ;
}

template <typename T>
bool bit_stream<T>::get_bool()
{
	return get_imp<unsigned char>( 1 ) == 1U ;
}

template <typename T>
unsigned char bit_stream<T>::get()
{
	return get_imp<unsigned char>( 8 ) ;
}

template <typename T>
unsigned char bit_stream<T>::get( int n )
{
	return get_imp<unsigned char>( n ) ;
}

template <typename T>
unsigned char bit_stream<T>::get_byte()
{
	return get_imp<unsigned char>( 8 ) ;
}

template <typename T>
g_uint16_t bit_stream<T>::get_word()
{
	return get_imp<g_uint16_t>( 16 ) ;
}

template <typename T>
g_uint32_t bit_stream<T>::get_dword()
{
	return get_imp<g_uint32_t>( 32 ) ;
}

template <typename T>
int bit_stream<T>::get_signed_golomb()
{
	unsigned int u = ExpGolomb::decode<unsigned int>( m_p , m_end , &m_failbit ) ;
	return ExpGolomb::make_signed<int>( u ) ;
}

template <typename T>
unsigned int bit_stream<T>::get_unsigned_golomb()
{
	return ExpGolomb::decode<unsigned int>( m_p , m_end , &m_failbit ) ;
}

template <typename T>
bool bit_stream<T>::at_end( bool default_ ) const
{
	return m_end_defined ? ( m_p == m_end ) : default_ ;
}

template <typename T>
bool bit_stream<T>::good() const
{
	return !fail() ;
}

template <typename T>
bool bit_stream<T>::fail() const
{
	return m_failbit ;
}

template <typename T>
void bit_stream<T>::check()
{
	if( m_failbit )
		throw std::underflow_error("bitstream underflow") ;
}

template <typename T>
size_t bit_stream<T>::tellg() const
{
	return m_p - m_begin ;
}

template <typename T>
bit_stream<T> & operator>>( bit_stream<T> & stream , bool & value )
{
	value = stream.get_bool() ;
	return stream ;
}

template <typename T>
bit_stream<T> & operator>>( bit_stream<T> & stream , unsigned char & value )
{
	value = stream.get_byte() ;
	return stream ;
}

template <typename T>
bit_stream<T> & operator>>( bit_stream<T> & stream , g_uint16_t & value )
{
	value = stream.get_word() ;
	return stream ;
}

template <typename T>
bit_stream<T> & operator>>( bit_stream<T> & stream , g_uint32_t & value )
{
	value = stream.get_dword() ;
	return stream ;
}

template <typename T>
bit_stream<T> & operator>>( bit_stream<T> & stream , ExpGolomb::value<unsigned int> & value )
{
	value.n = stream.get_unsigned_golomb() ;
	return stream ;
}

template <typename T>
bit_stream<T> & operator>>( bit_stream<T> & stream , ExpGolomb::value<int> & value )
{
	value.n = stream.get_signed_golomb() ;
	return stream ;
}

}

#endif

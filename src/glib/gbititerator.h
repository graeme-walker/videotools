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
/// \file gbititerator.h
///

#ifndef G_BIT_ITERATOR__H
#define G_BIT_ITERATOR__H

#include "gdef.h"
#include <iterator> // std::distance()

namespace G
{

/// \class G::bit_iterator
/// A bit-by-bit input iterator that extracts bits in msb-to-lsb order from 
/// a sequence of bytes.
/// 
/// Beware of basing bit_iterators on top of istream_iterators because istream 
/// iterators can extract from the underlying istream when they are advanced. 
/// This means that if the istream lives longer than the iterator then the
/// istream will end up at a poorly-defined position. This bit_iterator 
/// implementation tries to delay the use of op++() to avoid this effect, 
/// but cannot do so for op==().
/// 
template <typename T>
class bit_iterator
{
public:
	typedef T byte_iterator_type ;

	explicit bit_iterator( T bytewise_input_iterator ) ;
		///< Constructor. Points to the msb of the given byte.

	unsigned int operator*() const ;
		///< Dereference operator, returning one or zero.

	bit_iterator & operator++() ;
		///< Advance operator. Moves from msb to lsb within each byte.

	bit_iterator operator++( int ) ;
		///< Advance operator. Moves from msb to lsb within each byte.

	bool operator==( const bit_iterator & other ) const ;
		///< Comparison operator.

	bool operator!=( const bit_iterator & other ) const ;
		///< Comparison operator.

	size_t operator-( const bit_iterator & other ) const ;
		///< Subtraction operator.

private:
	void normalise() const ;

private:
	mutable unsigned char m_mask ;
	mutable T m_p ;
} ;

template <typename T>
bit_iterator<T>::bit_iterator( T p ) :
	m_mask(0x80) ,
	m_p(p)
{
}

template <typename T>
unsigned int bit_iterator<T>::operator*() const
{
	normalise() ;
	unsigned char c = static_cast<unsigned char>( *m_p ) ;
	return ( c & m_mask ) ? 1U : 0U ;
}

template <typename T>
void bit_iterator<T>::normalise() const
{
	// lazy increment because istream_iterators do an extraction in op++
	if( m_mask == 0 )
	{
		m_mask = 0x80 ;
		++m_p ;
	}
}

template <typename T>
bit_iterator<T> & bit_iterator<T>::operator++()
{
	normalise() ;
	m_mask >>= 1 ;
	return *this ;
}

template <typename T>
bit_iterator<T> bit_iterator<T>::operator++( int )
{
	normalise() ;
	bit_iterator old( *this ) ;
	++(*this) ;
	return old ;
}

template <typename T>
bool bit_iterator<T>::operator==( const bit_iterator & other ) const
{
	// this is dodgy because we might be triggering a read of the
	// underlying stream -- the fix is to implement this method using
	// op+(1) and have it fail to compile for underlying iterator types
	// that do not have op+() (eg. istream_iterator) -- template
	// meta-programming will then be required for any template classes 
	// that are based on G::bit_iterator (such as G::bit_stream)
	//
	normalise() ;
	other.normalise() ;

	return m_p == other.m_p && m_mask == other.m_mask ;
}

template <typename T>
bool bit_iterator<T>::operator!=( const bit_iterator & other ) const
{
	return ! ( *this == other ) ;
}

template <typename T>
size_t bit_iterator<T>::operator-( const bit_iterator & other ) const
{
	size_t n = std::distance(other.m_p,m_p) * 8 ;
	unsigned int mask = m_mask ;
	unsigned int other_mask = other.m_mask ;
	if( mask == 0 ) mask = 0x80 , n += 8 ;
	if( other_mask == 0 ) other_mask = 0x80 , n -= 8 ;
	for( ; mask != 0x80 ; mask <<= 1 ) n++ ;
	for( ; other_mask != 0x80 ; other_mask <<= 1 ) n-- ;
	return n ;
}

}

#endif

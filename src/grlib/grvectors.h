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
/// \file grvectors.h
///

#ifndef GR_VECTORS__H
#define GR_VECTORS__H

#include "gdef.h"
#include "grdef.h"
#include "grtraits.h"
#include "glimits.h"
#include "ghexdump.h"
#include <iostream>
#include <streambuf>
#include <algorithm>
#include <utility>
#include <vector>

namespace Gr
{
	/// A candidate for Gr::ImageBuffer that uses nested std::vectors.
	/// 
	typedef std::vector<std::vector<char> > Vectors ;

	/// \namespace Gr::imp
	/// A private implementation namespace.
	/// 
	namespace imp
	{
		class vectors_const_byte_iterator ;
		class vectors_streambuf ;
	}

	/// \namespace Gr::traits
	/// A namespace for traits classes.
	/// 
	namespace traits
	{
		template <>
		struct imagebuffer<Vectors> /// Specialisation for Gr::Vectors.
		{
			typedef imp::vectors_streambuf streambuf_type ;
			typedef imp::vectors_const_byte_iterator const_byte_iterator ;
			typedef Vectors::iterator row_iterator ;
			typedef Vectors::const_iterator const_row_iterator ;
		} ;
	}

	/// \namespace Gr::imagebuffer
	/// A namespace for low-level free functions that operate on container 
	/// classes that are Gr::ImageBuffer candidates.
	/// 
	namespace imagebuffer
	{
		size_t size_of( const Vectors & vectors ) ;
		void resize( Vectors & vectors , int dx , int dy , int channels )  ;
		imp::vectors_const_byte_iterator bytes_begin( const Vectors & vectors ) ;
		imp::vectors_const_byte_iterator bytes_end( const Vectors & ) ;
		bool row_empty( Vectors::iterator row_p ) ;
		void row_resize( Vectors::iterator row_p , size_t n ) ;
		Vectors::iterator row_erase( Vectors & vectors , Vectors::iterator row_p ) ;
		Vectors::const_iterator row_begin( const Vectors & vectors ) ;
		Vectors::iterator row_begin( Vectors & vectors ) ;
		Vectors::const_iterator row_end( const Vectors & vectors ) ;
		Vectors::iterator row_end( Vectors & vectors ) ;
		Vectors::iterator row_add( Vectors & vectors ) ;
		char * row_ptr( Vectors::iterator row_p ) ;
		const char * row_ptr( Vectors::const_iterator row_p ) ;
		size_t row_size( Vectors::const_iterator row_p ) ;
	}
}
std::istream & operator>>( std::istream & stream , Gr::Vectors & ) ;
std::ostream & operator<<( std::ostream & stream , const Gr::Vectors & ) ;

/// \class Gr::imp::vectors_streambuf
/// A simple, read-only streambuf for Gr::Vectors.
/// 
class Gr::imp::vectors_streambuf : public std::streambuf
{
public:
	explicit vectors_streambuf( const Vectors & ) ;
		///< Constructor.

	virtual ~vectors_streambuf() ;
		///< Destructor.

	virtual std::streamsize xsgetn( char * s , std::streamsize n ) override ;
		///< Override from std::streambuf.

	virtual std::streampos seekpos( std::streampos pos , std::ios_base::openmode which ) override ;
		///< Override from std::streambuf.

	virtual std::streampos seekoff( std::streamoff off , std::ios_base::seekdir way , std::ios_base::openmode which ) override ;
		///< Override from std::streambuf.

	virtual int underflow() override ;
		///< Override from std::streambuf.

private:
	vectors_streambuf( const vectors_streambuf & ) ;
	void operator=( const vectors_streambuf & ) ;
	void setg_imp( const char * , const char * , const char * ) ;
	std::streampos pos() const ;
	std::streampos seek( std::streamoff off ) ;
	bool next_buf() ;
	const char * cbuf_head() const ;
	const char * cbuf_tail() const ;
	size_t cbuf_available() const ;
	size_t cbuf_available( const char * ) const ;
	size_t cbuf_offset() const ;
	size_t cbuf_offset( const char * ) const ;

private:
	const Vectors & m_vectors ;
	Vectors::const_iterator m_current ;
	Vectors::const_iterator m_end ;
	std::streampos m_size ;
} ;

/// \class Gr::imp::vectors_const_byte_iterator
/// A forward byte-by-byte input iterator for Gr::ImageVectors.
/// 
class Gr::imp::vectors_const_byte_iterator
{
public:
	vectors_const_byte_iterator() ;
		///< Default constructor for an end iterator.

	explicit vectors_const_byte_iterator( const Vectors & ) ;
		///< Constructor.

	char operator*() const ;
		///< Dereference operator.

	vectors_const_byte_iterator & operator++() ;
		///< Preincrement operator.

	vectors_const_byte_iterator operator++( int ) ;
		///< Postfix preincrement operator.

	bool operator==( const vectors_const_byte_iterator & ) const ;
		///< Equality operator.

	bool operator!=( const vectors_const_byte_iterator & ) const ;
		///< Inequality operator.

private:
	const Vectors * m_vectors ;
	Vectors::const_iterator m_current ;
	size_t m_current_offset ;
	size_t m_offset ;
} ;


namespace G
{
	template <int N> 
	imp::hexdump_streamable<N,Gr::Vectors,typename imp::hexdump_tohex,typename imp::hexdump_toprintable>
	hexdump( const Gr::Vectors & v )
	{
		typedef Gr::imp::vectors_const_byte_iterator iterator ;
		iterator begin( v ) ;
		iterator const end ;
		return hexdump<N>( begin , end ) ;
	}
}


inline
size_t Gr::imagebuffer::size_of( const Vectors & vectors )
{
	size_t n = 0U ;
	add_size< std::vector<char> > adder( n ) ;
	std::for_each( vectors.begin() , vectors.end() , adder ) ;
	return n ;
}

inline
void Gr::imagebuffer::resize( Vectors & vectors , int dx , int dy , int channels ) 
{
	if( dx > 0 && dy > 0 && channels > 0 )
	{
		vectors.resize( dy ) ;
		size_t drow = sizet(dx,channels) ;
		for( int y = 0 ; y < dy ; y++ )
		{
			vectors.at(y).resize( drow ) ;
		}
	}
}

inline
Gr::imp::vectors_const_byte_iterator Gr::imagebuffer::bytes_begin( const Vectors & vectors )
{
	return imp::vectors_const_byte_iterator( vectors ) ;
}

inline
Gr::imp::vectors_const_byte_iterator Gr::imagebuffer::bytes_end( const Vectors & )
{
	return imp::vectors_const_byte_iterator() ;
}

inline
bool Gr::imagebuffer::row_empty( Vectors::iterator row_p )
{
	return (*row_p).empty() ;
}

inline
void Gr::imagebuffer::row_resize( Vectors::iterator row_p , size_t n )
{
	(*row_p).resize( n ) ;
}

inline
Gr::Vectors::iterator Gr::imagebuffer::row_erase( Vectors & vectors , Vectors::iterator row_p )
{
	return vectors.erase( row_p , vectors.end() ) ;
}

inline
Gr::Vectors::const_iterator Gr::imagebuffer::row_begin( const Vectors & vectors )
{
	return vectors.begin() ;
}

inline
Gr::Vectors::iterator Gr::imagebuffer::row_begin( Vectors & vectors )
{
	return vectors.begin() ;
}

inline
Gr::Vectors::const_iterator Gr::imagebuffer::row_end( const Vectors & vectors )
{
	return vectors.end() ;
}

inline
Gr::Vectors::iterator Gr::imagebuffer::row_end( Vectors & vectors )
{
	return vectors.end() ;
}

inline
Gr::Vectors::iterator Gr::imagebuffer::row_add( Vectors & vectors )
{
	vectors.push_back( std::vector<char>() ) ; // emplace_back
	return vectors.begin() + (vectors.size()-1U) ;
}

inline
char * Gr::imagebuffer::row_ptr( Vectors::iterator row_p )
{
	return &(*row_p)[0] ;
}

inline
const char * Gr::imagebuffer::row_ptr( Vectors::const_iterator row_p )
{
	return &(*row_p)[0] ;
}

inline
size_t Gr::imagebuffer::row_size( Vectors::const_iterator row_p )
{
	return (*row_p).size() ;
}


inline
Gr::imp::vectors_const_byte_iterator::vectors_const_byte_iterator() :
	m_vectors(nullptr) ,
	m_current_offset(0U) ,
	m_offset(0U)
{
}

inline
Gr::imp::vectors_const_byte_iterator::vectors_const_byte_iterator( const Vectors & vectors ) :
	m_vectors(&vectors) ,
	m_current(vectors.begin()) ,
	m_current_offset(0U) ,
	m_offset(0U)
{
}

inline
char Gr::imp::vectors_const_byte_iterator::operator*() const
{
	if( m_vectors == nullptr || m_current == m_vectors->end() ) return '\0' ;
	return (*m_current).at(m_current_offset) ;
}

inline
Gr::imp::vectors_const_byte_iterator & Gr::imp::vectors_const_byte_iterator::operator++()
{
	if( m_vectors != nullptr && m_current != m_vectors->end() )
	{
		m_offset++ ;
		m_current_offset++ ;
		if( m_current_offset == (*m_current).size() )
		{
			for( ++m_current ; m_current != m_vectors->end() && (*m_current).empty() ; )
				++m_current ;
			m_current_offset = 0U ;
		}
	}
	return *this ;
}

inline
Gr::imp::vectors_const_byte_iterator Gr::imp::vectors_const_byte_iterator::operator++( int )
{
	vectors_const_byte_iterator orig( *this ) ;
	++(*this) ;
	return orig ;
}

inline
bool Gr::imp::vectors_const_byte_iterator::operator==( const vectors_const_byte_iterator & other ) const
{
	bool this_end = m_vectors == nullptr || m_current == m_vectors->end() ;
	bool other_end = m_vectors == nullptr || m_current == m_vectors->end() ;
	return 
		( this_end && other_end ) ||
		( m_vectors == other.m_vectors && m_offset == other.m_offset ) ;
}

inline
bool Gr::imp::vectors_const_byte_iterator::operator!=( const vectors_const_byte_iterator & other ) const
{
	return !( *this == other ) ;
}

#endif

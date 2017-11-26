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
// grvectors.cpp
//

#include "gdef.h"
#include "grvectors.h"
#include "glimits.h"
#include "gtest.h"
#include "gdebug.h"
#include <algorithm>
#include <stdexcept>
#include <limits>

Gr::imp::vectors_streambuf::vectors_streambuf( const Vectors & vectors ) :
	m_vectors(vectors) ,
	m_current(m_vectors.begin()) ,
	m_end(m_vectors.end()) ,
	m_size(imagebuffer::size_of(m_vectors))
{
	if( m_current != m_end )
		setg_imp( cbuf_head() , cbuf_head() , cbuf_tail() ) ;
}

Gr::imp::vectors_streambuf::~vectors_streambuf()
{
}

int Gr::imp::vectors_streambuf::underflow()
{
	if( m_vectors.empty() || m_current == m_end ) 
		return EOF ;

	if( gptr() == egptr() )
	{
		next_buf() ;
		setg_imp( cbuf_head() , cbuf_head() , cbuf_tail() ) ;
	}

	return m_current == m_end ? EOF : static_cast<int>(static_cast<unsigned char>(*gptr())) ;
}

const char * Gr::imp::vectors_streambuf::cbuf_head() const
{
	return m_current == m_vectors.end() ? nullptr : &(*m_current)[0] ;
}

const char * Gr::imp::vectors_streambuf::cbuf_tail() const
{
	return m_current == m_vectors.end() ? nullptr : ( &(*m_current)[0] + (*m_current).size() ) ;
}

size_t Gr::imp::vectors_streambuf::cbuf_available() const
{
	return cbuf_available( gptr() ) ;
}

size_t Gr::imp::vectors_streambuf::cbuf_available( const char * g ) const
{
	G_ASSERT( g == nullptr || ( g >= cbuf_head() && g <= cbuf_tail() ) ) ;
	return ( g >= cbuf_head() && g < cbuf_tail() ) ? (cbuf_tail()-g) : 0U ;
}

size_t Gr::imp::vectors_streambuf::cbuf_offset( const char * g ) const
{
	G_ASSERT( g == nullptr || ( g >= cbuf_head() && g <= cbuf_tail() ) ) ;
	return ( g >= cbuf_head() && g < cbuf_tail() ) ? (g-cbuf_head()) : 0U ;
}

size_t Gr::imp::vectors_streambuf::cbuf_offset() const
{
	return cbuf_offset( gptr() ) ;
}

bool Gr::imp::vectors_streambuf::next_buf()
{
	for( ++m_current ; m_current != m_end && (*m_current).empty() ; )
		++m_current ;
	return m_current != m_end ;
}

std::streamsize Gr::imp::vectors_streambuf::xsgetn( char * p , std::streamsize n )
{
#if 0
	// use the base class to be slow-but-sure
	return std::streambuf::xsgetn( p , n ) ;
#else

	G_ASSERT( gptr() >= cbuf_head() && gptr() <= cbuf_tail() ) ;

	const char * g = gptr() ;
	if( n > 0 && g == cbuf_tail() )
	{
		next_buf() ;
		g = cbuf_head() ;
	}

	std::streamsize rc = 0U ;
	size_t x = 0U ; // last memcpy size
	size_t co = cbuf_offset( g ) ; // offset into original buffer
	size_t a = cbuf_available( g ) ;

	while( n > 0 && a > 0 )
	{
		x = std::min( static_cast<size_t>(n) , a ) ;
		std::memcpy( p , g , x ) ;
		p += x ;
		n -= x ;
		rc += x ;
		if( n > 0 )
		{
			co = 0U ;
			if( !next_buf() ) break ;
			g = &(*m_current)[0] ;
			a = (*m_current).size() ;
		}
	}

	if( n > 0 )
	{
		G_ASSERT( m_current == m_end ) ;
		setg_imp( nullptr , nullptr , nullptr ) ;
	}
	else
	{
		G_ASSERT( m_current != m_end ) ;
		G_ASSERT( (x+co) <= (*m_current).size() ) ;
		setg_imp( cbuf_head() , cbuf_head()+co+x , cbuf_tail() ) ;
	}
	return rc ;
}
#endif

void Gr::imp::vectors_streambuf::setg_imp( const char * start , const char * get , const char * end )
{
	setg( const_cast<char*>(start) , const_cast<char*>(get) , const_cast<char*>(end) ) ;
}

std::streampos Gr::imp::vectors_streambuf::seekpos( std::streampos pos , std::ios_base::openmode which )
{
	return seekoff( pos , std::ios_base::beg , which ) ;
}

std::streampos Gr::imp::vectors_streambuf::seekoff( std::streamoff off , std::ios_base::seekdir way , std::ios_base::openmode which )
{
	if( which == std::ios_base::in )
	{
		if( way == std::ios_base::beg && off >= 0 )
		{
			return seek( off ) ;
		}
		else if( way == std::ios_base::end && off <= 0 && -off <= m_size )
		{
			return seek( m_size + off ) ;
		}
		else if( way == std::ios_base::cur )
		{
			return seek( pos() + off ) ;
		}
	}
	return -1 ;
}

std::streampos Gr::imp::vectors_streambuf::pos() const
{
	std::streampos n = 0 ;
	for( Vectors::const_iterator p = m_vectors.begin() ; p != m_current && p != m_vectors.end() ; ++p )
	{
		n += (*p).size() ;
	}
	return n + gptr() - eback() ;
}

std::streampos Gr::imp::vectors_streambuf::seek( std::streamoff off )
{
	if( off > m_size )
	{
		return -1 ;
	}
	if( off == m_size )
	{
		setg_imp( nullptr , nullptr , nullptr ) ;
		return m_size ; // ?
	}
	else if( off < 0 )
	{
		return -1 ;
	}
	else
	{
		Vectors::const_iterator p = m_vectors.begin() ;
		for( ; p != m_end && off >= static_cast<std::streamoff>((*p).size()) ; ++p )
		{
			off -= (*p).size() ;
		}
		if( p == m_end )
		{
			setg_imp( nullptr , nullptr , nullptr ) ;
			return -1 ;
		}
		else
		{
			m_current = p ;
			G_ASSERT( size_t(off) < (*m_current).size() ) ;
			setg_imp( cbuf_head() , cbuf_head()+off , cbuf_tail() ) ;
			return off ;
		}
	}
}

// ==

namespace
{
	template <typename T>
	size_t to_sizet( T n , bool do_throw = true , size_t default_ = 0U )
	{
		if( n <= 0 ) 
			return 0U ;

		size_t s = static_cast<size_t>(n) ; // narrow

		bool failed = false ;
		if( (s+1U) == 0U )
			failed = true ; // eg. streamoff(~size_t(0))

		const size_t top_bit = size_t(1U) << (sizeof(size_t)*8-1) ;
		if( (s+1U) & top_bit )
			failed = true ; // eg. streamoff(size_t(-1))

		if( sizeof(T) > sizeof(n) )
		{
			const T nn = static_cast<T>(s) ; // widen back
			if( n != nn )
				failed = true ;
		}

		if( failed )
		{
			if( do_throw )
				throw std::runtime_error( "numeric overflow" ) ;
			else
				s = default_ ;
		}

		return s ;
	}
}

std::istream & operator>>( std::istream & stream , Gr::Vectors & vectors )
{
	size_t buffer_size = static_cast<size_t>( G::limits::file_buffer ) ;
	if( G::Test::enabled("tiny-image-buffers") )
		buffer_size = size_t(10U) ;

	// get the stream size
	size_t stream_size = 0U ;
	if( vectors.empty() )
	{
		std::streampos pos = stream.tellg() ;
		if( pos >= 0 )
		{
			stream.seekg( 0 , std::ios_base::end ) ;
			std::streampos endpos = stream.tellg() ;
			if( endpos >= 0 && endpos > pos )
			{
				std::streamoff diff = endpos - pos ;
				stream_size = to_sizet( diff , false ) ;
			}
			stream.seekg( pos ) ;
			if( stream.tellg() != pos )
				throw std::runtime_error( "Gr::Vectors::operator>>: stream repositioning error" ) ;
		}
	}

	// reserve the outer vector
	if( stream_size > 0U )
	{
		const size_t reservation = (stream_size+buffer_size-1U) / buffer_size ;
		const size_t sanity = 100000U ;
		if( reservation < sanity )
			vectors.reserve( std::min(vectors.max_size(),reservation) ) ;
	}

	// read into existing chunks, or add new chunks as necessary -- try to
	// match the stream size exactly, but still allow for the stream to be
	// bigger than its pre-determined size
	//
	G_ASSERT( (stream_size+1U) != 0U ) ;
	size_t i = 0U ; 
	size_t n = stream_size ; // expected size remaining still to read
	bool over_n = false ; // already got what was expected
	for( ; stream.good() ; i++ )
	{
		if( i >= vectors.size() )
		{
			vectors.push_back( std::vector<char>() ) ; // emplace_back
			std::vector<char> & buffer = vectors.back() ;
			if( n > 0U && !over_n )
				buffer.resize( std::min(buffer_size,n+1U) ) ; // add one to avoid an extra go round at eof
			else
				buffer.resize( buffer_size ) ;
		}

		if( !vectors[i].empty() )
		{
			stream.read( &(vectors[i])[0] , vectors[i].size() ) ;
			std::streamsize stream_gcount = stream.gcount() ;
			if( stream_gcount <= 0 )
				break ;

			size_t gcount = to_sizet( stream_gcount ) ;
			vectors[i].resize( gcount ) ;

			if( gcount >= n )
				n = 0U , over_n = true ;
			else
				n -= gcount ;
		}
	}
	vectors.resize( i ) ;

	// read() sets the failbit if asked to read more than what's 
	// available, but we only want eof in that case
	if( stream.eof() && stream.fail() && !stream.bad() )
		stream.clear( std::ios_base::eofbit ) ;

	return stream ;
}

std::ostream & operator<<( std::ostream & stream , const Gr::Vectors & vectors )
{
	Gr::Vectors::const_iterator const end = vectors.end() ;
	for( Gr::Vectors::const_iterator p = vectors.begin() ; p != end ; ++p )
		stream.write( &(*p)[0] , (*p).size() ) ;
	return stream ;
}

/// \file grvectors.cpp

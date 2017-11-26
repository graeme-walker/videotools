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
// grjpeg.cpp
//

#include "gdef.h"
#include "grjpeg.h"
#include "glog.h"
#include <stdexcept>
#include <istream>

namespace {

struct StreamAdaptor
{
	explicit StreamAdaptor( std::istream & stream_in ) :
		m_s(stream_in) ,
		m_pos0(stream_in.tellg())
	{
	}
	void skip( unsigned int n )
	{
		m_s.seekg( n , std::ios_base::cur ) ;
	}
	bool good() const
	{
		return m_s.good() ;
	}
	unsigned int get()
	{
		if( !good() ) return 0U ;
		return static_cast<unsigned int>(m_s.get()) ;
	}
	unsigned int peek()
	{
		if( !good() ) return 0U ;
		return static_cast<unsigned int>(m_s.peek()) ;
	}
	size_t offset() const
	{
		return m_s.tellg() - m_pos0 ;
	}
	std::istream & m_s ;
	std::streampos m_pos0 ;
} ;

struct DataStream
{
	DataStream( const unsigned char * p , size_t n ) :
		m_begin(p) ,
		m_p(p) ,
		m_end(p+n)
	{
	}
	void skip( unsigned int n )
	{
		m_p += n ;
	}
	bool good() const
	{
		return m_p < m_end ;
	}
	unsigned int get()
	{
		if( m_p >= m_end ) return 0U ;
		return *m_p++ ;
	}
	unsigned int peek()
	{
		if( m_p >= m_end ) return 0U ;
		return *m_p ;
	}
	size_t offset() const
	{
		return m_p - m_begin ;
	}
	const unsigned char * m_begin ;
	const unsigned char * m_p ;
	const unsigned char * m_end ;
} ;

template <typename T>
unsigned int get2( T & in )
{
	unsigned int hi = in.get() ;
	unsigned int lo = in.get() ;
	return (hi<<8) + lo ;
}

template <typename T>
void read( T & in , int & dx , int & dy , int & channels )
{
	dx = 0 ;
	dy = 0 ;
	channels = 0 ;

	const unsigned int sof0 = 0xC0 ;
	const unsigned int sof1 = 0xC1 ;
	const unsigned int sof2 = 0xC2 ;
	const unsigned int sof9 = 0xC9 ;
	const unsigned int sof10 = 0xCA ;

	while( in.good() )
	{
		if( in.get() != 0xff ) break ;
		unsigned int type = in.get() ;
		unsigned int length = in.peek() == 0xff ? 0 : get2(in) ;
		if( type == sof0 || type == sof1 || type == sof2 || type == sof9 || type == sof10 )
		{
			in.get() ; // precision
			unsigned int height = get2(in) ;
			unsigned int width = get2(in) ;
			unsigned int components = in.get() ;
			dx = static_cast<int>( width ) ;
			dy = static_cast<int>( height ) ;
			channels = static_cast<int>( components ) ;
			break ;
		}
		if( length > 2U )
		{
			in.skip( length-2U ) ;
			if( !in.good() )
				G_DEBUG( "gjpeg::read: no sof chunk: offset=" << in.offset() << " skip=" << length ) ;
		}
	}
}

}

Gr::JpegInfo::JpegInfo( std::istream & stream_in )
{
	StreamAdaptor s( stream_in ) ;
	read( s , m_dx , m_dy , m_channels ) ;
}

Gr::JpegInfo::JpegInfo( const unsigned char * p , size_t n )
{
	DataStream s( p , n ) ;
	read( s , m_dx , m_dy , m_channels ) ;
}

Gr::JpegInfo::JpegInfo( const char * p , size_t n )
{
	DataStream s( reinterpret_cast<const unsigned char *>(p) , n ) ;
	read( s , m_dx , m_dy , m_channels ) ;
}

bool Gr::JpegInfo::valid() const
{
	return m_dx > 0 && m_dy > 0 && m_channels > 0 ;
}

int Gr::JpegInfo::dx() const
{
	return valid() ? m_dx : 0 ;
}

int Gr::JpegInfo::dy() const
{
	return valid() ? m_dy : 0 ;
}

int Gr::JpegInfo::channels() const
{
	return valid() ? m_channels : 0 ;
}

/// \file grjpeg.cpp

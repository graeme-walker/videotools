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
// grpnm.cpp
//

#include "gdef.h"
#include "grdef.h"
#include "grpnm.h"
#include "groot.h"
#include "gassert.h"
#include "gstr.h"
#include <algorithm> // std::min/max
#include <utility>
#include <iterator>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace
{
	bool is_space( char c )
	{
		return c == '\n' || c == '\r' || c == ' ' || c == '\t' ;
	}

	bool is_digit( char c )
	{
		return c >= '0' && c <= '9' ;
	}

	unsigned int value256( unsigned int maxval , unsigned int n )
	{
		unsigned long x = n ;
		x <<= 8 ;
		x /= maxval ;
		n = static_cast<unsigned int>(x) ;
		return n > 255U ? 255U : n ;
	}

	template <typename T>
	T skip( T in , T end , size_t & offset )
	{
		bool in_comment = false ;
		while( in != end )
		{
			if( *in == '#' ) in_comment = true ;
			if( in_comment && *in == '\n' ) in_comment = false ;
			if( !in_comment && !is_space(*in) ) break ;
			++in ; offset++ ;
		}
		return in ;
	}

	template <typename T>
	T skip_eol( T in , T end , size_t & offset )
	{
		if( in != end && *in == '\r' ) ++in , offset++ ;
		if( in != end ) ++in , offset++ ;
		return in ; 
	}

	template <typename T>
	int get_n( T & in , T end , size_t & offset )
	{
		if( in == end ) return 0 ;
		int result = 0 ;
		while( in != end && is_digit(*in) )
		{
			result *= 10 ;
			char c = *in++ ; offset++ ;
			result += (c-'0') ;
		}
		return result ;
	}

	template <typename T>
	bool readInfoImp( T & in , T end , Gr::PnmInfo & info )
	{
		int pn = 0 ;
		int dx = 0 ;
		int dy = 0 ; 
		int maxval = 0 ;
		size_t offset = 0U ;

		// read Pn
		if( in == end ) return false ; 
		char p = *in++ ; offset++ ;
		if( in == end || p != 'P' ) return false ;
		char n = *in++ ; offset++ ;
		if( ! ( n >= '1' && n <= '6' ) ) return false ;
		pn = static_cast<int>( n - '0' ) ;

		bool with_maxval = ! ( pn == 1 || pn == 4 ) ;
		bool binary = pn >= 4 ;
		if( in == end ) return false ; 

		// read dx
		in = skip( in , end , offset ) ; 
		if( in == end ) return false ; 
		dx = get_n( in , end , offset ) ; 

		// read dy
		in = skip( in , end , offset ) ;
		if( in == end ) return false ; 
		dy = get_n( in , end , offset ) ; 

		// read maxval
		maxval = 0 ;
		if( with_maxval )
		{
			in = skip( in , end , offset ) ;
			if( in == end ) return false ; 
			maxval = get_n( in , end , offset ) ; 
		}

		in = binary ? skip_eol(in,end,offset) : skip(in,end,offset) ;
		if( in == end ) return false ;

		info = Gr::PnmInfo( pn , dx , dy , maxval , offset ) ;
		return dx > 0 && dy > 0 && (!with_maxval||maxval>0) ;
	}

	template <typename T>
	Gr::PnmInfo readInfoImp( T & in , T end )
	{
		Gr::PnmInfo info ;
		if( !readInfoImp( in , end , info ) )
			return Gr::PnmInfo() ;
		return info ;
	}

	template <typename T>
	struct ScalingAdaptor
	{
		ScalingAdaptor( T & t , const Gr::PnmInfo & info , int scale ) :
			m_t(t) ,
			m_info(info) ,
			m_scale(scale) ,
			m_x_in(0) ,
			m_y_in(0) ,
			m_x_out(0) ,
			m_y_out(0) ,
			m_x_out_max(-1) ,
			m_y_out_max(-1)
		{
		}
		void operator()( int x , int y , unsigned int r , unsigned int g , unsigned int b )
		{
			if( y == m_y_in && x == m_x_in )
			{
				m_x_out_max = m_x_out ;
				m_y_out_max = m_y_out ;
				m_t( m_x_out , m_y_out , r , g , b ) ;
				m_x_in += m_scale ;
				m_x_out++ ;
				if( m_x_in >= m_info.dx() )
				{
					m_x_in = 0 ;
					m_y_in += m_scale ;
					m_x_out = 0 ;
					m_y_out++ ;
				}
			}
		}
		int dx() const
		{
			G_ASSERT( Gr::scaled(m_info.dx(),m_scale) == (m_x_out_max+1) ) ;
			return m_x_out_max + 1 ;
		}
		int dy() const
		{
			G_ASSERT( Gr::scaled(m_info.dy(),m_scale) == (m_y_out_max+1) ) ;
			return m_y_out_max + 1 ;
		}
		private:
		T & m_t ;
		const Gr::PnmInfo & m_info ;
		int m_scale ;
		int m_x_in ;
		int m_y_in ;
		int m_x_out ;
		int m_y_out ;
		int m_x_out_max ;
		int m_y_out_max ;
	} ;

	struct ImageDataAdaptor
	{
		ImageDataAdaptor( Gr::ImageData & data ) :
			m_data(data)
		{
			G_ASSERT( data.dx() > 0 ) ;
		}
		void operator()( int x , int y , unsigned int r , unsigned int g , unsigned int b )
		{
			m_data.rgb( x , y , r , g , b ) ;
		}
		private: Gr::ImageData & m_data ;
	} ;

	struct RawAdaptor
	{
		RawAdaptor( std::vector<char> & out , const Gr::PnmInfo & info ) :
			m_out(out) , 
			m_channels(info.channels())
		{
			G_ASSERT( !out.empty() ) ;
			m_out_p = m_out.begin() ;
		}
		void operator()( int , int , unsigned int r , unsigned int g , unsigned int b )
		{
			if( m_channels == 1 )
			{
				*m_out_p++ = r ;
			}
			else
			{
				*m_out_p++ = r ;
				*m_out_p++ = g ;
				*m_out_p++ = b ;
			}
		}
		private:
		std::vector<char> & m_out ;
		std::vector<char>::iterator m_out_p ;
		int m_channels ;
	} ;

	template <typename Tout>
	void readBodyImp( Tout & out , std::istream & in , const Gr::PnmInfo & info )
	{
		const bool is_binary = info.binary() ;
		const bool is_boolean = info.pn() == 1 || info.pn() == 4 ; // 1bpp
		if( is_binary && is_boolean )
		{
			const int dx_in = (1 + (info.dx()-1)/8) ; // bytes
			const int info_dx = info.dx() ;
			for( int y = 0 ; y < info.dy() ; y++ )
			{
				int x_out = 0 ;
				for( int x_in = 0 ; x_in < dx_in ; x_in++ )
				{
					unsigned int n = in.get() ;
					for( unsigned mask = 0x80 ; mask != 0U && x_out < info_dx ; mask >>= 1 )
						out( x_out++ , y , (n & mask)?0U:255U /*sic*/ , (n & mask)?0U:255U , (n & mask)?0U:255U ) ;
				}
			}
		}
		else
		{
			const int channels_in = ( info.pn() == 3 || info.pn() == 6 ) ? 3 : 1 ;
			for( int y = 0 ; y < info.dy() ; y++ )
			{
				for( int x = 0 ; x < info.dx() ; x++ )
				{
					unsigned int r = 0U ;
					unsigned int g = 0U ;
					unsigned int b = 0U ;
					for( int c = 0 ; c < channels_in ; c++ )
					{
						unsigned int n = 0U ;
						if( is_binary )
							n = in.get() ;
						else
							in >> n ;
	
						if( is_boolean ) 
						{
							n = n ? 0U : 255U ; // sic
						}
						else if( info.maxval() != 0 && info.maxval() != 255 )
						{
							n = value256( info.maxval() , n ) ;
						}
	
						if( c == 0 )
							r = g = b = n ;
						else if( c == 1 )
							g = n ;
						else if( c == 2 )
							b = n ;
					}
					out( x , y , r , g , b ) ;
				}
			}
		}
	}
}

Gr::PnmInfo::PnmInfo( int pn , int dx , int dy , int maxval , size_t offset ) : 
	m_pn(pn) , 
	m_dx(dx) , 
	m_dy(dy) , 
	m_maxval(maxval) , 
	m_offset(offset) 
{
	if( pn == 1 || pn == 4 )
		m_maxval = 1 ;
}

Gr::PnmInfo::PnmInfo( std::istream & in )
{
	std::ios::fmtflags ff = in.flags() ;
	in.unsetf( std::ios::skipws ) ;

	std::istream_iterator<char> p = std::istream_iterator<char>(in) ;
	std::istream_iterator<char> end ;
	PnmInfo info = readInfoImp( p , end ) ;

	in.unget() ; // beware istream-iterator's read-ahead -- extracts on op++(), not op*()

	in.flags( ff ) ;

	if( !info.valid() )
		in.setstate( std::ios_base::failbit ) ; // sic

	*this = info ;
}

Gr::PnmInfo::PnmInfo( const std::vector<char> & buffer )
{
	std::vector<char>::const_iterator p = buffer.begin() ;
	PnmInfo info = readInfoImp( p , buffer.end() ) ;
	*this = info ;
}

Gr::PnmInfo::PnmInfo( const char * buffer , size_t buffer_size )
{
	const char * p = buffer ;
	PnmInfo info = readInfoImp( p , buffer+buffer_size ) ;
	*this = info ;
}

Gr::PnmInfo::PnmInfo( const unsigned char * buffer , size_t buffer_size )
{
	const unsigned char * p = buffer ;
	PnmInfo info = readInfoImp( p , buffer+buffer_size ) ;
	*this = info ;
}

Gr::PnmInfo::PnmInfo( const ImageBuffer & image_buffer )
{
	typedef traits::imagebuffer<ImageBuffer>::const_byte_iterator const_byte_iterator ;
	const_byte_iterator p = imagebuffer::bytes_begin( image_buffer ) ;
	PnmInfo info = readInfoImp( p , imagebuffer::bytes_end(image_buffer) ) ;
	*this = info ;
}

size_t Gr::PnmInfo::rowsize() const
{
	return static_cast<size_t>( dx() * channels() ) ;
}

Gr::PnmReader::PnmReader( int scale , bool monochrome_out ) :
	m_scale(scale) ,
	m_monochrome_out(monochrome_out)
{
}

void Gr::PnmReader::setup( int scale , bool monochrome_out )
{
	m_scale = scale ;
	m_monochrome_out = monochrome_out ;
}

void Gr::PnmReader::decode( ImageData & out , const G::Path & path )
{
	std::ifstream in ;
	{
		G::Root claim_root ;
		in.open( path.str().c_str() ) ;
	}
	m_info = PnmInfo( in ) ;
	if( !m_info.valid() )
		throw std::runtime_error( "failed to read pnm file format: [" + path.str() + "]" ) ;
	if( !readBody(in,m_info,out,m_scale,m_monochrome_out) )
		throw std::runtime_error( "failed to read pnm file: [" + path.str() + "]" ) ;
}

void Gr::PnmReader::decode( ImageData & out , std::istream & in )
{
	m_info = PnmInfo( in ) ;
	if( !m_info.valid() || !readBody(in,m_info,out,m_scale,m_monochrome_out) )
		throw std::runtime_error( "failed to read pnm data" ) ;
}

void Gr::PnmReader::decode( ImageData & out , const char * p , size_t n )
{
	std::istringstream in ;
	in.rdbuf()->pubsetbuf( const_cast<char*>(p) , n ) ;

	m_info = PnmInfo( in ) ;
	if( !m_info.valid() || !readBody(in,m_info,out,m_scale,m_monochrome_out) )
		throw std::runtime_error( "failed to read pnm data" ) ;
}

void Gr::PnmReader::decode( ImageData & out , const Gr::ImageBuffer & image_buffer )
{
	typedef Gr::traits::imagebuffer<ImageBuffer>::streambuf_type imagebuf ;
	imagebuf inbuf( image_buffer ) ;
	std::istream in( &inbuf ) ;
	m_info = PnmInfo( in ) ;
	if( !m_info.valid() || !readBody(in,m_info,out,m_scale,m_monochrome_out) )
		throw std::runtime_error( "failed to read pnm data" ) ;
}

bool Gr::PnmReader::readBody( std::istream & in , const PnmInfo & info , ImageData & data , int scale , bool monochrome_out )
{
	G_ASSERT( info.valid() ) ;
	G_ASSERT( info.dy() >= 1 ) ;
	G_ASSERT( scale >= 1 ) ;
	if( !info.valid() || info.dy() < 1 || scale < 1 )
		return false ;

	if( info.binary() && info.maxval() > 255 )
		return false ;

	data.resize( scaled(info.dx(),scale) , scaled(info.dy(),scale) , monochrome_out ? 1 : info.channels() ) ;

	if( info.binary() && info.maxval() == 255 ) // optimisation
	{
		std::vector<char> buffer( info.rowsize() ) ;
		unsigned char * buffer_p = reinterpret_cast<unsigned char*>(&buffer[0]) ;
		const int data_dy = data.dy() ;
		for( int y = 0 ; y < data_dy ; y++ )
		{
			in.read( &buffer[0] , buffer.size() ) ;
			data.copyRowIn( y , buffer_p , buffer.size() , info.channels() , true , scale ) ;
			for( int i = 0 ; (y+1) < data_dy && i < (scale-1) ; i++ )
				in.ignore( buffer.size() ) ;
		}
	}
	else
	{
		ImageDataAdaptor out( data ) ;
		ScalingAdaptor<ImageDataAdaptor> out_scaled( out , info , scale ) ;
		readBodyImp( out_scaled , in , info ) ;
	}
	bool ok = !in.fail() ;
	return ok ;
}

/// \file grpnm.cpp

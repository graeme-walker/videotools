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
// grimagetype.cpp
//

#include "gdef.h"
#include "grdef.h"
#include "grimagetype.h"
#include "grpng.h"
#include "grjpeg.h"
#include "grpnm.h"
#include "ghexdump.h"
#include "gassert.h"
#include <algorithm> // std::reverse
#include <cstring>

namespace
{
	template <typename T> T append_c( T p , T p_end , char c )
	{
		if( p != p_end ) *p++ = c ;
		return p ;
	}
	template <typename T> T append_s( T p , T p_end , const char * s )
	{
		for( ; *s ; s++ ) 
			p = append_c( p , p_end , *s ) ;
		return p ;
	}
	template <typename T> T append_n( T p_in , T p_end , int n )
	{
		n = std::max( 0 , n ) ;
		T p = p_in ;
		for( ; p != p_end && n > 0 ; n /= 10 )
			*p++ = static_cast<char>('0'+(n%10)) ;
		std::reverse( p_in , p ) ;
		return p ;
	}
	template <typename T>
	T append_size( T p , T p_end , int dx , int dy , int channels )
	{
		p = append_s( p , p_end , ";xsize=" ) ;
		p = append_n( p , p_end , dx ) ;
		p = append_c( p , p_end , 'x' ) ;
		p = append_n( p , p_end , dy ) ;
		p = append_c( p , p_end , 'x' ) ;
		p = append_n( p , p_end , channels ) ;
		append_c( p , p_end , '\0' ) ;
		return p ;
	}
	int atoi( const std::string & s , size_t p , size_t n )
	{
		int result = 0 ;
		for( size_t i = 0U ; i < n ; i++ , p++ )
		{
			if( s.at(p) < '0' || s.at(p) > '9' ) break ;
			result *= 10 ;
			result += static_cast<int>(s.at(p)-'0') ;
		}
		return result ;
	}
}

// ==

Gr::ImageType::ImageType( ImageType::Type type_ , int dx_ , int dy_ , int channels_ ) :
	m_type(type_) ,
	m_dx(dx_) ,
	m_dy(dy_) ,
	m_channels(channels_)
{
	if( !valid() ) 
		m_type = t_invalid ;
}

Gr::ImageType::ImageType() :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
}

Gr::ImageType::ImageType( std::istream & stream ) :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
	init( stream ) ;
	if( !valid() ) 
		m_type = t_invalid ;
}

Gr::ImageType::ImageType( const std::vector<char> & buffer ) :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
	init( reinterpret_cast<const unsigned char *>(&buffer[0]) , buffer.size() ) ;
	if( !valid() ) 
		m_type = t_invalid ;
}

Gr::ImageType::ImageType( const ImageBuffer & buffer ) :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
	typedef traits::imagebuffer<ImageBuffer>::streambuf_type imagebuf ;
	imagebuf inbuf( buffer ) ;
	std::istream instream( &inbuf ) ;
	init( instream ) ;
	if( !valid() ) 
		m_type = t_invalid ;
}

Gr::ImageType::ImageType( const char * p , size_t n ) :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
	init( reinterpret_cast<const unsigned char *>(p) , n ) ;
	if( !valid() ) 
		m_type = t_invalid ;
}

Gr::ImageType::ImageType( const unsigned char * p , size_t n ) :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
	init( p , n ) ;
	if( !valid() ) 
		m_type = t_invalid ;
}

Gr::ImageType::ImageType( const std::string & type_str ) :
	m_type(t_invalid) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0)
{
	int default_channels = 3 ;
	if( type_str.find("image/x.raw") == 0U )
	{
		m_type = t_raw ;
		default_channels = 0 ;
	}
	else if( type_str.find("image/jpeg") == 0U )
	{
		m_type = t_jpeg ;
	}
	else if( type_str.find("image/png") == 0U )
	{
		m_type = t_png ;
	}
	else if( type_str.find("image/x-portable-anymap") == 0U )
	{
		m_type = t_pnm ;
	}

	if( m_type != t_invalid )
	{
		typedef std::string::size_type pos_t ;
		const pos_t npos = std::string::npos ;

		// "image/whatever;xsize=<dx>x<dy>x<channels>"
		pos_t p0 = type_str.find(';') ;
		pos_t p1 = p0 == npos ? npos : type_str.find("xsize=",p0+1) ;
		if( p1 != npos ) p1 += 5U ;
		pos_t p2 = p1 == npos ? npos : type_str.find_first_of(",_x",p1+1U) ;
		pos_t p3 = p2 == npos ? npos : type_str.find_first_of(",_x",p2+1U) ;

		m_dx = 0 ;
		m_dy = 0 ;
		m_channels = default_channels ;

		if( p1 != npos && p2 != npos )
			m_dx = atoi( type_str , p1+1U , p2-p1-1U ) ;

		if( m_dx && p2 != npos && p3 != npos )
			m_dy = atoi( type_str , p2+1U , p3-p2-1U ) ;

		if( m_dy && p3 != npos && p3 < type_str.size() )
			m_channels = atoi( type_str , p3+1U , type_str.size()-p3-1U ) ;

		if( !valid() )
			m_type = t_invalid ;
	}
}

std::string Gr::ImageType::str() const
{
	String s ; 
	set( s ) ;
	return std::string( s.s ) ;
}

bool Gr::ImageType::matches( const std::string & other ) const
{
	String s ; 
	set( s ) ;
	return 0 == std::strcmp( s.c_str() , other.c_str() ) ;
}

void Gr::ImageType::streamOut( std::ostream & stream ) const
{
	String s ;
	set( s ) ;
	stream << s.s ;
}

Gr::ImageType::String & Gr::ImageType::set( String & out ) const
{
	setsimple( out ) ;
	append_size( out.s+std::strlen(out.s) , out.s+sizeof(out.s)-1U , m_dx , m_dy , m_channels ) ;
	out.s[sizeof(out.s)-1U] = '\0' ;
	return out ;
}

void Gr::ImageType::setsimple( String & out ) const
{
	G_ASSERT( sizeof(out.s) > 30U ) ;
	out.s[0] = '\0' ;
	if( m_type == t_jpeg ) std::strcpy( out.s , "image/jpeg" ) ; // ignore warnings
	else if( m_type == t_png ) std::strcpy( out.s , "image/png" ) ; // ignore warnings
	else if( m_type == t_raw ) std::strcpy( out.s , "image/x.raw" ) ; // ignore warnings
	else if( m_type == t_pnm ) std::strcpy( out.s , "image/x-portable-anymap" ) ; // ignore warnings
}

std::string Gr::ImageType::simple() const
{
	String s ;
	setsimple( s ) ;
	return std::string( s.s ) ;
}

bool Gr::ImageType::valid() const
{
	// extra sanity checks and restrictions
	return m_type != t_invalid && m_dx > 0 && m_dy > 0 && (m_channels==1||m_channels==3) ;
}

bool Gr::ImageType::isRaw() const
{
	return m_type == t_raw ;
}

bool Gr::ImageType::isJpeg() const
{
	return m_type == t_jpeg ;
}

bool Gr::ImageType::isPng() const
{
	return m_type == t_png ;
}

bool Gr::ImageType::isPnm() const
{
	return m_type == t_pnm ;
}

size_t Gr::ImageType::size() const 
{ 
	return sizet( m_dx , m_dy , m_channels ) ;
}

size_t Gr::ImageType::rowsize() const
{
	G_ASSERT( isRaw() ) ;
	return sizet( m_dx , 1 , m_channels ) ;
}

bool Gr::ImageType::operator==( const ImageType & other ) const
{
	return m_type == other.m_type && m_dx == other.m_dx && m_dy == other.m_dy && m_channels == other.m_channels ;
}

bool Gr::ImageType::operator!=( const ImageType & other ) const
{
	return !( (*this) == other ) ;
}

bool Gr::ImageType::operator<( const ImageType & other ) const
{
	if( m_type != other.m_type ) return int(m_type) < int(other.m_type) ;
	if( m_dx != other.m_dx ) return m_dx < other.m_dx ;
	if( m_dy != other.m_dy ) return m_dy < other.m_dy ;
	return m_channels < other.m_channels ;
}

Gr::ImageType Gr::ImageType::jpeg( int dx , int dy , int channels )
{
	return ImageType( t_jpeg , dx , dy , channels ) ;
}

Gr::ImageType Gr::ImageType::jpeg( ImageType type_in , int scale , bool monochrome )
{
	return jpeg( scaled(type_in.dx(),scale) , scaled(type_in.dy(),scale) , monochrome?1:type_in.channels() ) ;
}

Gr::ImageType Gr::ImageType::raw( int dx , int dy , int channels )
{
	return ImageType( t_raw , dx , dy , channels ) ;
}

Gr::ImageType Gr::ImageType::raw( ImageType type_in , int scale , bool monochrome )
{
	return raw( scaled(type_in.dx(),scale) , scaled(type_in.dy(),scale) , monochrome?1:type_in.channels() ) ;
}

Gr::ImageType Gr::ImageType::png( int dx , int dy , int channels )
{
	return ImageType( t_png , dx , dy , channels ) ;
}

Gr::ImageType Gr::ImageType::png( ImageType type_in , int scale , bool monochrome )
{
	return png( scaled(type_in.dx(),scale) , scaled(type_in.dy(),scale) , monochrome?1:type_in.channels() ) ;
}

void Gr::ImageType::init( ImageType::Type t , int dx , int dy , int channels )
{
	m_type = t ;
	m_dx = dx ;
	m_dy = dy ;
	m_channels = channels ;
}

void Gr::ImageType::init( std::istream & stream )
{
	std::streampos pos = stream.tellg() ;

	unsigned char buffer[6] = { 0 , 0 , 0 , 0 , 0 , 0 } ;
	stream.read( reinterpret_cast<char*>(&buffer[0]) , sizeof(buffer) ) ;
	size_t n = static_cast<size_t>(stream.gcount()) ;

	stream.seekg( pos , std::ios_base::beg ) ;
	if( pos == -1 || stream.tellg() != pos )
		throw std::runtime_error( "stream not seekable" ) ;

	Type t = typeFromSignature( buffer , n ) ;
	if( t == t_jpeg )
	{
		JpegInfo info( stream ) ;
		if( info.valid() )
			init( t_jpeg , info.dx() , info.dy() , info.channels() ) ;
	}
	else if( t == t_png )
	{
		PngInfo info( stream ) ;
		if( info.valid() )
			init( t_png , info.dx() , info.dy() , 3 ) ;
	}
	else if( t == t_pnm )
	{
		PnmInfo info( stream ) ;
		if( info.valid() )
			init( t_pnm , info.dx() , info.dy() , info.channels() ) ;
	}

	stream.seekg( pos , std::ios_base::beg ) ;
	if( pos == -1 || stream.tellg() != pos )
		throw std::runtime_error( "stream not seekable" ) ;
}

void Gr::ImageType::init( const unsigned char * p , size_t n )
{
	Type t = typeFromSignature( p , n ) ;
	if( t == t_jpeg )
	{
		JpegInfo info( p , n ) ;
		if( info.valid() )
			init( t_jpeg , info.dx() , info.dy() , info.channels() ) ;
	}
	else if( t == t_png )
	{
		PngInfo info( p , n ) ;
		if( info.valid() )
			init( t_png , info.dx() , info.dy() , 3 ) ;
	}
	else if( t == t_pnm )
	{
		PnmInfo info( p , n ) ;
		if( info.valid() )
			init( t_pnm , info.dx() , info.dy() , info.channels() ) ;
	}
}

Gr::ImageType::Type Gr::ImageType::typeFromSignature( const unsigned char * p , size_t n )
{
	if( n > 4U && 
		p[0] == 0xff && p[1] == 0xd8 && // SOI
		p[2] == 0xff )
	{
		return t_jpeg ;
	}
	else if( n > 4U &&
		p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G' )
	{
		return t_png ;
	}
	else if( n > 4U && p[0] == 'P' && 
		// 1..3=>ascii, 4..6=>binary
		( p[1] == '1' || p[1] == '2' || p[1] == '3' || p[1] == '4' || p[1] == '5' || p[1] == '6' ) && 
		( p[2] == ' ' || p[2] == '\n' || p[2] == '\r' ) )
	{
		return t_pnm ;
	}
	else
	{
		return t_invalid ;
	}
}

/// \file grimagetype.cpp

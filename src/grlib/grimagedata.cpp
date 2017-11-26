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
// grimagedata.cpp
//

#include "gdef.h"
#include "grdef.h"
#include "grimagedata.h"
#include "grcolourspace.h"
#include "grglyph.h"
#include "gtest.h"
#include "gassert.h"
#include <algorithm>

Gr::ImageData::ImageData( Type type ) :
	m_type(G::Test::enabled("contiguous")?Contiguous:type) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0) ,
	m_data(m_data_store) ,
	m_rows_set(false)
{
	G_ASSERT( valid() ) ;
}

Gr::ImageData::ImageData( ImageBuffer & image_buffer , Type type ) :
	m_type(type) ,
	m_dx(0) ,
	m_dy(0) ,
	m_channels(0) ,
	m_data(image_buffer) ,
	m_rows_set(false)
{
	G_ASSERT( image_buffer.empty() ) ;
}

Gr::ImageData::ImageData( ImageBuffer & image_buffer , int dx , int dy , int channels ) :
	m_type(image_buffer.size()>1U?Segmented:Contiguous) ,
	m_dx(dx) ,
	m_dy(dy) ,
	m_channels(channels) ,
	m_data(image_buffer) ,
	m_rows_set(false)
{
	G_ASSERT( image_buffer.empty() || ( dx > 0 && dy > 0 && (channels==1 || channels==3) ) ) ;
	G_ASSERT( image_buffer.empty() || image_buffer.size() == 1U || image_buffer.size() == sizet(dy) ) ;
	G_ASSERT( image_buffer.empty() || image_buffer.at(0U).size() == imagebuffer::size_of(image_buffer) || image_buffer.at(0U).size() == sizet(dx,channels) ) ;
	G_ASSERT( imagebuffer::size_of(image_buffer) == sizet(dx,dy,channels) ) ;

	if( image_buffer.empty() && dx != 0 && dy != 0 )
		throw Error( "empty image buffer" ) ;

	if( dx < 0 || dy < 0 || !(image_buffer.empty() || channels==1 || channels==3) )
		throw Error( "invalid dimensions" ) ;

	if( !image_buffer.empty() && image_buffer.size() != 1U && image_buffer.size() != sizet(dy) )
		throw Error( "incorrect image buffer size" ) ;
}

Gr::ImageData::~ImageData()
{
}

Gr::ImageData::Type Gr::ImageData::type() const
{
	return m_type ;
}

bool Gr::ImageData::empty() const
{
	return m_dx == 0 || m_dy == 0 || m_channels == 0 ;
}

void Gr::ImageData::resize( int dx , int dy , int channels )
{
	G_ASSERT( dx > 0 && dy > 0 ) ;
	G_ASSERT( channels == 1 || channels == 3 ) ;

	if( dx == m_dx && dy == m_dy && channels == m_channels )
		return ;

	m_dx = dx ;
	m_dy = dy ;
	m_channels = channels ;

	if( m_type == Contiguous )
	{
		m_data.resize( 1U ) ;
		m_data[0].resize( sizet(m_dx,m_dy,m_channels) ) ;
	}
	else
	{
		m_data.clear() ;
		m_data.reserve( m_dy ) ;
		const size_t rowsize = sizet(m_dx,m_channels) ;
		for( int y = 0 ; y < m_dy ; y++ )
		{
			m_data.push_back( std::vector<char>() ) ; // ie. emplace_back(rowsize)
			m_data.back().resize( rowsize ) ;
		}
	}
	m_rows_set = false ;
	G_ASSERT( valid() ) ;
}

void Gr::ImageData::setRows() const
{
	// set up row pointers lazily
	if( !m_rows_set )
	{
		m_rows.resize( m_dy ) ;
		if( m_type == Contiguous )
		{
			const unsigned char * row_p = storerow(0) ;
			const size_t rowsize = sizet( m_dx , m_channels ) ;
			for( int y = 0 ; y < m_dy ; y++ , row_p += rowsize )
				m_rows[y] = const_cast<unsigned char*>(row_p) ;
		}
		else
		{
			for( int y = 0 ; y < m_dy ; y++ )
				m_rows[y] = const_cast<unsigned char*>(storerow(y)) ;
		}
	}
	m_rows_set = true ;
}

unsigned char ** Gr::ImageData::rowPointers()
{
	setRows() ;
	return &m_rows[0] ;
}

const unsigned char * const * Gr::ImageData::rowPointers() const
{
	setRows() ;
	return &m_rows[0] ;
}

char ** Gr::ImageData::rowPointers( int )
{
	return reinterpret_cast<char**>(rowPointers()) ;
}

bool Gr::ImageData::contiguous() const
{
	return m_data.size() == 1U ;
}

void Gr::ImageData::scale( int scale_in , bool monochrome_out , bool use_colourspace )
{
	G_ASSERT( valid() ) ;
	if( scale_in < 1 ) throw Error( "invalid scale factor" ) ;
	if( scale_in == 1 && (monochrome_out?1:m_channels) == m_channels ) return ;
	if( m_data.empty() ) throw Error( "empty" ) ;

	int x_new = 0 ;
	int y_new = 0 ;
	unsigned char * p_out = storerow( 0 ) ;
	const size_t dp = sizet( m_channels , scale_in ) ;
	for( int y = 0 ; y < m_dy ; y += scale_in , y_new++ )
	{
		const unsigned char * p_in = row( y ) ;
		if( m_data.size() > 1U ) p_out = row( y_new ) ;
		for( int x = x_new = 0 ; x < m_dx ; x += scale_in , p_in += dp , x_new++ )
		{
			if( monochrome_out && m_channels == 3 && use_colourspace )
			{
				*p_out++ = Gr::ColourSpace::y_int( p_in[0] , p_in[1] , p_in[2] ) ;
			}
			else if( monochrome_out || m_channels == 1 )
			{
				*p_out++ = p_in[0] ;
			}
			else
			{
				*p_out++ = p_in[0] ;
				*p_out++ = p_in[1] ;
				*p_out++ = p_in[2] ;
			}
		}
	}
	G_ASSERT( x_new == scaled(m_dx,scale_in) ) ;
	G_ASSERT( y_new == scaled(m_dy,scale_in) ) ;
	m_dx = x_new ;
	m_dy = y_new ;
	m_channels = monochrome_out ? 1 : m_channels ;

	if( m_data.size() == 1U )
	{
		m_data[0].resize( sizet(m_dx,m_dy,m_channels) ) ;
	}
	else
	{
		size_t drow = sizet( m_dx , m_channels ) ;
		m_data.resize( m_dy ) ;
		for( int y = 0 ; y < m_dy ; y++ )
			m_data[y].resize( drow ) ;
	}
	m_rows_set = false ;
	G_ASSERT( valid() ) ;
}

void Gr::ImageData::copyRowOut( int y , std::vector<char> & out , int scale , bool monochrome_out , bool use_colourspace ) const
{
	G_ASSERT( y >= 0 && y < m_dy ) ;
	G_ASSERT( scale >= 1 ) ;
	if( y < 0 || y >= m_dy || scale < 1 )
		throw Error( "invalid parameters" ) ;

	if( scale == 1 && !monochrome_out )
	{
		std::memcpy( &out[0] , row(y) , rowsize() ) ;
	}
	else
	{
		out.resize( sizet(scaled(m_dx,scale),monochrome_out?1:m_channels) ) ;
		const unsigned char * p_in = row( y ) ;
		const unsigned char * p_in_end = p_in + rowsize() ;
		unsigned char * p_out = reinterpret_cast<unsigned char*>(&out[0]) ;
		size_t dp_in = sizet( scale , m_channels ) ;
		for( ; p_in < p_in_end ; p_in += dp_in )
		{
			if( m_channels == 3 && monochrome_out && use_colourspace )
			{
				*p_out++ = Gr::ColourSpace::y_int( p_in[0] , p_in[1] , p_in[2] ) ;
			}
			else if( m_channels == 3 && monochrome_out )
			{
				*p_out++ = *p_in ;
			}
			else if( m_channels == 3 )
			{
				*p_out++ = p_in[0] ;
				*p_out++ = p_in[1] ;
				*p_out++ = p_in[2] ;
			}
			else
			{
				*p_out++ = *p_in ;
			}
		}
	}
}

void Gr::ImageData::copyRowIn( int y , const unsigned char * p_in , size_t n_in , int channels_in , bool use_colourspace , int scale )
{
	G_ASSERT( y >= 0 && p_in != nullptr ) ;
	G_ASSERT( y < m_dy ) ;
	G_ASSERT( channels_in == 1 || channels_in == 3 ) ;
	G_ASSERT( scale >= 1 ) ;
	if( p_in == nullptr || y < 0 || y >= m_dy || !(channels_in==1||channels_in==3) || scale < 1 )
		throw Error( "invalid row parameter" ) ;
	if( m_dx != scaled(int(n_in)/channels_in,scale) )
		throw Error( "invalid row copy" ) ;

	copyRowInImp( row(y) , p_in , channels_in , use_colourspace , scale ) ;
}

void Gr::ImageData::copyRowInImp( unsigned char * row_out , const unsigned char * row_in , int channels_in , bool use_colourspace , int scale )
{
	if( m_channels == 3 && channels_in == 1 )
	{
		// more channels -- triple-up
		int x_out = 0 ;
		const int drow_out = m_dx * 3 ;
		for( int x_in = 0 ; x_out < drow_out ; x_in += scale , x_out += 3 )
		{
			row_out[x_out+0] = row_out[x_out+1] = row_out[x_out+2] = row_in[x_in] ;
		}
	}
	else if( m_channels == 1 && channels_in == 3 )
	{
		// fewer channels -- use first channel or do a colourspace transform
		int x_out = 0 ;
		const int dx_in = 3 * scale ;
		for( int x_in = 0 ; x_out < m_dx ; x_in += dx_in , x_out++ )
		{
			row_out[x_out] = use_colourspace ? Gr::ColourSpace::y_int(row_in[x_in],row_in[x_in+1],row_in[x_in+2]) : row_in[x_in] ;
		}
	}
	else if( scale > 1 && m_channels == 1 )
	{
		int x_out = 0 ;
		for( int x_in = 0 ; x_out < m_dx ; x_in += scale , x_out++ )
		{
			row_out[x_out] = row_in[x_in] ;
		}
	}
	else if( scale > 1 )
	{
		int x_out = 0 ;
		const int dx_in = 3 * scale ;
		int drow_out = m_dx * 3 ;
		for( int x_in = 0 ; x_out < drow_out ; x_in += dx_in , x_out += 3 )
		{
			row_out[x_out+0] = row_in[x_in+0] ;
			row_out[x_out+1] = row_in[x_in+1] ;
			row_out[x_out+2] = row_in[x_in+2] ;
		}
	}
	else
	{
		std::memcpy( row_out , row_in , sizet(m_dx,m_channels) ) ;
	}
}

void Gr::ImageData::copyIn( const char * data_in , size_t size_in , int dx_in , int dy_in , int channels_in , bool use_colourspace , int scale )
{
	G_ASSERT( scale >= 1 ) ;
	G_ASSERT( channels_in==1 || channels_in==3 ) ;
	G_ASSERT( dx_in > 0 && dy_in > 0 ) ;
	G_ASSERT( data_in != nullptr && size_in != 0U ) ;

	if( scale < 1 || data_in == nullptr || size_in == 0U || dx_in <= 0 || dy_in <= 0 || !( channels_in == 1 || channels_in == 3 ) )
		throw Error( "invalid parameter" ) ;

	if( size_in != sizet(dx_in,dy_in,channels_in) || scaled(dx_in,scale) != m_dx || scaled(dy_in,scale) != m_dy )
		throw Error( "copy-in size mismatch" ) ;

	const size_t drow_in = sizet( dx_in , channels_in , scale ) ;
	const unsigned char * p_in = reinterpret_cast<const unsigned char*>(data_in) ;
	for( int y = 0 ; y < m_dy ; y++ , p_in += drow_in )
	{
		copyRowInImp( row(y) , p_in , channels_in , use_colourspace , scale ) ;
	}
}

void Gr::ImageData::copyIn( const ImageBuffer & data_in , int dx_in , int dy_in , int channels_in , bool use_colourspace , int scale )
{
	G_ASSERT( scale >= 1 ) ;
	G_ASSERT( channels_in==1 || channels_in==3 ) ;
	G_ASSERT( !data_in.empty() ) ;
	G_ASSERT( data_in.size() == sizet(dy_in) || data_in.size() == 1U ) ;
	G_ASSERT( data_in.at(0U).size() == sizet(channels_in,dx_in) || data_in.at(0U).size() == sizet(channels_in,dx_in,dy_in) ) ;

	if( scale < 1 || ( data_in.size() != sizet(dy_in) && data_in.size() != 1U ) || data_in.empty() || 
		( data_in.at(0U).size() != sizet(channels_in,dx_in) && data_in.at(0U).size() != sizet(channels_in,dx_in,dy_in) ) ||
		!(channels_in==1 || channels_in==3) )
			throw Error( "invalid parameter" ) ;

	if( scaled(dx_in,scale) != m_dx || scaled(dy_in,scale) != m_dy )
		throw Error( "copy-in size mismatch" ) ;

	if( data_in.size() == 1U ) // contiguous
	{
		const size_t drow_in = sizet(scale,dx_in,channels_in) ;
		const unsigned char * row_in = reinterpret_cast<const unsigned char*>(&(data_in.at(0))[0]) ;
		for( int y_out = 0 ; y_out < m_dy ; y_out++ , row_in += drow_in )
		{
			copyRowInImp( row(y_out) , row_in , channels_in , use_colourspace , scale ) ;
		}
	}
	else
	{
		int y_in = 0 ;
		for( int y_out = 0 ; y_out < m_dy ; y_out++ , y_in += scale )
		{
			const unsigned char * row_in = reinterpret_cast<const unsigned char*>(&(data_in.at(y_in))[0]) ;
			copyRowInImp( row(y_out) , row_in , channels_in , use_colourspace , scale ) ;
		}
	}
}

void Gr::ImageData::copyTo( std::vector<char> & out ) const
{
	out.resize( size() ) ;
	if( contiguous() && m_dy > 0 )
	{
		std::memcpy( &out[0] , storerow(0) , size() ) ; 
	}
	else
	{
		char * out_p = &out[0] ;
		const size_t drow = sizet( m_dx , m_channels ) ;
		for( int y = 0 ; y < m_dy ; y++ , out_p += drow )
		{
			std::memcpy( out_p , row(y) , drow ) ;
		}
	}
}

void Gr::ImageData::copyTo( ImageBuffer & out ) const
{
	typedef traits::imagebuffer<ImageBuffer>::row_iterator row_iterator ;
	imagebuffer::resize( out , m_dx , m_dy , m_channels ) ;
	const size_t drow = rowsize() ;
	int y = 0 ;
	row_iterator const end = imagebuffer::row_end( out ) ;
	for( row_iterator p = imagebuffer::row_begin(out) ; p != end ; ++p , y++ )
	{
		std::memcpy( imagebuffer::row_ptr(p) , row(y) , drow ) ;
	}
}

unsigned char * Gr::ImageData::p()
{
	if( m_dy == 0 )
		throw Error( "empty" ) ;
	if( !contiguous() )
		throw Error( "not contiguous" ) ;
	return storerow(0) ;
}

const unsigned char * Gr::ImageData::p() const
{
	if( m_dy == 0 )
		throw Error( "empty" ) ;
	if( !contiguous() )
		throw Error( "not contiguous" ) ;
	return storerow(0) ;
}

size_t Gr::ImageData::size() const
{
	return sizet( m_dx , m_dy , m_channels ) ;
}

size_t Gr::ImageData::rowsize() const
{
	return sizet( m_dx , m_channels ) ;
}

void Gr::ImageData::fill( unsigned char r , unsigned char g , unsigned char b )
{
	if( (g==r && b==r ) || m_channels == 1 )
	{
		size_t n = rowsize() ;
		for( int y = 0 ; y < m_dy ; y++ )
			std::memset( row(y) , r , n ) ;
	}
	else
	{
		for( int y = 0 ; y < m_dy ; y++ )
		{
			for( int x = 0 ; x < m_dx ; x++ )
				rgb( x , y , r , g , b ) ;
		}
	}
}

template <typename Fn>
void Gr::ImageData::modify( Fn fn )
{
	const size_t drow = rowsize() ;
	for( int y = 0 ; y < m_dy ; y++ )
	{
		unsigned char * p = row( y ) ;
		for( size_t i = 0U ; i < drow ; i++ , p++ )
		{
			fn( *p ) ;
		}
	}
}

template <typename Fn>
void Gr::ImageData::modify( const Gr::ImageData & other , Fn fn )
{
	int dy = std::min( m_dy , other.m_dy ) ;
	int dx = std::min( m_dx , other.m_dx ) ;
	if( m_channels == other.m_channels )
	{
		const size_t drow = sizet( dx , m_channels ) ;
		for( int y = 0 ; y < dy ; y++ )
		{
			unsigned char * p_this = row( y ) ;
			const unsigned char * p_other = other.row( y ) ;
			for( size_t i = 0U ; i < drow ; i++ , p_this++ , p_other++ )
			{
				fn( *p_this , *p_other ) ;
			}
		}
	}
	else if( m_channels == 1 && other.m_channels == 3 )
	{
		const size_t drow = sizet( dx ) ;
		for( int y = 0 ; y < dy ; y++ )
		{
			unsigned char * p_this = row( y ) ;
			const unsigned char * p_other = other.row( y ) ;
			for( size_t i = 0U ; i < drow ; i++ , p_this++ , p_other += 3 )
			{
				fn( *p_this , p_other[0] , p_other[1] , p_other[2] ) ;
			}
		}
	}
	else if( m_channels == 3 && other.m_channels == 1 )
	{
		const size_t drow = sizet( dx , 3 ) ;
		for( int y = 0 ; y < dy ; y++ )
		{
			unsigned char * p_this = row( y ) ;
			const unsigned char * p_other = other.row( y ) ;
			int c = 0 ;
			for( size_t i = 0U ; i < drow ; i++ , p_this++ )
			{
				fn( *p_this , *p_other ) ;
				c = c == 0 ? 1 : ( c == 1 ? 2 : 0 ) ;
				if( c == 0 ) ++p_other ;
			}
		}
	}
}

template <typename Fn>
void Gr::ImageData::modify( const Gr::ImageData & data_1 , const Gr::ImageData & data_2 , Fn fn )
{
	if( m_dx != data_1.m_dx || 
		m_dy != data_1.m_dy || 
		m_channels != data_1.m_channels ||
		data_1.m_dx != data_2.m_dx || 
		data_1.m_dy != data_2.m_dy || 
		data_1.m_channels != data_2.m_channels )
			throw ImageData::Error( "mismatched image sizes" ) ;

	const size_t drow = sizet( m_dx , m_channels ) ;
	for( int y = 0 ; y < m_dy ; y++ )
	{
		unsigned char * p_this = row( y ) ;
		const unsigned char * p_1 = data_1.row( y ) ;
		const unsigned char * p_2 = data_2.row( y ) ;
		for( size_t i = 0U ; i < drow ; i++ , p_this++ , p_1++ , p_2++ )
		{
			fn( *p_this , *p_1 , *p_2 ) ;
		}
	}
}

namespace
{
	struct Shift
	{
		Shift( unsigned int n ) : m_n(n) {}
		void operator()( unsigned char & c ) const
		{
			c >>= m_n ;
		}
		unsigned int m_n ;
	} ;
}

void Gr::ImageData::dim( unsigned int shift )
{
	modify( Shift(shift) ) ;
}

namespace
{
	struct Fractionate
	{
		Fractionate( unsigned int n , unsigned int d ) : m_n(n) , m_d(d) {}
		void operator()( unsigned char & c ) const
		{
			unsigned int cc = c ;
			cc *= m_n ;
			cc /= m_d ;
			cc = std::min( 255U , cc ) ;
			c = static_cast<unsigned char>(cc) ;
		}
		unsigned int m_n ;
		unsigned int m_d ;
	} ;
}

void Gr::ImageData::dim( unsigned int numerator , unsigned int denominator )
{
	if( numerator == 0U )
		fill( 0 , 0 , 0 ) ;
	else if( denominator != 0U && numerator != denominator )
		modify( Fractionate(numerator,denominator) ) ;
}

namespace
{
	struct Mix
	{
		Mix( unsigned int n1 , unsigned int n2 , unsigned int d ) : m_n1(n1) , m_n2(n2) , m_d(d?d:1U) {}
		void operator()( unsigned char & c , unsigned char c_1 , unsigned char c_2 ) const
		{
			unsigned int cc_1 = c_1 ;
			cc_1 *= m_n1 ;
			cc_1 /= m_d ;
			cc_1 = std::min( 255U , cc_1 ) ;

			unsigned int cc_2 = c_2 ;
			cc_2 *= m_n2 ;
			cc_2 /= m_d ;
			cc_2 = std::min( 255U , cc_2 ) ;

			unsigned int cc = cc_1 + cc_2 ;
			cc = std::min( 255U , cc ) ;

			c = static_cast<unsigned char>(cc) ;
		}
		unsigned int m_n1 ;
		unsigned int m_n2 ;
		unsigned int m_d ;
	} ;
}

void Gr::ImageData::mix( const ImageData & data_1 , const ImageData & data_2 , unsigned int n1 , unsigned n2 , unsigned int d )
{
	G_ASSERT( d != 0 ) ;
	resize( data_1.dx() , data_1.dy() , data_1.channels() ) ;
	modify( data_1 , data_2 , Mix(n1,n2,d) ) ;
}

namespace
{
	struct Add
	{
		void operator()( unsigned char & c_this , unsigned char c_other ) const
		{
			const unsigned int n = c_this + c_other ;
			c_this = std::min( n , 255U ) ;
		}
		void operator()( unsigned char & c_this , unsigned char c0 , unsigned char c1 , unsigned char c2 ) const
		{
			this->operator()( c_this , Gr::ColourSpace::y_int(c0,c1,c2) ) ;
		}
	} ;
}

void Gr::ImageData::add( const Gr::ImageData & other )
{
	modify( other , Add() ) ;
}

namespace
{
	struct Subtract
	{
		void operator()( unsigned char & c_this , unsigned char c_other )
		{
			const unsigned int n = c_this > c_other ? static_cast<unsigned int>(c_this-c_other) : 0U ;
			c_this = std::min( n , 255U ) ;
		}
		void operator()( unsigned char & c_this , unsigned char c0 , unsigned char c1 , unsigned char c2 )
		{
			this->operator()( c_this , Gr::ColourSpace::y_int(c0,c1,c2) ) ;
		}
	} ;
}

void Gr::ImageData::subtract( const Gr::ImageData & other )
{
	modify( other , Subtract() ) ;
}

void Gr::ImageData::crop( int dx_new , int dy_new )
{
	if( dx_new >= m_dx && dy_new >= m_dy ) 
		return ; // no-op

	dx_new = std::min( dx_new , m_dx ) ;
	dy_new = std::min( dy_new , m_dy ) ;

	const int top = (m_dy-dy_new) / 2 ;
	const int left = (m_dx-dx_new) / 2 ;

	const size_t dleft = sizet( left , m_channels ) ;
	const size_t dtop = sizet( top , m_dx , m_channels ) ;
	const size_t drow_out = sizet( dx_new , m_channels ) ;
	const size_t drow_in = sizet( m_dx , m_channels ) ;

	if( contiguous() )
	{
		const unsigned char * row_in = storerow(0) + dtop ;
		unsigned char * row_out = storerow(0) ;
		for( int y = 0 ; y < dy_new ; y++ , row_in += drow_in , row_out += drow_out )
		{
			std::memmove( row_out , row_in+dleft , drow_out ) ;
		}
		m_data[0].resize( sizet(dx_new,dy_new,m_channels) ) ;
	}
	else
	{
		std::rotate( m_data.begin() , m_data.begin() + top , m_data.end() ) ;
		m_data.resize( dy_new ) ;
		for( int y = 0 ; y < dy_new ; y++ )
		{
			unsigned char * p_out = storerow( y ) ;
			std::memmove( p_out , p_out+dleft , drow_out ) ;
			m_data[y].resize( drow_out ) ;
		}
	}

	m_dx = dx_new ;
	m_dy = dy_new ;
	m_rows_set = false ;
	G_ASSERT( valid() ) ;
}

void Gr::ImageData::expand( int dx_new , int dy_new )
{
	if( dx_new <= m_dx && dy_new <= m_dy )
		return ; // no-op

	dx_new = std::max( dx_new , m_dx ) ;
	dy_new = std::max( dy_new , m_dy ) ;

	const int top = (dy_new-m_dy) / 2 ;
	const int left = (dx_new-m_dx) / 2 ;
	const int right = dx_new - m_dx - left ;

	const size_t dleft = sizet( left , m_channels ) ;
	const size_t dright = sizet( right , m_channels ) ;
	const size_t drow_in = sizet( m_dx , m_channels ) ;
	const size_t drow_out = sizet( dx_new , m_channels ) ;

	if( contiguous() )
	{
		m_data[0].resize( sizet(dx_new,dy_new,m_channels) ) ;
		const unsigned char * p_in = storerow(0) + (m_dy-1)*m_dx*m_channels ;
		unsigned char * p_out = storerow(0) + (top+(m_dy-1))*dx_new*m_channels ;
		for( int y = m_dy-1 ; y >= 0 ; y-- , p_in -= drow_in , p_out -= drow_out )
		{
			std::memmove( p_out+dleft , p_in , drow_in ) ;
			std::memset( p_out , 0 , dleft ) ;
			std::memset( p_out+dleft+drow_in , 0 , dright ) ;
		}
		p_out = storerow(0) ;
		for( int y = 0 ; y < dy_new ; y++ , p_out += drow_out )
		{
			if( y < top ) std::memset( p_out , 0 , drow_out ) ;
		}
	}
	else
	{
		m_data.resize( dy_new ) ;
		for( int y = m_dy ; y < dy_new ; y++ )
			m_data[y].resize( dx_new ) ;
		std::rotate( m_data.begin() , m_data.begin()+m_dy , m_data.begin()+m_dy+top ) ; // pic-top-bottom -> top-pic-bottom
		for( int y = 0 ; y < dy_new ; y++ )
		{
			m_data[y].resize( drow_out ) ;
			unsigned char * row = storerow( y ) ;
			std::memmove( row+dleft , row , drow_in ) ;
			std::memset( row , 0 , dleft ) ;
		}
	}

	m_dx = dx_new ;
	m_dy = dy_new ;
	m_rows_set = false ;
	G_ASSERT( valid() ) ;
}

namespace
{
	struct GlyphOut
	{
		GlyphOut( Gr::ImageData & image_data , int x , int y ,
			Gr::Colour foreground , Gr::Colour background , bool draw_background ) :
				m_image_data(image_data) ,
				m_x(x) ,
				m_y(y) ,
				m_foreground(foreground) ,
				m_background(background) ,
				m_draw_background(draw_background)
		{
		}
		void operator()( int x_in , int y_in , bool b )
		{
			int x = m_x + x_in ;
			int y = m_y + y_in ;
			if( x >= 0 && x < m_image_data.dx() && 
				y >= 0 && y < m_image_data.dy() )
			{
				if( b )
					m_image_data.rgb( x , y , m_foreground.r() , m_foreground.g() , m_foreground.b() ) ;
				else if( m_draw_background )
					m_image_data.rgb( x , y , m_background.r() , m_background.g() , m_background.b() ) ;
			}
		}
		Gr::ImageData & m_image_data ;
		int m_x ;
		int m_y ;
		Gr::Colour m_foreground ;
		Gr::Colour m_background ;
		bool m_draw_background ;
	} ;
}

Gr::ImageDataWriter Gr::ImageData::writer( int x , int y , 
	Colour foreground , Colour background , bool draw_background , bool wrap_on_nl )
{
	return ImageDataWriter( *this , x , y , foreground , background , draw_background , wrap_on_nl ) ;
}

void Gr::ImageData::write( char c , int x , int y , 
	Gr::Colour foreground , Gr::Colour background , bool draw_background )
{
	GlyphOut out( *this , x , y , foreground , background , draw_background ) ;
	Glyph::output( std::string(1U,c) , out ) ;
}

namespace
{
	struct checker
	{
		checker( bool & ok ) : m_ok(ok) {}
		void operator()( bool checked_ok ) { G_ASSERT(checked_ok) ; if(!checked_ok) m_ok = false ; }
		bool & m_ok ;
	} ;
}

bool Gr::ImageData::valid() const
{
	// this is only used in a debug build to check class invariants...

	bool ok = true ;
	checker check( ok ) ;

	if( m_dx == 0 || m_dx == 0 || m_channels == 0 || m_data.empty() )
	{
		check( m_dx == 0 ) ;
		check( m_dy == 0 ) ;
		check( m_channels == 0 ) ;
		check( m_data.empty() ) ;
		check( !m_rows_set ) ;
	}
	else
	{
		check( m_dx > 0 && m_dy > 0 && m_channels > 0 ) ;
		check( m_data.empty() || m_data.size() == 1U || m_data.size() == sizet(m_dy) ) ;
		if( m_data.empty() )
		{
			check( m_rows.empty() ) ;
		}
		else if( m_data.size() == 1U )
		{
			check( m_data[0].size() == sizet(m_dx,m_dy,m_channels) ) ;
			check( !m_rows_set || m_rows.size() == sizet(m_dy) ) ;
			check( !m_rows_set || m_rows[0] == storerow(0) ) ;
			check( contiguous() ) ;
		}
		else
		{
			check( m_data[0].size() == sizet(m_dx,m_channels) ) ;
			check( m_data[1].size() == sizet(m_dx,m_channels) ) ;
			if( m_rows_set )
			{
				check( m_rows.size() == sizet(m_dy) ) ;
				for( size_t y = 0 ; y < std::min(m_rows.size(),sizet(m_dy)) ; y++ )
					check( m_rows.at(y) == storerow(y) ) ;
			}
		}
	}
	return ok ;
}

// ==

Gr::ImageDataWriter::ImageDataWriter( Gr::ImageData & data , int x , int y , 
	Colour foreground , Colour background , bool draw_background , bool wrap_on_nl ) :
		m_data(&data),
		m_dx(data.dx()),
		m_dy(data.dy()),
		m_x0(x) ,
		m_x(x),
		m_y(y),
		m_foreground(foreground) ,
		m_background(background) ,
		m_draw_background(draw_background) ,
		m_wrap_on_nl(wrap_on_nl)
{
}

bool Gr::ImageDataWriter::pre( char c )
{
	if( m_wrap_on_nl && c == '\n' )
	{
		m_x = m_x0 ;
		m_y += 9 ;
		return false ;
	}
	else
	{
		return true ;
	}
}

void Gr::ImageDataWriter::post( char c )
{
	m_x += 9 ;
	if( (m_x+8) > m_dx )
	{
		m_x = m_x0 ;
		m_y += 9 ;
	}
}

/// \file grimagedata.cpp

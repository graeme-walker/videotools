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
// gvcapturebuffer.cpp
//

#include "gdef.h"
#include "gvcapturebuffer.h"
#include <stdexcept>
#include <sys/mman.h> // munmap
#include <stdlib.h>

namespace
{
	void * memalign_( void **aligned_p , size_t page , size_t n )
	{
		// roll our own for portability
		void * p = malloc( n + page + 1 ) ;
		char *pp = reinterpret_cast<char*>(p) ;
		while( (reinterpret_cast<unsigned long>(pp) % page) != 0 )
			pp++ ;
		*aligned_p = pp ;
		return p ; // free()able
	}
}

Gv::CaptureBuffer::CaptureBuffer( size_t length ) :
	m_format(nullptr) ,
	m_freeme(nullptr) ,
	m_start(nullptr) ,
	m_length(length) ,
	m_unmap(0)
{
	// "read" i/o -- malloc(), with free() in dtor
	m_freeme = m_start = malloc( length ) ;
	if( m_start == nullptr )
		throw std::runtime_error( "out of memory" ) ;
}

Gv::CaptureBuffer::CaptureBuffer( size_t length , void * p , int (*unmap)(void*,size_t) ) :
	m_format(nullptr) ,
	m_freeme(nullptr) ,
	m_start(p) ,
	m_length(length) ,
	m_unmap(unmap)
{
	// "mmap" i/o -- no-op, with munmap() in dtor
	G_ASSERT( p != nullptr ) ;
} 

Gv::CaptureBuffer::CaptureBuffer( size_t page_size , size_t buffer_size ) :
	m_format(nullptr) ,
	m_length(size4(buffer_size)) ,
	m_unmap(0)
{
	// "userptr" i/o -- aligned malloc(), with free() in dtor
	m_freeme = memalign_( &m_start , page_size , buffer_size ) ;
	if( m_start == nullptr )
		throw std::runtime_error( "out of memory" ) ;
}

Gv::CaptureBuffer::~CaptureBuffer()
{
	if( m_freeme )
	{
		free( m_freeme ) ;
	}
	else // mmap
	{
		G_IGNORE_RETURN( int , (*m_unmap)( m_start, m_length ) ) ;
	}
}

void Gv::CaptureBuffer::setFormat( const CaptureBufferFormat & f , const CaptureBufferScale & s )
{
	m_format = &f ;
	m_scale = s ;
	checkFormat() ;
}

void Gv::CaptureBuffer::checkFormat() const
{
	// sanity check the format vs. the buffer size
	G_ASSERT( m_scale.m_dx > 0U && m_scale.m_dy > 1U ) ;
	const size_t x_1 = m_scale.m_dx - 1U ;
	const size_t y_1 = m_scale.m_dy - 1U ;
	const size_t offset_0 = m_format->component(0).offset(x_1,y_1,m_scale) ;
	const size_t offset_1 = m_format->component(1).offset(x_1,y_1,m_scale) ;
	const size_t offset_2 = m_format->component(2).offset(x_1,y_1,m_scale) ;
	const size_t max_offset = std::max(offset_0,std::max(offset_1,offset_2)) ;
	G_ASSERT( max_offset < size() ) ;
	//G_ASSERT( (size()-max_offset) < 4 ) ; // too strong for libv4l
	if( max_offset >= size() )
		throw std::runtime_error( "pixelformat inconsistent with capture buffer size" ) ;
}

void Gv::CaptureBuffer::copyTo( Gr::ImageData & data ) const
{
	if( m_format->is_simple() && m_scale.is_simple(data.dx(),data.dy()) && data.channels() == 3 ) // optimisation
	{
		G_ASSERT( data.rowsize() == m_scale.m_linesize ) ;
		const size_t rowsize = data.rowsize() ;
		const unsigned char * p_in = begin() ;
		for( size_t y = 0U ; y < m_scale.m_dy ; y++ , p_in += rowsize )
		{
			std::memcpy( data.row(y) , p_in , rowsize ) ;
		}
	}
	else
	{
		for( size_t y = 0U ; y < m_scale.m_dy ; y++ )
		{
			CaptureBufferIterator bp = row( y ) ;
			for( size_t x = 0U ; x < m_scale.m_dx ; x++ , ++bp )
			{
				typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
				const triple_t & rgb = *bp ;
				data.rgb( x , y , rgb.r() , rgb.g() , rgb.b() ) ;
			}
		}
	}
}

void Gv::CaptureBuffer::copy( int dx , int dy , unsigned char * p_out , size_t n_out ) const
{
	G_ASSERT( p_out != nullptr && dx > 0 && dy > 0 && n_out > 0U ) ;
	G_ASSERT( n_out == (size_t(dx)*size_t(dy)*size_t(3U)) ) ;
	if( p_out == nullptr || n_out != (size_t(dx)*size_t(dy)*size_t(3U)) )
		throw std::runtime_error( "capture buffer size mismatch" ) ;

	if( m_format->is_simple() && m_scale.is_simple(dx,dy) ) // optimisation
	{
		G_ASSERT( n_out == size() ) ;
		std::memcpy( p_out , begin() , std::min(n_out,size()) ) ;
	}
	else
	{
		unsigned char * const p_end = p_out + n_out ;
		for( int y = 0 ; y < dy ; y++ )
		{
			CaptureBufferIterator bp = row( y ) ;
			if( (p_out+dx+dx+dx) > p_end ) break ; // just in case
			for( int x = 0 ; x < dx ; x++ , ++bp )
			{
				typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
				triple_t rgb = *bp ;
				*p_out++ = rgb.r() ;
				*p_out++ = rgb.g() ;
				*p_out++ = rgb.b() ;
			}
		}
	}
}

void Gv::CaptureBuffer::copy( int dx , int dy , char * p_out , size_t n ) const
{
	copy( dx , dy , reinterpret_cast<unsigned char *>(p_out) , n ) ;
}

// ==

Gv::CaptureBufferComponent::CaptureBufferComponent() :
	m_offset(0),
	m_x_shift(0) ,
	m_y_shift(0) ,
	m_step(3),
	m_depth(8),
	m_mask(0xff),
	m_shift(0),
	m_type(c_byte) ,
	m_linesize_shift(0) ,
	m_simple(false)
{
}

Gv::CaptureBufferComponent::CaptureBufferComponent( size_t offset_ , unsigned short x_shift_ , unsigned short y_shift_ , 
	size_t step_ , unsigned short depth_ , unsigned short shift_ , Type type_ , short linesize_shift_ ) :
		m_offset(offset_) ,
		m_x_shift(x_shift_) ,
		m_y_shift(y_shift_) ,
		m_step(step_) ,
		m_depth(depth_) ,
		m_mask(0U) ,
		m_shift(shift_) ,
		m_type(type_) ,
		m_linesize_shift(linesize_shift_) ,
		m_simple(m_depth==8&&m_type==c_byte&&m_shift==0) 
{
	m_mask = (1U << m_depth) - 1U ;
}

// ==

Gv::CaptureBufferFormat::CaptureBufferFormat( unsigned int id_ , const char * name_ , Type type_ ) :
	m_id(id_) ,
	m_name(name_) ,
	m_type(type_)
{
}

Gv::CaptureBufferFormat::CaptureBufferFormat( unsigned int id_ , const char * name_ , Type type_ , 
	const CaptureBufferComponent & c0 , const CaptureBufferComponent & c1 , const CaptureBufferComponent & c2 ) :
		m_id(id_) ,
		m_name(name_) ,
		m_type(type_)
{
	m_c[0] = c0 ;
	m_c[1] = c1 ;
	m_c[2] = c2 ;
}

bool Gv::CaptureBufferFormat::is_simple() const
{
	return m_type == rgb && m_c[0].is_simple(0) && m_c[1].is_simple(1) && m_c[2].is_simple(2) ;
}

/// \file gvcapturebuffer.cpp

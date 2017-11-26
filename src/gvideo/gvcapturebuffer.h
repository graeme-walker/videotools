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
/// \file gvcapturebuffer.h
///

#ifndef GV_CAPTURE_BUFFER__H
#define GV_CAPTURE_BUFFER__H

#include "gdef.h"
#include "grcolourspace.h"
#include "grimagedata.h"
#include "gassert.h"
#include <cstdlib>

namespace Gv
{
	class CaptureBuffer ;
	class CaptureBufferFormat ;
	class CaptureBufferScale ;
	class CaptureBufferComponent ;
	class CaptureBufferIterator ;
}

/// \class Gv::CaptureBufferScale
/// A structure holding capture buffer dimensions. The dimensions are obtained 
/// from a successful set-format ioctl.
/// 
class Gv::CaptureBufferScale
{
public:
	CaptureBufferScale() ;
		///< Default constructor.

	CaptureBufferScale( size_t buffersize_ , size_t linesize_ , size_t dx_ , size_t dy_ ) ;
		///< Constructor.

	bool is_simple( int dx , int dy ) const ;
		///< Returns true if this buffer scale matches the given image
		///< size using packed rgb with no end-of-line padding.

public:
	size_t m_buffersize ;
	size_t m_linesize ;
	size_t m_dx ;
	size_t m_dy ;
	size_t m_xy ;
	size_t m_xy2 ;
	size_t m_xy4 ;
	size_t m_xy8 ;
} ;

/// \class Gv::CaptureBuffer
/// A video-capture buffer class to hold image data, with overloaded constructors
/// for the various V4l i/o mechanisms. The buffer contents are straight from
/// the video device; the setFormat() must be used to tell the buffer what
/// format it contains. The accessor methods row() and copy()/copyTo() can then 
/// be used to extract RGB pixels. The low-level pixelformat conversion is 
/// performed by the Gv::CaptureBufferIterator class.
/// 
class Gv::CaptureBuffer
{
public:
	explicit CaptureBuffer( size_t length ) ;
		///< Constructor for a malloc()ed buffer suitable for "read()" i/o.

	CaptureBuffer( size_t length , void * p , int (*unmap)(void*,size_t) ) ; 
		///< Constructor for a mmap-ed i/o.

	CaptureBuffer( size_t page_size , size_t buffer_size ) ;
		///< Constructor for a memalign()ed buffer suitable for "userptr" i/o.

	~CaptureBuffer() ;
		///< Destructor.

	const unsigned char * begin() const ;
		///< Returns a pointer to start of the dword-aligned data buffer.

	unsigned char * begin() ;
		///< Non-const overload.

	const unsigned char * end() const ;
		///< Returns a pointer off the end of the raw data buffer.

	size_t size() const ;
		///< Returns the size of the buffer.

	CaptureBufferIterator row( int y ) const ;
		///< Returns a pixel iterator for the y'th row.

	void setFormat( const CaptureBufferFormat & , const CaptureBufferScale & ) ;
		///< Used by the Gv::Capture class to imbue the buffer with a particular
		///< format description and scale.

	void copyTo( Gr::ImageData & ) const ;
		///< Copies the image to a correctly-sized image data buffer.

	void copy( int dx , int dy , char * p_out , size_t out_size ) const ;
		///< Copies the image to an rgb output buffer.

	void copy( int dx , int dy , unsigned char * p_out , size_t out_size ) const ;
		///< Overload for unsigned char.

	size_t offset_imp( int c , int x , int y ) const ;
		///< Used by Gv::CaptureBufferIterator.

	size_t offset_imp( int c , int y ) const ;
		///< Used by Gv::CaptureBufferIterator.

	Gr::ColourSpace::triple<unsigned char> rgb_imp( size_t , size_t , size_t ) const ;
		///< Used by Gv::CaptureBufferIterator.

	unsigned char luma_imp( size_t , size_t , size_t ) const ;
		///< Used by Gv::CaptureBufferIterator.

private:
	CaptureBuffer( const CaptureBuffer & ) ;
	void operator=( const CaptureBuffer & ) ;
	unsigned int value( int c , size_t ) const ;
	unsigned int value( int c , int x , int y ) const ;
	unsigned char value8( int c , int x , int y ) const ;
	unsigned char value8( int c , size_t ) const ;
	void checkFormat() const ;
	static size_t size4( size_t ) ;

private:
	const CaptureBufferFormat * m_format ;
	CaptureBufferScale m_scale ;
	mutable void * m_freeme ;
	mutable void * m_start ;
	size_t m_length ;
	int (*m_unmap)(void*,size_t) ; 
} ;

/// \class Gv::CaptureBufferComponent
/// A descriptor for one colour component in a Gv::CaptureBufferFormat structure.
/// 
class Gv::CaptureBufferComponent
{
public:
	enum PlanarOffset { XY = 0x800 , XY2 = 0x400 , XY4 = 0x200 , XY8 = 0x100 } ;
	enum Type { c_byte , c_word_le , c_word_be } ;

	CaptureBufferComponent() ;
		///< Default constructor for a simple step-three component.

	CaptureBufferComponent( size_t offset_ , unsigned short x_shift_ , unsigned short y_shift_ , 
		size_t step_ , unsigned short depth_ = 8 , unsigned short shift_ = 0 , Type type_ = c_byte ,
		short linesize_shift_ = 0 ) ;
			///< Constructor. The offset parameter is small fixed offset 
			///< (eg. 0, 1 or 2) optionally combined with flags of 
			///< XY/XY4/XY8 representing fractions of dy*linesize.

	unsigned int value( const unsigned char * p ) const ;
		///< Returns the value at p.

	unsigned char value8( const unsigned char * p ) const ;
		///< Returns the 0..255 value at p.

	size_t offset( int x , int y , const CaptureBufferScale & scale ) const ;
		///< Returns the buffer offset for the given pixel.

	size_t offset( int y , const CaptureBufferScale & scale ) const ;
		///< Returns the buffer offset for a start-of-row pixel.

	bool is_simple( size_t offset ) const ;
		///< Returns true if the component x_shift and y_shift are zero,
		///< the depth is eight, the shift is zero, the step is three,
		///< and the offset matches the parameter.

	unsigned short xshift() const ;
		///< Returns the x_shift.

	unsigned short yshift() const ;
		///< Returns the y_shift.

	size_t step() const ;
		///< Returns the step.

	unsigned short depth() const ;
		///< Returns the depth.

private:
	bool is_simple() const ;

private:
	size_t m_offset ; // fixed offset into the buffer, with symbolic high bits
	unsigned short m_x_shift ; // right shift of x (for chroma sub-sampling)
	unsigned short m_y_shift ; // right shift of y (for chroma sub-sampling)
	size_t m_step ; // offset for each new pixel
	unsigned short m_depth ; // number of bits, eg. 8
	unsigned int m_mask ; // mask for 'depth' bits
	unsigned short m_shift ; // right-shift to get the value
	Type m_type ; // bytes-or-words enum
	short m_linesize_shift ; // right shift of overall linesize (for planar formats)
	bool m_simple ; // simple wrt. type/depth/shift
} ;

/// \class Gv::CaptureBufferFormat
/// A descriptor for a v4l pixel format. The descriptor, together with a scale 
/// structure, allow other classes to extract pixel values from a raw v4l buffer 
/// that is laid out in the way described.
/// 
class Gv::CaptureBufferFormat
{
public:
	enum Type { rgb , yuv , grey } ; // colour components

	CaptureBufferFormat( unsigned int id_ = 0 , const char * name_ = "" , Type type_ = rgb ) ;
		///< Default constructor for an unusable, un-named rgb format.

	CaptureBufferFormat( unsigned int id_ , const char * name_ , Type type_ , 
		const CaptureBufferComponent & , const CaptureBufferComponent & , const CaptureBufferComponent & ) ;
			///< Constructor for a named format. Use a string literal for the name.

	bool is_grey() const ;
		///< Returns true if grey colour components.

	bool is_rgb() const ;
		///< Returns true if rgb colour components.

	bool is_yuv() const ;
		///< Returns true if yuv colour components.

	bool is_simple() const ;
		///< Returns true for a packed rgb format with 24 bits per pixel.

	const CaptureBufferComponent & component( int c ) const ;
		///< Returns the c'th colour component descriptor.
		///< Every format has exactly three components; grey formats
		///< have a zero depth for two of them.

	unsigned int id() const ;
		///< Returns the id.

	const char * name() const ;
		///< Returns the name.

	Type type() const ;
		///< Returns the type.

private:
	unsigned int m_id ;
	const char * m_name ;
	Type m_type ; 
	CaptureBufferComponent m_c[3] ;
} ;

/// \class Gv::CaptureBufferIterator
/// A const iterator for a single row of Gv::CaptureBuffer pixels.
/// 
class Gv::CaptureBufferIterator
{
public:
	explicit CaptureBufferIterator( const CaptureBuffer & , 
		size_t offset_0 , unsigned short x_shift_0 , size_t step_0 ,
		size_t offset_1 , unsigned short x_shift_1 , size_t step_1 ,
		size_t offset_2 , unsigned short x_shift_2 , size_t step_2 ) ;
			///< Constructor used CaptureBuffer::row().

	Gr::ColourSpace::triple<unsigned char> rgb() const ;
		///< Returns the three rgb values at the current position.

	Gr::ColourSpace::triple<unsigned char> operator*() const ;
		///< Same as rgb().

	unsigned char luma() const ;
		///< Returns the luma value at the current position.

	CaptureBufferIterator & operator++() ;
		///< Advances the iterator to the next pixel in the row.

	bool operator==( const CaptureBufferIterator & ) const ;
		///< Comparison operator.

	bool operator!=( const CaptureBufferIterator & ) const ;
		///< Comparison operator.

private:
	const CaptureBuffer & m_buffer ;
	unsigned int m_x ;
	size_t m_offset[3] ;
	unsigned int m_x_shift_mask[3] ;
	size_t m_step[3] ;
} ;


inline
Gv::CaptureBufferScale::CaptureBufferScale() :
	m_buffersize(0U) ,
	m_linesize(0U) ,
	m_dy(0U) ,
	m_xy(0U) ,
	m_xy2(0U) ,
	m_xy4(0U) ,
	m_xy8(0U)
{
}

inline
Gv::CaptureBufferScale::CaptureBufferScale( size_t buffersize_ , size_t linesize_ , size_t dx_ , size_t dy_ ) :
	m_buffersize(buffersize_) ,
	m_linesize(linesize_) ,
	m_dx(dx_) ,
	m_dy(dy_) ,
	m_xy(dy_*linesize_) ,
	m_xy2(m_xy/2U) ,
	m_xy4(m_xy/4U) ,
	m_xy8(m_xy/8U)
{
	G_ASSERT( m_buffersize > m_linesize && m_dy != 0U ) ;
}

inline
bool Gv::CaptureBufferScale::is_simple( int dx_in , int dy_in ) const
{
	size_t dx = dx_in ;
	size_t dy = dy_in ;
	return m_dy == dy && m_linesize == (dx*3U) && m_buffersize == (dx*dy*3U) ;
}


inline
unsigned short Gv::CaptureBufferComponent::depth() const
{
	return m_depth ;
}

inline
size_t Gv::CaptureBufferComponent::step() const
{
	return m_step ;
}

inline
unsigned short Gv::CaptureBufferComponent::xshift() const
{
	return m_x_shift ;
}

inline
unsigned short Gv::CaptureBufferComponent::yshift() const
{
	return m_y_shift ;
}

inline
size_t Gv::CaptureBufferComponent::offset( int y , const CaptureBufferScale & scale ) const
{
	const unsigned short offset_small = m_offset & 0xff ;
	const size_t linesize = m_linesize_shift == 0 ? scale.m_linesize :
		( m_linesize_shift >= 1 ? (scale.m_linesize>>m_linesize_shift) : (scale.m_linesize<<-m_linesize_shift) ) ;
	const size_t offset_line = (y>>m_y_shift) * linesize ;

	// (optimised (?) multiplication)
	const bool add_xy = !!( m_offset & XY ) ;
	const bool add_xy2 = !!( m_offset & XY2 ) ;
	const bool add_xy4 = !!( m_offset & XY4 ) ;
	const bool add_xy8 = !!( m_offset & XY8 ) ;
	const size_t offset_plane = 
		(add_xy?scale.m_xy:0U) + 
		(add_xy2?scale.m_xy2:0U) +
		(add_xy4?scale.m_xy4:0U) +
		(add_xy8?scale.m_xy8:0U) ;

	return offset_small + offset_plane + offset_line ;
}

inline
size_t Gv::CaptureBufferComponent::offset( int x , int y , const CaptureBufferScale & scale ) const
{
	return offset(y,scale) + ((x>>m_x_shift)*m_step) ;
}

inline
unsigned int Gv::CaptureBufferComponent::value( const unsigned char * p ) const
{
	unsigned int v = *p ;
	if( m_type != c_byte )
	{
		unsigned int v2 = p[1] ;
		v = m_type == c_word_be ? ((v<<8)|v2) : ((v2<<8)|v) ;
	}
	return ( v >> m_shift ) & m_mask ;
}

inline
unsigned char Gv::CaptureBufferComponent::value8( const unsigned char * p ) const
{
	if( m_simple ) // wrt. depth/type/shift
		return *p ;

	const unsigned int v = value(p) ;
	if( m_depth == 8 )
		return v ;
	else if( m_depth < 8 )
		return v << (8-m_depth) ;
	else
		return v >> (m_depth-8) ;
}

inline
bool Gv::CaptureBufferComponent::is_simple() const
{
	return m_simple && m_x_shift == 0 && m_y_shift == 0 && m_step == 3 ;
}

inline
bool Gv::CaptureBufferComponent::is_simple( size_t offset ) const
{
	return is_simple() && m_offset == offset ;
}


inline
const unsigned char * Gv::CaptureBuffer::begin() const
{
	return reinterpret_cast<const unsigned char *>(m_start) ;
}

inline
unsigned char * Gv::CaptureBuffer::begin()
{
	return reinterpret_cast<unsigned char *>(m_start) ;
}

inline
const unsigned char * Gv::CaptureBuffer::end() const
{
	return begin() + size() ;
}

inline
size_t Gv::CaptureBuffer::size() const
{
	return m_length ;
}

inline
size_t Gv::CaptureBuffer::size4( size_t n )
{
	return n < 4U ? 0U : (n & ~size_t(3U)) ;
}

inline
Gv::CaptureBufferIterator Gv::CaptureBuffer::row( int y ) const
{
	return CaptureBufferIterator( *this , 
		offset_imp(0,y) , m_format->component(0).xshift() , m_format->component(0).step() ,
		offset_imp(1,y) , m_format->component(1).xshift() , m_format->component(1).step() ,
		offset_imp(2,y) , m_format->component(2).xshift() , m_format->component(2).step() ) ;
}

inline
size_t Gv::CaptureBuffer::offset_imp( int c , int y ) const
{
	return m_format->component(c).offset( y , m_scale ) ;
}

inline
size_t Gv::CaptureBuffer::offset_imp( int c , int x , int y ) const
{
	size_t result = m_format->component(c).offset( x , y , m_scale ) ;
	G_ASSERT( result < size() ) ;
	return result ;
}

inline
unsigned int Gv::CaptureBuffer::value( int c , int x , int y ) const
{
	return value( c , offset_imp(c,x,y) ) ;
}

inline
unsigned int Gv::CaptureBuffer::value( int c , size_t offset ) const
{
	return m_format->component(c).value( begin() + offset ) ;
}

inline
unsigned char Gv::CaptureBuffer::value8( int c , int x , int y ) const
{
	return value8( c , offset_imp(c,x,y) ) ;
}

inline
unsigned char Gv::CaptureBuffer::value8( int c , size_t offset ) const
{
	return m_format->component(c).value8( begin() + offset ) ;
}

inline
unsigned char Gv::CaptureBuffer::luma_imp( size_t offset_0 , size_t offset_1 , size_t offset_2 ) const
{
	G_ASSERT( m_format != nullptr ) ;
	typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
	if( m_format->is_grey() || m_format->is_yuv() )
	{
		return value8( 0 , offset_0 ) ;
	}
	else
	{
		return Gr::ColourSpace::y_int( value8(0,offset_0) , value8(1,offset_1) , value8(2,offset_2) ) ;
	}
}

inline
Gr::ColourSpace::triple<unsigned char> Gv::CaptureBuffer::rgb_imp( size_t offset_0 , size_t offset_1 , size_t offset_2 ) const
{
	G_ASSERT( m_format != nullptr ) ;
	typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
	if( m_format->is_yuv() )
	{
		return Gr::ColourSpace::rgb_int( triple_t( value8(0,offset_0) , value8(1,offset_1) , value8(2,offset_2) ) ) ;
	}
	else if( m_format->is_rgb() )
	{
		return triple_t( value8(0,offset_0) , value8(1,offset_1) , value8(2,offset_2) ) ;
	}
	else // m_format->is_grey()
	{
		const unsigned char luma = value8( 0 , offset_0 ) ;
		return triple_t( luma , luma , luma ) ;
	}
}


inline
Gv::CaptureBufferIterator::CaptureBufferIterator( const CaptureBuffer & buffer ,
	size_t offset_0 , unsigned short x_shift_0 , size_t step_0 ,
	size_t offset_1 , unsigned short x_shift_1 , size_t step_1 ,
	size_t offset_2 , unsigned short x_shift_2 , size_t step_2 ) :
		m_buffer(buffer) ,
		m_x(0U)
{
	m_offset[0] = offset_0 ;
	m_offset[1] = offset_1 ;
	m_offset[2] = offset_2 ;
	m_x_shift_mask[0] = ((1U<<x_shift_0)-1U) ;
	m_x_shift_mask[1] = ((1U<<x_shift_1)-1U) ;
	m_x_shift_mask[2] = ((1U<<x_shift_2)-1U) ;
	m_step[0] = step_0 ;
	m_step[1] = step_1 ;
	m_step[2] = step_2 ;
}

inline
Gr::ColourSpace::triple<unsigned char> Gv::CaptureBufferIterator::operator*() const
{
	return m_buffer.rgb_imp( m_offset[0] , m_offset[1] , m_offset[2] ) ;
}

inline
Gr::ColourSpace::triple<unsigned char> Gv::CaptureBufferIterator::rgb() const
{
	return m_buffer.rgb_imp( m_offset[0] , m_offset[1] , m_offset[2] ) ;
}

inline
unsigned char Gv::CaptureBufferIterator::luma() const
{
	return m_buffer.luma_imp( m_offset[0] , m_offset[1] , m_offset[2] ) ;
}

inline
Gv::CaptureBufferIterator & Gv::CaptureBufferIterator::operator++()
{
	m_x++ ;
	if( (m_x & m_x_shift_mask[0]) == 0 ) m_offset[0] += m_step[0] ;
	if( (m_x & m_x_shift_mask[1]) == 0 ) m_offset[1] += m_step[1] ;
	if( (m_x & m_x_shift_mask[2]) == 0 ) m_offset[2] += m_step[2] ;
	return *this ;
}

inline
bool Gv::CaptureBufferIterator::operator==( const CaptureBufferIterator & other ) const
{
	return m_x == other.m_x ;
}

inline
bool Gv::CaptureBufferIterator::operator!=( const CaptureBufferIterator & other ) const
{
	return !(*this == other) ;
}


inline
bool Gv::CaptureBufferFormat::is_grey() const
{
	return m_type == grey ;
}

inline
bool Gv::CaptureBufferFormat::is_rgb() const
{
	return m_type == rgb ;
}

inline
bool Gv::CaptureBufferFormat::is_yuv() const
{
	return m_type == yuv ;
}

inline
const Gv::CaptureBufferComponent & Gv::CaptureBufferFormat::component( int c ) const
{
	G_ASSERT( c >= 0 && c < 3 ) ;
	return m_c[c] ;
}

inline
unsigned int Gv::CaptureBufferFormat::id() const
{
	return m_id ;
}

inline
const char * Gv::CaptureBufferFormat::name() const
{
	return m_name ;
}

inline
Gv::CaptureBufferFormat::Type Gv::CaptureBufferFormat::type() const
{
	return m_type ;
}

#endif

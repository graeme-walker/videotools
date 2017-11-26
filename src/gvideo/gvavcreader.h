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
/// \file gvavcreader.h
///

#ifndef GV_AVC_READER__H
#define GV_AVC_READER__H

#include "gdef.h"
#include "gravc.h"
#include "grcolourspace.h"
#include "grimagedata.h"
#include "grimagetype.h"

namespace Gv
{
	class AvcReaderStream ;
	class AvcReaderStreamImp ;
	class AvcReader ;
}

/// \class Gv::AvcReaderStream
/// Holds state for an AVC (aka H.264) decoder. Each image is decoded 
/// separately by Gr::AvcReader, but state for a sequence of images is
/// managed by this class.
/// 
class Gv::AvcReaderStream
{
public:
	AvcReaderStream() ;
		///< Default constructor. Since no configuration is supplied
		///< the decoding will only work once the the required SPS 
		///< and PPS NALUs have been supplied.

	explicit AvcReaderStream( const Gr::Avc::Configuration & avcc ) ;
		///< Constructor taking a configuration object.

	~AvcReaderStream() ;
		///< Destructor.

	int dx() const ;
		///< Returns the image width as indicated by the first SPS structure,
		///< or zero if default constructed.

	int dy() const ;
		///< Returns the image height as indicated by the first SPS structure,
		///< or zero if default constructed.

private:
	AvcReaderStream( const AvcReaderStream & ) ;
	void operator=( const AvcReaderStream & ) ;

private:
	friend class AvcReader ;
	AvcReaderStreamImp * m_imp ;
} ;

/// \class Gv::AvcReader
/// A decoder for an AVC (aka  H.264) video packet.
/// 
/// The class name alludes to the Gr::JpegReader and Gr::PngReader classes,
/// which have a similar interface.
/// 
class Gv::AvcReader
{
public:
	AvcReader( AvcReaderStream & stream , const char * p , size_t n ) ;
		///< Constructor taking a complete NALU data buffer, including
		///< a leading four-byte 00-00-00-01 start code.

	AvcReader( AvcReaderStream & stream , const unsigned char * p , size_t n ) ;
		///< Constructor overload for unsigned char.

	static bool available() ;
		///< Returns true if the decoder library is built in.

	bool valid() const ;
		///< Returns true if a picture was decoded successfully.

	int dx() const ;
		///< Returns the image width.

	int dy() const ;
		///< Returns the image height.

	bool keyframe() const ;
		///< Returns true if a key frame.

	unsigned char r( int x , int y ) const ;
		///< Returns a pixel red value.

	unsigned char g( int x , int y ) const ;
		///< Returns a pixel green value.

	unsigned char b( int x , int y ) const ;
		///< Returns a pixel blue value.

	unsigned char luma( int x , int y ) const ;
		///< Returns a pixel luma value.

	unsigned int rgb( int x , int y ) const ;
        ///< Returns the three rgb values packed into one integer.

	Gr::ImageType fill( std::vector<char> & , int scale = 1 , bool monochrome = false ) ;
		///< Fills the supplied buffer with RGB or greyscale image data 
		///< and returns the raw image type.

	bool simple() const ;
		///< Returns true if the data format is simple enough for the
		///< optimised iterator, Gv::AvcReader::SimpleIterator.

private:
	friend struct Iterator ;
	friend struct SimpleIterator ;
	AvcReader( const AvcReader & ) ;
	void operator=( const AvcReader & ) ;
	void init( const unsigned char * p , size_t n ) ;
	unsigned char value( int c , int x , int y ) const ;
	unsigned char value( int c , size_t offset ) const ;
	unsigned int fullvalue( int c , int x , int y ) const ;
	unsigned int fullvalue( int c , size_t offset ) const ;
	size_t offset( int c , int x , int y ) const  ;

public:
	struct Component /// Describes one plane of a Gv::AvcReader image, and points to its data.
	{
		const unsigned char * data ;
		size_t offset ; // fixed offset in data
		size_t linesize ; // offset for each new line
		size_t step ; // offset for next horizontal pixel
		unsigned short depth ; // number of bits, eg. 8
		unsigned int mask ; // mask for 'depth' bits
		unsigned short shift ; // right-shift to get the value
		bool eightbit ; // bytes rather than words
		unsigned short x_shift ; // chroma bit-shift for x (also Data.m_x_shift)
		unsigned short y_shift ; // chroma bit-shift for y (also Data.m_y_shift)
		//
		unsigned int fullvalue( const unsigned char * p , bool be ) const ;
		unsigned char value( const unsigned char * p , bool be ) const ;
		unsigned char simplevalue( const unsigned char * p ) const ;
		size_t rowoffset( int y ) const ;
		size_t xoffset( int x ) const ;
		size_t simplexoffset( int x ) const ;
		bool simple() const ;
	} ;
	struct Data /// Describes a Gv::AvcReader image, and points to its data via up to four Components.
	{
		int m_dx ;
		int m_dy ;
		bool m_rgb ;
		bool m_big_endian ;
		unsigned short m_x_shift ; // chroma bit-shift for x
		unsigned short m_y_shift ; // chroma bit-shift for y
		Component m_component[4] ;
	} ;
	struct Iterator /// A row iterator for Gv::AvcReader providing r/g/b or y/u/v across a row.
	{
		Iterator( const AvcReader & , int y , int scale ) ;
		unsigned char value0() const ;
		unsigned char value1() const ;
		unsigned char value2() const ;
		unsigned char r() const ;
		unsigned char g() const ;
		unsigned char b() const ;
		unsigned char luma() const ;
		void operator++() ;
		//
		int m_x ;
		int m_y ;
		int m_scale ;
		const Component & m_c0 ;
		const Component & m_c1 ;
		const Component & m_c2 ;
		const unsigned char * m_rp0 ;
		const unsigned char * m_rp1 ;
		const unsigned char * m_rp2 ;
		bool m_data_rgb ;
		size_t m_data_big_endian ;
	} ;
	struct SimpleIterator /// An optimised row iterator for Gv::AvcReader when simple().
	{
		SimpleIterator( const AvcReader & , int y , int scale ) ;
		unsigned char value0() const ;
		unsigned char value1() const ;
		unsigned char value2() const ;
		unsigned char r() const ;
		unsigned char g() const ;
		unsigned char b() const ;
		unsigned char luma() const ;
		void operator++() ;
		//
		int m_x ;
		int m_y ;
		int m_scale ;
		const Component & m_c0 ;
		const Component & m_c1 ;
		const Component & m_c2 ;
		const unsigned char * m_rp0 ;
		const unsigned char * m_rp1 ;
		const unsigned char * m_rp2 ;
		bool m_data_rgb ;
	} ;

private:
	AvcReaderStream & m_stream ;
	Data m_data ;
} ;

inline
Gv::AvcReader::AvcReader( AvcReaderStream & stream , const char * p , size_t n ) :
	m_stream(stream)
{ 
	init( reinterpret_cast<const unsigned char*>(p) , n ) ; 
}

inline
Gv::AvcReader::AvcReader( AvcReaderStream & stream , const unsigned char * p , size_t n ) :
	m_stream(stream)
{ 
	init( p , n ) ; 
}

inline
bool Gv::AvcReader::valid() const
{
	return 
		m_data.m_component[0].data != nullptr &&
		m_data.m_component[1].data != nullptr &&
		m_data.m_component[2].data != nullptr ;
}

inline
bool Gv::AvcReader::simple() const
{
	return 
		m_data.m_component[0].simple() &&
		m_data.m_component[1].simple() &&
		m_data.m_component[2].simple() ;
}

inline
int Gv::AvcReader::dx() const 
{ 
	return m_data.m_dx ; 
}

inline
int Gv::AvcReader::dy() const 
{ 
	return m_data.m_dy ; 
}

inline
unsigned char Gv::AvcReader::luma( int x , int y ) const 
{
	return m_data.m_rgb ? Gr::ColourSpace::y_int( value(0,x,y) , value(1,x,y) , value(2,x,y) ) : value(0,x,y) ;
}

inline
unsigned int Gv::AvcReader::fullvalue( int c , size_t offset ) const 
{
	// cf. Gv::CaptureBuffer::value()
	const Component & comp = m_data.m_component[c] ;
	return comp.fullvalue( comp.data+offset , m_data.m_big_endian ) ;
}

inline
unsigned int Gv::AvcReader::Component::fullvalue( const unsigned char * p , bool big_endian ) const
{
	unsigned int v = *p ;
	if( !eightbit )
	{
		unsigned int v2 = p[1] ;
		v = big_endian ? ((v<<8)|v2) : ((v2<<8)|v) ;
	}
	return ( v >> shift ) & mask ;
}

inline
unsigned char Gv::AvcReader::Component::value( const unsigned char * p , bool big_endian ) const
{
	unsigned int v = fullvalue( p , big_endian ) ;
	if( depth <= 8U )
		v <<= (8U-depth) ;
	else
		v >>= (depth-8U) ;
	return v ;
}

inline
bool Gv::AvcReader::Component::simple() const
{
	return eightbit && shift == 0 && depth == 8 && step == 1 && ( x_shift == 0 || x_shift == 1 ) ;
}

inline
unsigned char Gv::AvcReader::Component::simplevalue( const unsigned char * p ) const
{
	//G_ASSERT( eightbit && shift==0 && mask=0xff && depth==8 ) ;
	return *p ;
}

inline
size_t Gv::AvcReader::Component::rowoffset( int y ) const
{
	size_t yy = static_cast<size_t>(y) ;
	yy >>= y_shift ;
	return yy*linesize + offset ;
}

inline
size_t Gv::AvcReader::Component::xoffset( int x ) const
{
	size_t xx = static_cast<size_t>(x) ;
	xx >>= x_shift ;
	if( step <= 8 )
	{
		switch( step )
		{
			case 8: xx += xx ;
			case 7: xx += xx ;
			case 6: xx += xx ;
			case 5: xx += xx ;
			case 4: xx += xx ;
			case 3: xx += xx ;
			case 2: xx += xx ;
		}
		return xx ;
	}
	else
	{
		return xx*step ;
	}
}

inline
size_t Gv::AvcReader::Component::simplexoffset( int x ) const
{
	//G_ASSERT( step == 1 ) ;
	//G_ASSERT( x_shift == 0 || x_shift == 1 ) ;
	return x_shift == 0 ? static_cast<size_t>(x) : static_cast<size_t>(x/2) ;
}

inline
size_t Gv::AvcReader::offset( int c , int x , int y ) const 
{
	const Component & comp = m_data.m_component[c] ;
	return comp.rowoffset(y) + comp.xoffset(x) ;
}

inline
unsigned int Gv::AvcReader::fullvalue( int c , int x , int y ) const 
{ 
	return fullvalue( c , offset(c,x,y) ) ;
}

inline
unsigned char Gv::AvcReader::value( int c , size_t offset ) const 
{
	const Component & comp = m_data.m_component[c] ;
	return comp.value( comp.data+offset , m_data.m_big_endian ) ;
}

inline
unsigned char Gv::AvcReader::value( int c , int x , int y ) const 
{
	const Component & comp = m_data.m_component[c] ;
	unsigned int v = fullvalue( c , x , y ) ;
	if( comp.depth <= 8U )
		v <<= (8U-comp.depth) ;
	else
		v >>= (comp.depth-8U) ;
	return v ;
}

inline
unsigned char Gv::AvcReader::r( int x , int y ) const 
{ 
	return m_data.m_rgb ? value(0,x,y) : Gr::ColourSpace::r_int( value(0,x,y) , value(1,x,y) , value(2,x,y) ) ;
}

inline
unsigned char Gv::AvcReader::g( int x , int y ) const 
{ 
	return m_data.m_rgb ? value(1,x,y) : Gr::ColourSpace::g_int( value(0,x,y) , value(1,x,y) , value(2,x,y) ) ;
}

inline
unsigned char Gv::AvcReader::b( int x , int y ) const 
{ 
	return m_data.m_rgb ? value(2,x,y) : Gr::ColourSpace::b_int( value(0,x,y) , value(1,x,y) , value(2,x,y) ) ;
}

inline
unsigned int Gv::AvcReader::rgb( int x , int y ) const
{
	return
		( static_cast<unsigned int>(r(x,y)) << 16 ) |
		( static_cast<unsigned int>(g(x,y)) << 8 ) |
		( static_cast<unsigned int>(b(x,y)) ) ;
}

inline
Gv::AvcReader::Iterator::Iterator( const AvcReader & reader , int y , int scale ) :
	m_x(0) ,
	m_y(y) ,
	m_scale(scale) ,
	m_c0(reader.m_data.m_component[0]) ,
	m_c1(reader.m_data.m_component[1]) ,
	m_c2(reader.m_data.m_component[2]) ,
	m_rp0(m_c0.data+m_c0.rowoffset(y)) ,
	m_rp1(m_c1.data+m_c1.rowoffset(y)) ,
	m_rp2(m_c2.data+m_c2.rowoffset(y)) ,
	m_data_rgb(reader.m_data.m_rgb) ,
	m_data_big_endian(reader.m_data.m_big_endian)
{
}

inline
Gv::AvcReader::SimpleIterator::SimpleIterator( const AvcReader & reader , int y , int scale ) :
	m_x(0) ,
	m_y(y) ,
	m_scale(scale) ,
	m_c0(reader.m_data.m_component[0]) ,
	m_c1(reader.m_data.m_component[1]) ,
	m_c2(reader.m_data.m_component[2]) ,
	m_rp0(m_c0.data+m_c0.rowoffset(y)) ,
	m_rp1(m_c1.data+m_c1.rowoffset(y)) ,
	m_rp2(m_c2.data+m_c2.rowoffset(y)) ,
	m_data_rgb(reader.m_data.m_rgb)
{
}

inline
unsigned char Gv::AvcReader::Iterator::value0() const
{
	return m_c0.value( m_rp0+m_c0.xoffset(m_x) , m_data_big_endian ) ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::value0() const
{
	return m_c0.simplevalue( m_rp0+m_c0.simplexoffset(m_x) ) ;
}

inline
unsigned char Gv::AvcReader::Iterator::value1() const
{
	return m_c1.value( m_rp1+m_c1.xoffset(m_x) , m_data_big_endian ) ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::value1() const
{
	return m_c1.simplevalue( m_rp1+m_c1.simplexoffset(m_x) ) ;
}

inline
unsigned char Gv::AvcReader::Iterator::value2() const
{
	return m_c2.value( m_rp2+m_c2.xoffset(m_x) , m_data_big_endian ) ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::value2() const
{
	return m_c2.simplevalue( m_rp2+m_c2.simplexoffset(m_x) ) ;
}

inline
unsigned char Gv::AvcReader::Iterator::r() const
{
	return m_data_rgb ? value0() : Gr::ColourSpace::r_int( value0() , value1() , value2() ) ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::r() const
{
	return m_data_rgb ? value0() : Gr::ColourSpace::r_int( value0() , value1() , value2() ) ;
}

inline
unsigned char Gv::AvcReader::Iterator::g() const
{
	return m_data_rgb ? value1() : Gr::ColourSpace::g_int( value0() , value1() , value2() ) ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::g() const
{
	return m_data_rgb ? value1() : Gr::ColourSpace::g_int( value0() , value1() , value2() ) ;
}

inline
unsigned char Gv::AvcReader::Iterator::b() const
{
	return m_data_rgb ? value2() : Gr::ColourSpace::b_int( value0() , value1() , value2() ) ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::b() const
{
	return m_data_rgb ? value2() : Gr::ColourSpace::b_int( value0() , value1() , value2() ) ;
}

inline
unsigned char Gv::AvcReader::Iterator::luma() const
{
	return m_data_rgb ? Gr::ColourSpace::y_int( value0() , value1() , value2() ) : value0() ;
}

inline
unsigned char Gv::AvcReader::SimpleIterator::luma() const
{
	return m_data_rgb ? Gr::ColourSpace::y_int( value0() , value1() , value2() ) : value0() ;
}

inline
void Gv::AvcReader::Iterator::operator++()
{
	m_x += m_scale ;
}

inline
void Gv::AvcReader::SimpleIterator::operator++()
{
	m_x += m_scale ;
}

#endif

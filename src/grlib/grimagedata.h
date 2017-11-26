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
/// \file grimagedata.h
///

#ifndef GR_IMAGE_DATA__H
#define GR_IMAGE_DATA__H

#include "gdef.h"
#include "grdef.h"
#include "grcolour.h"
#include "grimagebuffer.h"
#include "gexception.h"
#include <vector>

namespace Gr
{
	class ImageData ;
	class ImageDataWrapper ;
	class ImageDataWriter ;
}

/// \class Gr::ImageData
/// A holder for image data, having eight bits per sample and one or three
/// channels. The data can be either contiguous or allocated row by row.
/// 
/// This class is primarily intended as an output target for the various
/// decoders, and a data source for their encoders. See also Gr::JpegReader,
/// Gr::PngReader, Gr::PnmReader.
/// 
class Gr::ImageData
{
public:
	G_EXCEPTION( Error , "image data error" ) ;
	enum Type // a type-safe boolean indicating contiguous or row-by-row storage
		{ Segmented , Contiguous } ;

	explicit ImageData( Type = Segmented ) ;
		///< Default constructor for a zero-size image. After contstruction the
		///< object should normally be resize()d. The Contiguous/Segmented type
		///< does not change across resize(). A Contiguous object has a usable 
		///< p() method that returns a single pointer to the whole image.

	ImageData( ImageBuffer & image_buffer , Type = Segmented ) ;
		///< Constructor for an empty ImageData object that wraps an empty 
		///< ImageBuffer.
		///< Precondition: image_buffer.empty()

	ImageData( ImageBuffer & , int dx , int dy , int channels ) ;
		///< Constructor for an ImageData object that wraps the given ImageBuffer.
		///< The image buffer can be empty, but its dimensions must always match 
		///< the other parameters. This ImageData object type() will be contiguous 
		///< or segmented to match the supplied image buffer, or contiguous by 
		///< default if dy is less than two.

	~ImageData() ;
		///< Destructor.

	Type type() const ;
		///< Returns the contiguous/segmented enumeration. Contiguous objects
		///< have a usable p() method that returns a pointer to the whole image.

	int dx() const ;
		///< Returns the width.

	int dy() const ;
		///< Returns the height.

	int channels() const ;
		///< Returns the number of channels (zero, one or three).

	bool empty() const ;
		///< Returns true if the size() is zero.

	size_t size() const ;
		///< Returns the total image size, ie. dx()*dy()*channels().

	size_t rowsize() const ;
		///< Returns the row size, ie. dx()*channels().

	void resize( int dx , int dy , int channels ) ;
		///< Resizes the image data.

	unsigned char r( int x , int y ) const ;
		///< Returns the R-value for a point.

	unsigned char g( int x , int y ) const ;
		///< Returns the G-value for a point. Returns the R-value if there is only one channel.

	unsigned char b( int x , int y ) const ;
		///< Returns the B-value for a point. Returns the R-value if there is only one channel.

	unsigned int rgb( int x , int y ) const ;
		///< Returns the colour of a pixel as rgb values packed into one integer.

	void rgb( int x , int y , unsigned char r , unsigned char g , unsigned char b ) ;
		///< Sets a pixel colour. The trailing 'g' and 'b' values are ignored if there is
		///< only one channel.

	void fill( unsigned char r , unsigned char g , unsigned char b ) ;
		///< Fills the image with a solid colour as if calling rgb() for every pixel.

	ImageDataWriter writer( int x , int y , Colour foreground , Colour background ,
		bool draw_background , bool wrap_on_nl ) ;
			///< Returns a functor that calls write().

	void write( char c , int x , int y , Colour foreground , Colour background ,
		bool draw_background ) ;
			///< Draws a latin-1 character into the image at the given position.

	unsigned char * row( int y ) ;
		///< Returns a pointer to the y'th row.

	const unsigned char * row( int y ) const ;
		///< Returns a const pointer to the y'th row.

	unsigned char ** rowPointers() ;
		///< Returns a pointer to the array of row pointers.

	const unsigned char * const * rowPointers() const ;
		///< Returns a pointer to the array of const row pointers.

	char ** rowPointers( int ) ;
		///< An overload returning a char pointer array.

	const unsigned char * p() const ;
		///< Returns a const pointer to the image data, but throws if the data is not
		///< contiguous.

	unsigned char * p() ;
		///< Returns a pointer to the image data, but throws if the data is not
		///< contiguous.

	void copyRowIn( int y , const unsigned char * row_buffer_in , size_t row_buffer_in_size , int channels_in ,
		bool use_colourspace = false , int scale = 1 ) ;
			///< Sets a row of pixels by copying, with channel-count adjustment and
			///< optional scaling.
			///< 
			///< If the number of channels is reduced then there is the option to
			///< either take the first channel or do a colourspace transform.
			///< Prefer a direct memcpy() to row() for speed where possible.

	void copyIn( const char * data_in , size_t data_size_in , int dx_in , int dy_in , int channels_in ,
		bool use_colourspace = false , int scale = 1 ) ;
			///< Copies the image in from a raw buffer, with channel-count adjustment
			///< and optional scaling, but no resize(). The 'monochrome-out' parameter
			///< is implicit in the current number of channels and the number of channels
			///< passed in.
			///< 
			///< If the number of channels is reduced then there is the option to
			///< just take the first channel or do a colourspace transform.
			///< Prefer a direct memcpy()s to row()s for speed where possible.

	void copyIn( const ImageBuffer & data_in , int dx_in , int dy_in , int channels_in ,
		bool use_colourspace = false , int scale = 1 ) ;
			///< Overload that copies from an ImageBuffer. The dx and dy parameters are used 
			///< as a sanity check against the dimensions of the ImageBuffer.

	void copyRowOut( int y , std::vector<char> & out , int scale = 1 , bool monochrome_out = false , bool use_colourspace = true ) const ;
		///< Copies a row into the given output buffer, resizing the output vector
		///< as necessary.

	void copyTo( std::vector<char> & out ) const ;
		///< Copies the image to the given output buffer, resizing the output vector
		///< as necessary.

	void copyTo( ImageBuffer & out ) const ;
		///< Copies the image to the given ImageBuffer, resizing the output
		///< as necessary.

	void scale( int factor , bool monochrome , bool use_colourspace ) ;
		///< Scales-down the image by sub-sampling.

	void dim( unsigned int shift ) ;
		///< Dims the image by right-shifting all pixel values.

	void dim( unsigned int numerator , unsigned int denominator ) ;
		///< Dims the image by multiplying all pixel values by the given fraction.
		///< Use small numbers to avoid arithmetic overflow.

	void mix( const ImageData & _1 , const ImageData & _2 , unsigned int numerator_1 , unsigned int numerator_2 , unsigned int denominator ) ;
		///< Creates a mixed image from two equally-shaped sources images.
		///< There is no colourspace subtelty; mixing is done separately 
		///< for each channel so arithmetic overflow will create colour 
		///< distortion.

	void add( const ImageData & other ) ;
		///< Adds the given image data to this.

	void subtract( const ImageData & other ) ;
		///< Subtracts the given image data from this.

	void crop( int dx , int dy ) ;
		///< Crops the image so that it fits inside the given dimensions.
		///< Does nothing if the image is already small enough.

	void expand( int dx , int dy ) ;
		///< Expands the image so that the new image has the dimensions given,
		///< with the original image centered inside using black borders.
		///< Does nothing if the image is already big enough.

private:
	ImageData( const ImageData & ) ;
	void operator=( const ImageData & ) ;
	void copyRowInImp( unsigned char * , const unsigned char * , int , bool , int ) ;
	unsigned char * storerow( int y ) ;
	const unsigned char * storerow( int y ) const ;
	void setRows() const ;
	bool valid() const ;
	bool contiguous() const ;
	template <typename Fn> void modify( Fn fn ) ;
	template <typename Fn> void modify( const Gr::ImageData & other , Fn fn ) ;
	template <typename Fn> void modify( const Gr::ImageData & , const Gr::ImageData & , Fn fn ) ;

private:
	const Type m_type ;
	int m_dx ;
	int m_dy ;
	int m_channels ;
	ImageBuffer m_data_store ;
	ImageBuffer & m_data ;
	mutable bool m_rows_set ;
	mutable std::vector<unsigned char*> m_rows ;
} ;

/// \class Gr::ImageDataWrapper
/// A class that wraps a read-only image buffer with a few methods that are
/// similar to those of Gr::ImageData.
/// 
class Gr::ImageDataWrapper
{
public:
	ImageDataWrapper( const ImageBuffer & , int dx , int dy , int channels ) ;
		///< Constructor.

	int dx() const ;
		///< Returns the width.

	int dy() const ;
		///< Returns the height.

	int channels() const ;
		///< Returns the number of channels.

	const unsigned char * row( int y ) const ;
		///< Returns a pointer to the y-th row.

	unsigned char r( int x , int y ) const ;
		///< Returns the R-value for a point.

	unsigned char g( int x , int y ) const ;
		///< Returns the G-value for a point. Returns the R-value if there is only one channel.

	unsigned char b( int x , int y ) const ;
		///< Returns the B-value for a point. Returns the R-value if there is only one channel.

	unsigned int rgb( int x , int y ) const ;
		///< Returns the colour of a pixel as rgb values packed into one integer.

private:
	ImageDataWrapper( const ImageDataWrapper & ) ;
	void operator=( const ImageDataWrapper & ) ;

private:
	const ImageBuffer & m_image_buffer ;
	int m_dx ;
	int m_dy ;
	int m_channels ;
} ;

class Gr::ImageDataWriter
{
public:
	ImageDataWriter( Gr::ImageData & data , int x , int y , 
		Gr::Colour foreground , Gr::Colour background , 
		bool draw_background , bool wrap_on_nl ) ;
			///< Constructor.

	template <typename T> void write( T p , T end ) ;
		///< Calls ImageData::write() for each character in the range.

private:
	bool pre( char ) ;
	void post( char ) ;

private:
	Gr::ImageData * m_data ;
	int m_dx ;
	int m_dy ;
	int m_x0 ;
	int m_x ;
	int m_y ;
	Gr::Colour m_foreground ;
	Gr::Colour m_background ;
	bool m_draw_background ;
	bool m_wrap_on_nl ;
} ;

template <typename T>
void Gr::ImageDataWriter::write( T p , T end )
{
	for( ; p != end ; ++p )
	{
		if( pre(*p) )
		{
			m_data->write( *p , m_x , m_y , m_foreground , m_background , m_draw_background ) ;
			post( *p ) ;
		}
	}
}


inline
int Gr::ImageData::dx() const
{
	return m_dx ;
}

inline
int Gr::ImageData::dy() const
{
	return m_dy ;
}

inline
int Gr::ImageData::channels() const
{
	return m_channels ;
}

inline
unsigned char * Gr::ImageData::storerow( int y )
{
	return reinterpret_cast<unsigned char*>( &(m_data[y])[0] ) ;
}

inline
const unsigned char * Gr::ImageData::storerow( int y ) const
{
	return reinterpret_cast<const unsigned char*>( &(m_data[y])[0] ) ;
}

inline
const unsigned char * Gr::ImageData::row( int y ) const
{
	return
		m_type == Contiguous ?
			( storerow(0) + sizet(y,m_dx,m_channels) ) :
			storerow( y ) ;
}

inline
unsigned char * Gr::ImageData::row( int y )
{
	return
		m_type == Contiguous ?
			( storerow(0) + sizet(y,m_dx,m_channels) ) :
			storerow( y ) ;
}

inline
void Gr::ImageData::rgb( int x , int y , unsigned char r , unsigned char g , unsigned char b )
{
	int x_offset = m_channels == 1 ? x : (x*3) ;
	unsigned char * out_p = row(y) + x_offset ;
	*out_p++ = r ;
	if( m_channels != 1 )
	{
		*out_p++ = g ;
		*out_p++ = b ;
	}
}

inline
unsigned char Gr::ImageData::r( int x , int y ) const
{
	const unsigned char * row_p = m_rows_set ? m_rows[y] : row(y) ;
	return m_channels == 1 ? row_p[x] : row_p[x*3] ;
}

inline
unsigned char Gr::ImageData::g( int x , int y ) const
{
	const unsigned char * row_p = m_rows_set ? m_rows[y] : row(y) ;
	return m_channels == 1 ? row_p[x] : row_p[x*3+1] ;
}

inline
unsigned char Gr::ImageData::b( int x , int y ) const
{
	const unsigned char * row_p = m_rows_set ? m_rows[y] : row(y) ;
	return m_channels == 1 ? row_p[x] : row_p[x*3+2] ;
}

inline
unsigned int Gr::ImageData::rgb( int x , int y ) const
{
	return
		( static_cast<unsigned int>(r(x,y)) << 16 ) |
		( static_cast<unsigned int>(g(x,y)) << 8 ) |
		( static_cast<unsigned int>(r(x,y)) << 0 ) ;
}


inline
Gr::ImageDataWrapper::ImageDataWrapper( const ImageBuffer & image_buffer , int dx , int dy , int channels ) :
	m_image_buffer(image_buffer) ,
	m_dx(dx) ,
	m_dy(dy) ,
	m_channels(channels)
{
	if( dx <= 0 || dy <= 0 || (channels != 1 && channels != 3) ||
		( image_buffer.size() != 1U && image_buffer.size() != sizet(dy) ) ||
		imagebuffer::size_of(image_buffer) != sizet(dx,dy,channels) )
			throw ImageData::Error() ;
}

inline
int Gr::ImageDataWrapper::dx() const
{
	return m_dx ;
}

inline
int Gr::ImageDataWrapper::dy() const
{
	return m_dy ;
}

inline
int Gr::ImageDataWrapper::channels() const
{
	return m_channels ;
}

inline
const unsigned char * Gr::ImageDataWrapper::row( int y ) const
{
	if( m_image_buffer.size() == 1U && y > 0 && y < m_dy )
		return reinterpret_cast<const unsigned char*>(&(m_image_buffer.at(0))[0]) + sizet(y,m_dx,m_channels) ;
	else
		return reinterpret_cast<const unsigned char*>(&(m_image_buffer.at(y))[0]) ;
}

inline
unsigned char Gr::ImageDataWrapper::r( int x , int y ) const
{
	const unsigned char * row_p = row( y ) ;
	return m_channels == 1 ? row_p[x] : row_p[x*3] ;
}

inline
unsigned char Gr::ImageDataWrapper::g( int x , int y ) const
{
	const unsigned char * row_p = row( y ) ;
	return m_channels == 1 ? row_p[x] : row_p[x*3+1] ;
}

inline
unsigned char Gr::ImageDataWrapper::b( int x , int y ) const
{
	const unsigned char * row_p = row( y ) ;
	return m_channels == 1 ? row_p[x] : row_p[x*3+2] ;
}

inline
unsigned int Gr::ImageDataWrapper::rgb( int x , int y ) const
{
	return
		( static_cast<unsigned int>(r(x,y)) << 16 ) |
		( static_cast<unsigned int>(g(x,y)) << 8 ) |
		( static_cast<unsigned int>(r(x,y)) << 0 ) ;
}

#endif

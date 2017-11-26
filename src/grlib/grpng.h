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
/// \file grpng.h
///

#ifndef GR_PNG__H
#define GR_PNG__H

#include "gdef.h"
#include "grdef.h"
#include "grimagedata.h"
#include "grimagebuffer.h"
#include "gpath.h"
#include "gexception.h"
#include <istream>
#include <map>
#include <utility>
#include <string>
#include <cstdio>
#include <vector>

namespace Gr
{
	class Png ;
	class PngInfo ;
	class PngReader ;
	class PngWriter ;
	//
	class PngImp ;
	class PngTagsImp ;
	class PngReaderImp ;
	class PngWriterImp ;
}

/// \class Gr::Png
/// A static class providing information about the png library.
/// 
class Gr::Png
{
public:
	G_EXCEPTION( Error , "png error" ) ;

	static bool available() ;
		///< Returns true if the png library is available.

private:
	Png() ;
} ;

/// \class Gr::PngInfo
/// A class that reads a png header in order to provide the image dimensions.
/// The number of channels is not provided because PngReader always transforms to
/// three channels. An implementation of PngInfo is available even without libpng.
/// Png files must have the IHDR as the first chunk, so it is sufficient to supply 
/// only 30 bytes to be correctly parsed.
/// 
class Gr::PngInfo
{
public:
	explicit PngInfo( std::istream & ) ;
		///< Constructor.

	PngInfo( const unsigned char * p , size_t ) ;
		///< Constructor.

	PngInfo( const char * p , size_t ) ;
		///< Constructor overload for char.

	PngInfo( const ImageBuffer & ) ;
		///< Constructor overload for ImageBuffer.

	bool valid() const ;
		///< Returns true if successfully constructed.

	int dx() const ;
		///< Returns the image width. Returns zero on error.

	int dy() const ;
		///< Returns the image height. Returns zero on error.

private:
	PngInfo( const PngInfo & ) ;
	void operator=( const PngInfo & ) ;
	void init( const unsigned char * , size_t ) ;
	void init( std::istream & ) ;
	static std::pair<int,int> parse( const unsigned char * , size_t ) ;

private:
	int m_dx ;
	int m_dy ;
} ;

/// \class Gr::PngReader
/// A read interface for libpng.
/// 
class Gr::PngReader
{
public:
	typedef std::multimap<std::string,std::string> Map ;

	PngReader( int scale = 1 , bool monochrome_out = false ) ;
		///< Constructor.

	~PngReader() ;
		///< Destructor.

	void setup( int scale , bool monochrome_out = false ) ;
		///< Sets the decoding scale factor.

	void decode( ImageData & out , const G::Path & in ) ;
		///< Decodes a png file into an image. Throws on error.

	void decode( ImageData & out , const char * p_in , size_t n ) ;
		///< Decodes a png data buffer into an image. Throws on error.

	void decode( ImageData & out , const unsigned char * p_in , size_t n ) ;
		///< Decodes a png data buffer into an image. Throws on error.

	void decode( ImageData & out , const ImageBuffer & ) ;
		///< Decodes a png image buffer into an image. Throws on error.

	Map tags() const ;
		///< Returns the text tags from the last decode().

private:
	PngReader( const PngReader & ) ;
	void operator=( const PngReader & ) ;

private:
	int m_scale ;
	bool m_monochrome_out ;
	Map m_tags ;
} ;

/// \class Gr::PngWriter
/// An interface for writing a raw dgb image as a png file.
/// 
class Gr::PngWriter
{
public:
	typedef std::multimap<std::string,std::string> Map ;

	struct Output /// Abstract interface for Gr::PngWriter::write().
	{
		virtual void operator()( const unsigned char * , size_t ) = 0 ;
		virtual ~Output() {}
	} ;

	explicit PngWriter( const ImageData & , Map tags = Map() ) ;
		///< Constructor with the raw image data prepared in a ImageData
		///< object. The references are kept.

	~PngWriter() ;
		///< Destructor.

	void write( const G::Path & path ) ;
		///< Writes to file.

	void write( Output & out ) ;
		///< Writes to an Output interface.

private:
	PngWriter( const PngWriter & ) ;
	void operator=( const PngWriter & ) ;

private:
	PngWriterImp * m_imp ;
} ;


inline
Gr::PngInfo::PngInfo( std::istream & stream ) :
	m_dx(0) ,
	m_dy(0)
{
	init( stream ) ;
}

inline
Gr::PngInfo::PngInfo( const unsigned char * p , size_t n ) :
	m_dx(0) ,
	m_dy(0)
{
	init( p , n ) ;
}

inline
Gr::PngInfo::PngInfo( const char * p , size_t n ) :
	m_dx(0) ,
	m_dy(0)
{
	init( reinterpret_cast<const unsigned char *>(p) , n ) ;
}

inline
Gr::PngInfo::PngInfo( const ImageBuffer & image_buffer ) :
	m_dx(0) ,
	m_dy(0)
{
	typedef Gr::traits::imagebuffer<ImageBuffer>::streambuf_type imagebuf ;
	imagebuf buf( image_buffer ) ;
	std::istream in( &buf ) ;
	init( in ) ;
}

inline
bool Gr::PngInfo::valid() const
{
	return m_dx > 0 && m_dy > 0 ;
}

inline
int Gr::PngInfo::dx() const
{
	return m_dx ;
}

inline
int Gr::PngInfo::dy() const
{
	return m_dy ;
}

inline
std::pair<int,int> Gr::PngInfo::parse( const unsigned char * p , size_t n )
{
	unsigned int dx = 0U ;
	unsigned int dy = 0U ;
	if( n > 29U && 
		p[0] == 0x89 && p[1] == 0x50 && p[2] == 0x4e && p[3] == 0x47 && // .PNG
		p[4] == 0x0d && p[5] == 0x0a && p[6] == 0x1a && p[7] == 0x0a &&
		p[12] == 0x49 && p[13] == 0x48 && p[14] == 0x44 && p[15] == 0x52 ) // IHDR
	{
		dx += p[16] ; dx <<= 8 ;
		dx += p[17] ; dx <<= 8 ;
		dx += p[18] ; dx <<= 8 ;
		dx += p[19] ;
		dy += p[20] ; dy <<= 8 ;
		dy += p[21] ; dy <<= 8 ;
		dy += p[22] ; dy <<= 8 ;
		dy += p[23] ;
	}
	return std::make_pair( static_cast<int>(dx) , static_cast<int>(dy) ) ;
}

#endif

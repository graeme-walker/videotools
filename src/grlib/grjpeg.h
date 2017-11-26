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
/// \file grjpeg.h
///

#ifndef GR_JPEG__H
#define GR_JPEG__H

#include "gdef.h"
#include "grdef.h"
#include "grimagedata.h"
#include "grimagebuffer.h"
#include "gpath.h"
#include "gexception.h"
#include <map>
#include <string>
#include <cstdio>
#include <vector>
#include <iostream>
#include <utility>

namespace Gr
{
	typedef unsigned char jpeg_byte ; ///< Equivalent to libjpeg JSAMPLE. A static-assert checks the size.
	class Jpeg ;
	class JpegInfo ;
	class JpegReaderImp ;
	class JpegReader ;
	class JpegWriterImp ;
	class JpegWriter ;
}

/// \class Gr::Jpeg
/// A scoping class for other jpeg classes.
/// 
class Gr::Jpeg
{
public:
	G_EXCEPTION( Error , "jpeg error" ) ;

	static bool available() ;
		///< Returns true if a jpeg library is available.
} ;

/// \class Gr::JpegInfo
/// Provides some basic information about a jpeg image without full decoding.
/// 
class Gr::JpegInfo
{
public:
	JpegInfo( const unsigned char * p , size_t ) ;
		///< Constructor. This requires the whole file contents because
		///< the relevant SOF chunk can be anywhere, so prefer the
		///< istream interface.

	JpegInfo( const char * p , size_t ) ;
		///< Constructor overload for char.

	explicit JpegInfo( std::istream & ) ;
		///< Constructor overload for an istream.

	bool valid() const ;
		///< Returns true if constructed successfully.

	int dx() const ;
		///< Returns the image width.

	int dy() const ;
		///< Returns the image height.

	int channels() const ;
		///< Returns the number of channels (eg. 3).

private:
	JpegInfo( const JpegInfo & ) ;
	void operator=( const JpegInfo & ) ;

private:
	int m_dx ;
	int m_dy ;
	int m_channels ;
} ;

/// \class Gr::JpegReader
/// A read interface for libjpeg.
/// 
class Gr::JpegReader
{
public:
	typedef Jpeg::Error Error ;

	JpegReader( int scale = 1 , bool monochrome_out = false ) ;
		///< Constructor.

	~JpegReader() ;
		///< Destructor.

	void setup( int scale , bool monochrome_out = false ) ;
		///< Sets the decoding scale factor. In practice scale factors of
		///< 1, 2, 4 and 8 are supported directly by the jpeg library,
		///< and others scale factors are applied only after decoding
		///< are full scale.

	void decode( ImageData & out , const G::Path & in ) ;
		///< Decodes a jpeg file into an image. Throws on error.

	void decode( ImageData & out , const char * p_in , size_t n ) ;
		///< Decodes a jpeg buffer into an image. Throws on error.

	void decode( ImageData & out , const unsigned char * p_in , size_t n ) ;
		///< Decodes a jpeg buffer into an image. Throws on error.

	void decode( ImageData & out , const ImageBuffer & ) ;
		///< Decodes a jpeg buffer into an image. Throws on error.

private:
	JpegReader( const JpegReader & ) ;
	void operator=( const JpegReader & ) ;

private:
	unique_ptr<JpegReaderImp> m_imp ;
	int m_scale ;
	bool m_monochrome_out ;
} ;

/// \class Gr::JpegWriter
/// A write interface for libjpeg.
/// 
class Gr::JpegWriter
{
public:
	explicit JpegWriter( int scale = 1 , bool monochrome_out = false ) ;
		///< Constructor.

	~JpegWriter() ;
		///< Destructor.

	void setup( int scale , bool monochrome_out = false ) ;
		///< Sets the encoding scale factor.

	void encode( const ImageData & in , const G::Path & path_out ) ;
		///< Encodes to a file.

	void encode( const ImageData & in , std::vector<char> & out ) ;
		///< Encodes to a buffer. Use an output buffer that is one byte bigger
		///< than the expected output size in order to avoid copying.

	void encode( const ImageData & in , ImageBuffer & out ) ;
		///< Encodes to an image buffer, allocated in reasonably-size chunks.

private:
	JpegWriter( const JpegWriter & ) ;
	void operator=( const JpegWriter & ) ;

private:
	unique_ptr<JpegWriterImp> m_imp ;
} ;

#endif

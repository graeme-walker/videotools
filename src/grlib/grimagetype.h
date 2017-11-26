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
/// \file grimagetype.h
///

#ifndef GR_IMAGE_TYPE__H
#define GR_IMAGE_TYPE__H

#include "gdef.h"
#include "grimagebuffer.h"
#include <string>
#include <vector>
#include <iostream>

namespace Gr
{
	class ImageType ;
}

/// \class Gr::ImageType
/// An encapsulation of image type, including width, height and number of channels, 
/// with support for a string form that encodes the same information. 
/// 
/// The string format is like "image/png;xsize=640x480x3" where the last of the
/// three numbers is the number of channels (1 or 3), and the image type is 
/// "x.raw", "jpeg", "png" or "x-portable-anymap".
/// 
class Gr::ImageType
{
public:
	struct String /// A small-string class used for stringised Gr::ImageType instances.
	{
		String() { s[0] = '\0' ; }
		const char * c_str() const { return s ; }
		std::string str() const ;
		char s[60] ;
	} ;

	ImageType() ;
		///< Default constructor for an in-valid() image type with dimensions
		///< of zero.

	explicit ImageType( std::istream & ) ;
		///< Constructor that examines the image data to determine its type.
		///< The stream position is not advanced. Throws if the stream is
		///< not seekable.

	explicit ImageType( const std::vector<char> & ) ;
		///< Constructor that examines the image data to determine its type.
		///< The image data should be for the complete image, not just the
		///< first section (because jpeg).

	explicit ImageType( const ImageBuffer & ) ;
		///< Constructor overload for ImageBuffer.

	ImageType( const char * p , size_t n ) ;
		///< Constructor overload for a char buffer.

	ImageType( const unsigned char * p , size_t n ) ;
		///< Constructor overload for an unsigned char buffer.

	explicit ImageType( const std::string & ) ;
		///< Constructor taking a stringised image type that includes image size 
		///< information.

	static ImageType jpeg( int dx , int dy , int channels = 3 ) ;
		///< Factory function for a jpeg image type.

	static ImageType jpeg( ImageType , int scale = 1 , bool monochrome_out = false ) ;
		///< Factory function for a jpeg image type with the same dimensions
		///< as the given image type, optionally scaled.

	static ImageType png( int dx , int dy , int channels = 3 ) ;
		///< Factory function for a png image type.

	static ImageType png( ImageType , int scale = 1 , bool monochrome_out = false ) ;
		///< Factory function for a png image type with the same dimensions
		///< as the given image type, optionally scaled.

	static ImageType raw( int dx , int dy , int channels ) ;
		///< Factory function for a raw image type.

	static ImageType raw( ImageType , int scale = 1 , bool monochrome_out = false ) ;
		///< Factory function for a raw image type with the same dimensions
		///< as the given image type, optionally scaled.

	bool valid() const ;
		///< Returns true if valid.

	bool matches( const std::string & str ) const ;
		///< Returns true if this type matches the given type (including size decorations).

	std::string str() const ;
		///< Returns the image type string (including the size parameter).

	String & set( String & out ) const ;
		///< Returns str() by reference.

	std::string simple() const ;
		///< Returns the basic image type string, excluding the size parameter.

	bool isRaw() const ;
		///< Returns true if a raw image type.

	bool isJpeg() const ;
		///< Returns true if a jpeg image type.

	bool isPng() const ;
		///< Returns true if a png image type.

	bool isPnm() const ;
		///< Returns true if a pnm image type.

	int dx() const ;
		///< Returns the image width.

	int dy() const ;
		///< Returns the image height.

	int channels() const ;
		///< Returns the number of channels.

	size_t size() const ;
		///< Returns the product of dx, dy and channels.
		///< This is usually only meaningful for raw types.

	size_t rowsize() const ;
		///< Returns the product of dx and channels.
		///< This is usually only meaningful for raw types.

	bool operator==( const ImageType & ) const ;
		///< Comparison operator.

	bool operator!=( const ImageType & ) const ;
		///< Comparison operator.

	bool operator<( const ImageType & ) const ;
		///< Comparison operator.

	void streamOut( std::ostream & ) const ;
		///< Used by op<<().

private:
	enum Type { t_invalid , t_jpeg , t_png , t_raw , t_pnm } ;
	ImageType( Type type_ , int dx_ , int dy_ , int channels_ ) ;
	static Type typeFromSignature( const unsigned char * , size_t ) ;
	void init( std::istream & ) ;
	void init( const unsigned char * p , size_t n ) ;
	void init( Type , int , int , int ) ;
	void setsimple( String & ) const ;

private:
	Type m_type ;
	int m_dx ;
	int m_dy ;
	int m_channels ;
} ;

namespace Gr
{
	inline
	std::ostream & operator<<( std::ostream & stream , const ImageType & s )
	{
		s.streamOut( stream ) ;
		return stream ;
	}
}

inline
int Gr::ImageType::dx() const
{
	return m_dx ;
}

inline
int Gr::ImageType::dy() const
{
	return m_dy ;
}

inline
int Gr::ImageType::channels() const
{
	return m_channels ;
}

#endif

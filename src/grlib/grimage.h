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
/// \file grimage.h
///

#ifndef GR_IMAGE__H
#define GR_IMAGE__H

#include "gdef.h"
#include "grdef.h"
#include "grimagetype.h"
#include "grimagedata.h"
#include "grimagebuffer.h"
#include "gpublisher.h"

namespace Gr
{
	class Image ;
}

/// \class Gr::Image
/// A class holding shared read-only image data (Gr::ImageBuffer) 
/// and its associated image type (Gr::ImageType). Static methods 
/// are used to read images from file or publication channel.
/// 
class Gr::Image
{
public:
	G_EXCEPTION( ReadError , "cannot read image" ) ;

	Image() ;
		///< Default constructor for an empty() image with an in-valid() 
		///< type() and a null ptr().

	Image( shared_ptr<const ImageBuffer> ptr , Gr::ImageType ) ;
		///< Constructor, taking a shared pointer to the image data and its 
		///< image type. 
		///< 
		///< The image type is allowed to be in-valid() while still having a
		///< non-empty image buffer; this allows Image object to hold non-image 
		///< data or non-standard image types, with the image or non-image
		///< type information held separately.

	bool empty() const ;
		///< Returns true if constructed with no image data.

	void clear() ;
		///< Clears the image as if default constructed.
		///< Postcondition: empty()

	bool valid() const ;
		///< Returns !empty() && type().valid().

	ImageType type() const ;
		///< Returns the image type.

	size_t size() const ; 
		///< Returns the image data total size.

	shared_ptr<ImageBuffer> recycle() ;
		///< Creates a new shared pointer that can be deposited into a new Image,
		///< but if the current Image is not empty() and not shared (unique()) then 
		///< its buffer is returned instead. This can avoid memory allocation when 
		///< processing a sequence of images all of the same size, while still 
		///< allowing slow code to take copies.
		///< \code
		///<
		///< Image ImageFactory::newImage( Image & old_image ) {
		///<   shared_ptr<ImageBuffer> ptr = old_image.recycle() ;
		///<   resize_and_fill( *ptr , m_type ) ;
		///<   return Image( ptr , m_type ) ;
		///< }
		///< \endcode

	shared_ptr<const ImageBuffer> ptr() const ;
		///< Returns the image data shared pointer.

	const ImageBuffer & data() const ;
		///< Returns the image data.
		///< Precondition: !empty()

	static void read( std::istream & , Image & , const std::string & exception_help_text = std::string() ) ;
		///< Reads an input stream into an image. Throws on error.

	static bool receive( G::PublisherSubscription & channel , Image & , std::string & type_str ) ;
		///< Reads a publication channel into an image. If a non-image is received 
		///< then the non-image type will be deposited in the supplied string
		///< and the Image type() will be invalid(). Returns false iff the channel
		///< receive fails.

	static bool peek( G::PublisherSubscription & channel , Image & , std::string & type_str ) ;
		///< A variant on receive() that does a channel peek().

	static ImageBuffer * blank( Image & , ImageType raw_type , bool contiguous = false ) ;
		///< Factory function for a not-really-blank raw image that is temporarily writable 
		///< via the returned image buffer pointer. The implementation recycle()s the 
		///< image and resizes its image buffer to match the given image type and 
		///< contiguity.
		///< Precondition: type().isRaw()

private:
	static bool receiveImp( bool , G::PublisherSubscription & , Image & , std::string & ) ;

private:
	shared_ptr<const ImageBuffer> m_ptr ;
	ImageType m_type ;
} ;

#endif

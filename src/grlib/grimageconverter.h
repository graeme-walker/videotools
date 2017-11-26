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
/// \file grimageconverter.h
///

#ifndef GR_IMAGE_CONVERTER__H
#define GR_IMAGE_CONVERTER__H

#include "gdef.h"
#include "grimage.h"
#include "grimagedata.h"
#include "grimagedecoder.h"
#include <vector>
#include <string>

namespace Gr
{
	class ImageConverter ;
}

/// \class Gr::ImageConverter
/// An image format converter that can convert to and from the raw and jpeg 
/// formats (only), with scaling and monochrome options.
/// 
/// The implementation uses Gr::ImageDecoder for decoding, and Gr::JpegWriter
/// for encoding.
/// 
class Gr::ImageConverter
{
public:
	ImageConverter() ;
		///< Default constructor.

	bool toRaw( Image image_in , Image & image_out , int scale = 1 , bool monochrome_out = false ) ;
		///< Converts the image to raw format. Returns a false on error.

	bool toJpeg( Image image_in , Image & image_out , int scale = 1 , bool monochrome_out = false ) ;
		///< Converts the image to jpeg format. Returns false on error.

	static bool convertible( Gr::ImageType ) ;
		///< Returns true if the image type is convertible. In practice this
		///< returns true for jpeg and raw image types.

private:
	bool toRawImp( Image image_in , Image & image_out , int scale , bool monochrome_out ) ;
	bool toJpegImp( Image image_in , Image & image_out , int scale , bool monochrome_out ) ;

private:
	ImageDecoder m_decoder ;
	JpegWriter m_jpeg_writer ;
	Image m_image_tmp ;
} ;

#endif

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
/// \file grimagedecoder.h
///

#ifndef GR_IMAGE_DECODER__H
#define GR_IMAGE_DECODER__H

#include "gdef.h"
#include "grimagetype.h"
#include "grimagedata.h"
#include "grpng.h"
#include "grjpeg.h"
#include "grpnm.h"
#include "gpath.h"
#include "gexception.h"
#include <vector>

namespace Gr
{
	class ImageDecoder ;
}

/// \class Gr::ImageDecoder
/// An interface for decoding encoded images into a raw format. The input 
/// images can be in raw, png, jpeg or pnm formats.
/// 
/// Note that raw format is not self-describing, so raw images often use a 
/// binary pnm format when stored to file and these files are trivially 
/// decoded into the raw format as they are read in.
/// 
class Gr::ImageDecoder
{
public:
	G_EXCEPTION( Error , "image decoding error" ) ;

	struct ScaleToFit /// Describes scale-to-fit target dimensions.
	{
		ScaleToFit() ;
		ScaleToFit( int dx , int dy , int fudge_factor ) ;
		int operator()( const ImageType & ) const ;
		operator bool() const ;
		int dx ;
		int dy ;
		int ff ;
	} ;

	ImageDecoder() ;
		///< Default constructor.

	void setup( int scale , bool monochrome_out ) ;
		///< Sets the scale factor.

	ImageType decode( const G::Path & path_in , 
		ImageData & out , const ScaleToFit & = ScaleToFit() ) ;
			///< Decodes a file, with a scale-to-fit option. 
			///< Scale-to-fit scaling takes precedence over the decoder's default scale factor. 
			///< Returns the raw image type. Throws on error.

	ImageType decode( const ImageType & type_in , const G::Path & path_in , 
		ImageData & out , ScaleToFit = ScaleToFit() ) ;
			///< Decodes a file with the given image type, with a scale-to-fit option. 
			///< This is a more efficient overload if the file type is already known. 
			///< Scale-to-fit scaling overrides the decoder's default scale factor. 
			///< Returns the raw image type. Throws on error.

	ImageType decode( const ImageType & type_in , const char * data_in , size_t , 
		ImageData & out , const ScaleToFit & = ScaleToFit() ) ;
			///< Decodes the image buffer and returns the raw image type. 
			///< Throws on error.

	ImageType decode( const ImageType & type_in , const std::vector<char> & data_in , 
		ImageData & out , const ScaleToFit & = ScaleToFit() ) ;
			///< Decodes the image buffer and returns the raw image type. 
			///< Throws on error.

	ImageType decode( const ImageType & type_in , const ImageBuffer & data_in , 
		ImageData & out , const ScaleToFit & = ScaleToFit() ) ;
			///< Decodes the image buffer and returns the raw image type. 
			///< Throws on error.

	static ImageType readType( const G::Path & path , bool do_throw = true ) ;
		///< A convenience function to read a file's image type. 
		///< Throws on error, by default.

	ImageType decodeInPlace( ImageType type_in , char * & p , size_t size_in , ImageData & store ) ;
		///< Decodes an image buffer with raw decoding done in-place.
		///< The image pointer is passed by reference and is either unchanged 
		///< after an in-place decode, or it is set to point into the supplied
		///< contiguous ImageData object.

private:
	ImageDecoder( const ImageDecoder & ) ;
	void operator=( const ImageDecoder & ) ;
	ImageType decodeFile( ImageType , const G::Path & , ImageData & , const ScaleToFit & ) ;
	ImageType decodeBuffer( ImageType , const char * p , size_t n , ImageData & , const ScaleToFit & ) ;
	static bool jpegAvailable() ;
	static bool pngAvailable() ;

private:
	int m_scale ;
	bool m_monochrome_out ;
	JpegReader m_jpeg ;
	PngReader m_png ;
	PnmReader m_pnm ;
} ;


inline
Gr::ImageDecoder::ScaleToFit::ScaleToFit() :
	dx(0) ,
	dy(0) ,
	ff(0)
{
}

inline
Gr::ImageDecoder::ScaleToFit::ScaleToFit( int dx_ , int dy_ , int ff_ ) :
	dx(dx_) ,
	dy(dy_) ,
	ff(ff_)
{
}

#endif

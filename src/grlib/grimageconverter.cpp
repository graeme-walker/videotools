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
// grimageconverter.cpp
//

#include "gdef.h"
#include "grimageconverter.h"
#include "grimagetype.h"
#include "grimagedecoder.h"
#include "grcolourspace.h"
#include "grjpeg.h"
#include "gdebug.h"
#include <sstream>

Gr::ImageConverter::ImageConverter()
{
	m_image_tmp = Image( shared_ptr<ImageBuffer>(new ImageBuffer) , ImageType() ) ;
}

bool Gr::ImageConverter::convertible( Gr::ImageType type )
{
	return ( type.isRaw() || type.isPnm() || 
		( type.isJpeg() && Gr::Jpeg::available() ) || ( type.isPng() && Gr::Png::available() ) ) &&
		type.dx() > 0 && type.dy() > 0 && ( type.channels() == 1 || type.channels() == 3 ) ;
}

bool Gr::ImageConverter::toRaw( Image image_in , Image & image_out , int scale , bool monochrome_out )
{
	return toRawImp( image_in , image_out , std::max(1,scale) , monochrome_out ) ;
}

bool Gr::ImageConverter::toRawImp( Image image_in , Image & image_out , int scale , bool monochrome_out )
{
	if( image_in.type().isRaw() && scale == 1 && (image_in.type().channels()==1 || !monochrome_out) )
	{
		image_out = image_in ;
		return true ;
	}
	else if( convertible(image_in.type()) )
	{
		// delegate anything-to-raw to the ImageDecoder
		ImageType type_out = ImageType::raw( image_in.type() , scale , monochrome_out ) ;
		ImageBuffer * image_out_p = Gr::Image::blank( image_out , type_out , false ) ;
		ImageData image_data_out( *image_out_p , type_out.dx() , type_out.dy() , type_out.channels() ) ;
		m_decoder.setup( scale , monochrome_out ) ;
		type_out = m_decoder.decode( image_in.type() , image_in.data() , image_data_out ) ;
		return image_out.valid() ;
	}
	else
	{
		G_DEBUG( "Gr::ImageConverter::toRawImp: invalid input image type: " << image_in.type() ) ;
		return false ;
	}
}

bool Gr::ImageConverter::toJpeg( Image image_in , Image & image_out , int scale , bool monochrome_out )
{
	return toJpegImp( image_in , image_out , std::max(1,scale) , monochrome_out ) ;
}

bool Gr::ImageConverter::toJpegImp( Image image_in , Image & image_out , int scale , bool monochrome_out )
{
	if( convertible(image_in.type()) && image_in.type().isRaw() )
	{
		const ImageData data_in( const_cast<ImageBuffer&>(image_in.data()) , image_in.type().dx() , image_in.type().dy() , image_in.type().channels() ) ;

		ImageType type_out = ImageType::jpeg( image_in.type() , scale , monochrome_out ) ;
		shared_ptr<ImageBuffer> ptr_out = image_out.recycle() ;

		m_jpeg_writer.setup( scale , monochrome_out ) ;
		m_jpeg_writer.encode( data_in , *ptr_out ) ;
		image_out = Image( ptr_out , type_out ) ;
		return true ;
	}
	else if( convertible(image_in.type()) && image_in.type().isJpeg() )
	{
		if( scale == 1 && !monochrome_out )
		{
			image_out = image_in ;
			return true ;
		}
		else
		{
			return
				toRawImp( image_in , m_image_tmp , scale , false ) &&
				toJpegImp( m_image_tmp , image_out , 1 , monochrome_out ) ;
		}
	}
	else
	{
		return false ;
	}
}

/// \file grimageconverter.cpp

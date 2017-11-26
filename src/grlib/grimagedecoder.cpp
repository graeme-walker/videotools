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
// grimagedecoder.cpp
// 

#include "gdef.h"
#include "grdef.h"
#include "grimagedecoder.h"
#include "grimagetype.h"
#include "groot.h"
#include "grjpeg.h"
#include "grpng.h"
#include "grpnm.h"
#include "grcolourspace.h"
#include "glog.h"
#include "gassert.h"
#include <cstring> // memcpy()

namespace
{
	Gr::ImageType rawtype( const Gr::ImageData & image_data )
	{
		return Gr::ImageType::raw( image_data.dx() , image_data.dy() , image_data.channels() ) ;
	}
}

// ==

Gr::ImageDecoder::ImageDecoder() :
	m_scale(1) ,
	m_monochrome_out(false)
{
}

void Gr::ImageDecoder::setup( int scale , bool monochrome_out )
{
	m_scale = scale ;
	m_monochrome_out = monochrome_out ;
}

Gr::ImageType Gr::ImageDecoder::decode( const G::Path & path , ImageData & out , const ScaleToFit & scale_to_fit )
{
	return decodeFile( readType(path) , path , out , scale_to_fit ) ;
}

Gr::ImageType Gr::ImageDecoder::decode( const ImageType & image_type , const G::Path & path , ImageData & out , ScaleToFit scale_to_fit )
{
	return decodeFile( image_type , path , out , scale_to_fit ) ;
}

Gr::ImageType Gr::ImageDecoder::readType( const G::Path & path , bool do_throw )
{
	try
	{
		std::ifstream file ;
		{
			G::Root claim_root ;
			file.open( path.str().c_str() ) ;
		}
		if( !file.good() )
			throw Error( "cannot open [" + path.str() + "]" ) ;
		return ImageType( file ) ;
	}
	catch( std::exception & )
	{
		if( do_throw ) throw ;
		return ImageType() ;
	}
}

Gr::ImageType Gr::ImageDecoder::decodeFile( ImageType type_in , const G::Path & path , ImageData & out , const ScaleToFit & scale_to_fit )
{
	G_DEBUG( "Gr::ImageDecoder::decodeFile: path=[" << path << "] type=[" << type_in << "]" ) ;

	int scale = scale_to_fit ? scale_to_fit(type_in) : m_scale ;

	if( type_in.isJpeg() && jpegAvailable() )
	{
		m_jpeg.setup( scale , m_monochrome_out ) ;
		m_jpeg.decode( out , path ) ;
	}
	else if( type_in.isPng() && pngAvailable() )
	{
		m_png.setup( scale , m_monochrome_out ) ;
		m_png.decode( out , path ) ;
	}
	else if( type_in.isPnm() )
	{
		m_pnm.setup( scale , m_monochrome_out ) ;
		m_pnm.decode( out , path ) ;
	}
	else
	{
		throw Error( "invalid file format" , path.str() ) ;
	}
	return rawtype(out) ;
}

Gr::ImageType Gr::ImageDecoder::decode( const ImageType & type_in , const std::vector<char> & data , ImageData & out , const ScaleToFit & scale_to_fit )
{
	return decodeBuffer( type_in , data.data() , data.size() , out , scale_to_fit ) ;
}

Gr::ImageType Gr::ImageDecoder::decode( const ImageType & type_in , const char * p , size_t n , ImageData & out , const ScaleToFit & scale_to_fit )
{
	return decodeBuffer( type_in , p , n , out , scale_to_fit ) ;
}

Gr::ImageType Gr::ImageDecoder::decodeBuffer( ImageType type_in , const char * p , size_t n , ImageData & out , const ScaleToFit & scale_to_fit )
{
	G_DEBUG( "Gr::ImageDecoder::decodeBuffer: size=" << n << " type=[" << type_in << "]" ) ;

	int scale = scale_to_fit ? scale_to_fit(type_in) : m_scale ;

	if( type_in.isJpeg() && jpegAvailable() )
	{
		m_jpeg.setup( scale , m_monochrome_out ) ;
		m_jpeg.decode( out , p , n ) ;
	}
	else if( type_in.isPng() && pngAvailable() )
	{
		m_png.setup( scale , m_monochrome_out ) ;
		m_png.decode( out , p , n ) ;
	}
	else if( type_in.isPnm() )
	{
		m_pnm.setup( scale , m_monochrome_out ) ;
		m_pnm.decode( out , p , n ) ;
	}
	else if( type_in.isRaw() )
	{
		// raw-to-raw copy
		ImageType type_out = ImageType::raw( type_in , scale , m_monochrome_out ) ;
		out.resize( type_out.dx() , type_out.dy() , type_out.channels() ) ;
		out.copyIn( p , n , type_in.dx() , type_in.dy() , type_in.channels() , true/*colourspace*/ , scale ) ;
	}
	else
	{
		throw Error( "invalid format" ) ;
	}
	return rawtype( out ) ;
}

Gr::ImageType Gr::ImageDecoder::decode( const ImageType & type_in , const ImageBuffer & image_buffer , ImageData & out , const ScaleToFit & scale_to_fit )
{
	G_DEBUG( "Gr::ImageDecoder::decode: ImageBuffer type=[" << type_in << "]" ) ;

	int scale = scale_to_fit ? scale_to_fit(type_in) : m_scale ;

	if( type_in.isJpeg() && jpegAvailable() )
	{
		m_jpeg.setup( scale , m_monochrome_out ) ;
		m_jpeg.decode( out , image_buffer ) ;
	}
	else if( type_in.isPng() && pngAvailable() )
	{
		m_png.setup( scale , m_monochrome_out ) ;
		m_png.decode( out , image_buffer ) ;
	}
	else if( type_in.isPnm() )
	{
		m_pnm.setup( scale , m_monochrome_out ) ;
		m_pnm.decode( out , image_buffer ) ;
	}
	else if( type_in.isRaw() )
	{
		// raw-to-raw copy
		ImageType type_out = ImageType::raw( type_in , scale , m_monochrome_out ) ;
		out.resize( type_out.dx() , type_out.dy() , type_out.channels() ) ;
		out.copyIn( image_buffer , type_in.dx() , type_in.dy() , type_in.channels() , true/*colourspace*/ , scale ) ;
	}
	else
	{
		throw Error( "invalid format" ) ;
	}
	return rawtype( out ) ;
}

Gr::ImageType Gr::ImageDecoder::decodeInPlace( ImageType type_in , char * & p , size_t size_in , 
	Gr::ImageData & out_store )
{
	G_ASSERT( out_store.type() == ImageData::Contiguous ) ;

	PnmInfo pnm_info ;
	if( type_in.isPnm() )
		pnm_info = PnmInfo( p , size_in ) ;

	ImageType type_out ;
	if( type_in.isJpeg() && jpegAvailable() )
	{
		m_jpeg.setup( m_scale , m_monochrome_out ) ;
		m_jpeg.decode( out_store , p , size_in ) ;
		p = reinterpret_cast<char*>( out_store.p() ) ;
		type_out = rawtype( out_store ) ;
	}
	else if( type_in.isPng() && pngAvailable() )
	{
		m_png.setup( m_scale , m_monochrome_out ) ;
		m_png.decode( out_store , p , size_in ) ;
		p = reinterpret_cast<char*>( out_store.p() ) ;
		type_out = rawtype( out_store ) ;
	}
	else if( type_in.isPnm() && pnm_info.binary8() && 
		( m_scale > 1 || type_in.channels() != (m_monochrome_out?1:type_in.channels()) ) )
	{
		// strip the header
		type_out = ImageType::raw( type_in , m_scale , m_monochrome_out ) ;
		if( size_in <= pnm_info.offset() || (size_in-pnm_info.offset()) != type_out.size() ) 
			throw Error( "invalid pnm size" ) ;
		std::memmove( p , p+pnm_info.offset() , size_in-pnm_info.offset() ) ;

		// treat as raw
		unsigned char * p_out = reinterpret_cast<unsigned char*>(p) ;
		const unsigned char * p_in = p_out ;
		size_t dp_in = sizet( m_scale , type_in.channels() ) ;
		for( int y = 0 ; y < type_in.dy() ; y += m_scale )
		{
			for( int x = 0 ; x < type_in.dx() ; x += m_scale , p += dp_in )
			{
				if( m_monochrome_out && type_in.channels() == 3 )
					*p_out++ = Gr::ColourSpace::y_int( p_in[0] , p_in[1] , p_in[2] ) ;
				else if( m_monochrome_out )
					*p_out++ = *p_in ;
				else
					{ *p_out++ = p_in[0] ; *p_out++ = p_in[1] ; *p_out++ = p_in[2] ; }
			}
		}
	}
	else if( type_in.isPnm() && pnm_info.binary8() )
	{
		// just strip the header
		type_out = ImageType::raw( type_in , m_scale , m_monochrome_out ) ;
		if( size_in <= pnm_info.offset() || (size_in-pnm_info.offset()) != type_out.size() ) 
			throw Error( "invalid pnm size" ) ;
		std::memmove( p , p+pnm_info.offset() , size_in-pnm_info.offset() ) ;
	}
	else if( type_in.isPnm() )
	{
		m_pnm.setup( m_scale , m_monochrome_out ) ;
		m_pnm.decode( out_store , p , size_in ) ;
		p = reinterpret_cast<char*>( out_store.p() ) ;
		type_out = rawtype( out_store ) ;
	}
	else if( type_in.isRaw() && (m_scale > 1 || type_in.channels() != (m_monochrome_out?1:type_in.channels()) ) )
	{
		type_out = ImageType::raw( type_in , m_scale , m_monochrome_out ) ;
		unsigned char * p_out = reinterpret_cast<unsigned char*>(p) ;
		const unsigned char * p_in = p_out ;
		size_t dp_in = sizet( m_scale , type_in.channels() ) ;
		for( int y = 0 ; y < type_in.dy() ; y += m_scale )
		{
			for( int x = 0 ; x < type_in.dx() ; x += m_scale , p += dp_in )
			{
				if( m_monochrome_out && type_in.channels() == 3 )
					*p_out++ = Gr::ColourSpace::y_int( p_in[0] , p_in[1] , p_in[2] ) ;
				else if( m_monochrome_out )
					*p_out++ = *p_in ;
				else
					{ *p_out++ = p_in[0] ; *p_out++ = p_in[1] ; *p_out++ = p_in[2] ; }
			}
		}
	}
	else if( type_in.isRaw() )
	{
		type_out = type_in ;
	}
	else
	{
		throw Error( "invalid format" ) ;
	}
	return type_out ;
}

bool Gr::ImageDecoder::jpegAvailable()
{
	if( !Jpeg::available() )
		G_WARNING_ONCE( "Gr::ImageDecoder::jpegAvailable: no jpeg library built-in" ) ;
	return Jpeg::available() ;
}

bool Gr::ImageDecoder::pngAvailable()
{
	if( !Png::available() )
		G_WARNING_ONCE( "Gr::ImageDecoder::jpegAvailable: no png library built-in" ) ;
	return Jpeg::available() ;
}

// ==

int Gr::ImageDecoder::ScaleToFit::operator()( const ImageType & type ) const
{
	// work out an integral scale factor to more-or-less fit into the target 
	// image size -- the fudge factor says how much of the image can be
	// lost in one dimension when getting the other dimension to fit, eg.
	// 3 for a third, or zero to fit everything in
	//
	G_ASSERT( ff >= 0 && dx > 0 && dy > 0 ) ;
	if( !type.valid() ) return 1 ;
	int image_dx = type.dx() ; if( ff > 0 ) image_dx -= (image_dx/ff) ;
	int image_dy = type.dy() ; if( ff > 0 ) image_dy -= (image_dy/ff) ;
	int scale_x = (image_dx+dx-1) / std::max(1,dx) ;
	int scale_y = (image_dy+dy-1) / std::max(1,dy) ;

	{
		G_ASSERT( scaled(image_dx,scale_x) <= dx ) ;
		G_ASSERT( scaled(image_dy,scale_y) <= dy ) ;
		if( scale_x > 1 ) G_ASSERT( scaled(image_dx,scale_x-1) > dx ) ;
		if( scale_y > 1 ) G_ASSERT( scaled(image_dy,scale_y-1) > dy ) ;
	}

	int result = std::max( scale_x , scale_y ) ;
	return result ;
}

Gr::ImageDecoder::ScaleToFit::operator bool() const
{
	const bool zero = dx <= 0 && dy <= 0 ;
	return !zero ;
}

/// \file grimagedecoder.cpp

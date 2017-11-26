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
// gvavcreader_libav.cpp
//

#include "gdef.h"
#include "gravc.h"
#include "gvavcreader.h"
#include "grimagetype.h"
#include "gstr.h"
#include "gtest.h"
#include "ghexdump.h"
#include "gassert.h"
#include <cstring>
#include <vector>
#include <algorithm> // std::max
#include <iostream>
#include <sstream>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h" // av_image_alloc
#include "libavutil/mem.h" // av_free
}

// the libav api is a moving target, and these transitional version numbers 
// are not necessarily correct -- autoconf tests are preferable
// 
#ifndef GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN
#if LIBAVCODEC_VERSION_MAJOR >= 56
#define GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN 1
#else
#define GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN 0
#endif
#endif
//
#ifndef GCONFIG_LIBAV_NEW_DECODE_FN
#if LIBAVCODEC_VERSION_MAJOR >= 57
#define GCONFIG_LIBAV_NEW_DECODE_FN 1
#else
#define GCONFIG_LIBAV_NEW_DECODE_FN 0
#endif
#endif
//
#ifndef GCONFIG_LIBAV_NEW_DESCRIPTOR
#if LIBAVCODEC_VERSION_MAJOR >= 57
#define GCONFIG_LIBAV_NEW_DESCRIPTOR 1
#else
#define GCONFIG_LIBAV_NEW_DESCRIPTOR 0
#endif
#endif
//
#ifndef AV_PIX_FMT_FLAG_BE
#define AV_PIX_FMT_FLAG_BE PIX_FMT_BE
#define AV_PIX_FMT_FLAG_PAL PIX_FMT_PAL
#define AV_PIX_FMT_FLAG_BITSTREAM PIX_FMT_BITSTREAM
#define AV_PIX_FMT_FLAG_RGB PIX_FMT_RGB
#endif

static void logger( void * p , int level , const char * s , va_list args )
{
	const char * class_name = p && *(AVClass**)p ? (*(AVClass**)p)->class_name : "libav" ;

	std::string text ;
	{
		static std::vector<char> buffer( 100U ) ;
		buffer[0] = '\0' ;
		int rc = vsnprintf( &buffer[0] , buffer.size() , s , args ) ;
		if( rc <= 0 ) rc = 1 , buffer[0] = '\0' ;
		size_t lastpos = std::min(static_cast<size_t>(rc),buffer.size()) - 1 ;
		if( buffer[lastpos] == '\n' ) buffer[lastpos] = '\0' ;
		text = std::string(class_name) + ": [" + std::string(&buffer[0]) + "]" ;
	}

	if( text.find("AVCodecContext: [no frame!]") != std::string::npos )
		level = AV_LOG_INFO ;

	if( level <= AV_LOG_ERROR ) 
		G_WARNING( "Gv::AvcReaderStream: libav error: " << text ) ;
	else if( level <= AV_LOG_WARNING )
		G_WARNING( "Gv::AvcReaderStream: libav warning: " << text ) ;
	else if( level <= AV_LOG_INFO )
		G_LOG( "Gv::AvcReaderStream: libav info: " << text ) ;
	else 
		G_DEBUG( "Gv::AvcReaderStream: libav info: " << text ) ;
}

/// \class Gv::AvcReaderStreamImp
/// A private pimple class for Gv::AvcReaderStream, holding
/// AVCode, AVCodecContext and AVFrame libav objects.
///
class Gv::AvcReaderStreamImp
{
public:
	AvcReaderStreamImp() ;
	explicit AvcReaderStreamImp( const Gr::Avc::Configuration & avcc ) ;
	~AvcReaderStreamImp() ;
	void init( const Gr::Avc::Configuration * avcc_p ) ;

public:
	int m_dx ;
	int m_dy ;
	AVCodec * m_codec ;
	AVCodecContext * m_cc ;
	std::string m_sps_pps ;
	AVFrame * m_frame ;
} ;

Gv::AvcReaderStreamImp::AvcReaderStreamImp() :
	m_dx(0) ,
	m_dy(0) ,
	m_codec(nullptr) ,
	m_cc(nullptr) ,
	m_frame(nullptr)
{
	init( nullptr ) ;
}

Gv::AvcReaderStreamImp::AvcReaderStreamImp( const Gr::Avc::Configuration & avcc ) :
	m_dx(0) ,
	m_dy(0) ,
	m_codec(nullptr) ,
	m_cc(nullptr) ,
	m_frame(nullptr)
{
	init( &avcc ) ;
}

void Gv::AvcReaderStreamImp::init( const Gr::Avc::Configuration * avcc_p )
{
	if( avcc_p != nullptr )
	{
		m_dx = avcc_p->sps(0U).dx() ;
		m_dy = avcc_p->sps(0U).dy() ;
	}

	// logging
	{
		av_log_set_callback( logger ) ;
		if( G::LogOutput::instance() && G::LogOutput::instance()->at(G::Log::s_Debug) )
			av_log_set_level( AV_LOG_DEBUG ) ;
		else
			av_log_set_level( AV_LOG_VERBOSE ) ;
	}

	// m_codec
	{
		avcodec_register_all() ;

		m_codec = avcodec_find_decoder( AV_CODEC_ID_H264 ) ;
		if( m_codec == nullptr ) 
			throw std::runtime_error( "avcodec_find_decoder failed" ) ;
	}

	// m_cc
	{
		m_cc = avcodec_alloc_context3( m_codec ) ;
		G_ASSERT( m_cc != nullptr ) ;
		m_cc->debug |= FF_DEBUG_STARTCODE ;
		m_cc->debug |= FF_DEBUG_PICT_INFO ;
	}

	// pass any avcc from a fmtp into to the context
	if( avcc_p != nullptr )
	{
		// create a two-NALU stream from the SPS and PPS structures and feed it into
		// the libav context via its extradata fields -- see also 14496-10 B.1.2
		//
		m_sps_pps = avcc_p->nalus() ; // "00-00-00-01 <sps> 00-00-01 <pps>"
		m_cc->extradata = reinterpret_cast<uint8_t*>(const_cast<char*>(m_sps_pps.data())) ;
		m_cc->extradata_size = m_sps_pps.size() ;
	}

	// m_codec + m_cc
	{
		int e = avcodec_open2( m_cc , m_codec , nullptr ) ;
		if( e < 0 ) 
			throw std::runtime_error( "avcodec_open2 failed: " + G::Str::fromInt(e) ) ;

		//G_ASSERT( m_cc->codec_type == AVMEDIA_TYPE_VIDEO ) ;
		//G_ASSERT( m_cc->codec_id == AV_CODEC_ID_H264 ) ;
	}

	// m_frame
	{
#if GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN
		m_frame = av_frame_alloc() ;
		G_ASSERT( m_frame != nullptr ) ;
#else
		m_frame = avcodec_alloc_frame() ;
		G_ASSERT( m_frame != nullptr ) ;
		avcodec_get_frame_defaults( m_frame ) ;
#endif
	}
}

Gv::AvcReaderStreamImp::~AvcReaderStreamImp()
{
	if( m_cc ) avcodec_close(m_cc) , av_free(m_cc) ;
#if GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN
	if( m_frame ) av_frame_free( &m_frame ) ;
#else
	if( m_frame ) avcodec_free_frame( &m_frame ) ;
#endif
}

// ==

Gv::AvcReaderStream::AvcReaderStream() :
	m_imp(new AvcReaderStreamImp)
{
}

Gv::AvcReaderStream::AvcReaderStream( const Gr::Avc::Configuration & avcc ) :
	m_imp(new AvcReaderStreamImp(avcc))
{
}

Gv::AvcReaderStream::~AvcReaderStream()
{
	delete m_imp ;
}

int Gv::AvcReaderStream::dx() const
{
	return m_imp->m_dx ;
}

int Gv::AvcReaderStream::dy() const
{
	return m_imp->m_dy ;
}

// ==

namespace Gv
{
	std::ostream & operator<<( std::ostream & stream , const AvcReader::Component & c )
	{
		return stream 
			<< "data=" << (const void*)c.data << " "
			<< "offset=" << c.offset << " "
			<< "linesize=" << c.linesize << " "
			<< "step=" << c.step << " "
			<< "depth=" << c.depth << " "
			<< "mask=" << c.mask << " "
			<< "value-shift=" << c.shift << " "
			<< "eightbit=" << c.eightbit << " "
			<< "x-shift=" << c.x_shift << " "
			<< "y-shift=" << c.y_shift ;
	}
}

namespace
{
	Gv::AvcReader::Component make_component( const AVFrame & frame , const AVPixFmtDescriptor & pfd , 
		const AVComponentDescriptor & pfd_comp )
	{
		Gv::AvcReader::Component comp ;
		const bool big_endian = !!( pfd.flags & AV_PIX_FMT_FLAG_BE ) ;
		comp.data = frame.data[pfd_comp.plane] ;
		comp.linesize = frame.linesize[pfd_comp.plane] ;
#if GCONFIG_LIBAV_NEW_DESCRIPTOR
		comp.offset = pfd_comp.offset ;
		comp.step = pfd_comp.step ;
		comp.depth = pfd_comp.depth ;
#else
		comp.offset = pfd_comp.offset_plus1 - 1U ;
		comp.step = pfd_comp.step_minus1 + 1U ;
		comp.depth = pfd_comp.depth_minus1 + 1U ;
#endif
		comp.mask = (1U << comp.depth) - 1U ;
		comp.shift = pfd_comp.shift ;
		comp.eightbit = (comp.shift + comp.depth) <= 8U ;
		comp.offset += ( comp.eightbit && big_endian ? 1U : 0U ) ; // as in av_read_image_line()
		comp.x_shift = 0 ; // set later
		comp.y_shift = 0 ; // set later
		return comp ;
	}
	bool valid_pfd( const AVPixFmtDescriptor * pfd )
	{
		return
			pfd != nullptr &&
			pfd->nb_components >= 3U &&
			!(pfd->flags&AV_PIX_FMT_FLAG_PAL) && // (palette)
			!(pfd->flags&AV_PIX_FMT_FLAG_BITSTREAM) ;
	}
	void interpret_frame( AVFrame * frame , Gv::AvcReader::Data & m )
	{
		// set up the Component structures for efficient access by the inline methods

		// see av_read_image_line() in libavutil/pixdesc.c
		const AVPixFmtDescriptor * pfd = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(frame->format) ) ;
		const char * format_name = pfd ? pfd->name : "" ;
		G_DEBUG( "Gv::AvcReader::init: pixel format " << format_name ) ;
		if( !valid_pfd(pfd) )
			throw std::runtime_error( std::string("unexpected pixel format [") + format_name + "]" ) ;

		// parse the description
		m.m_rgb = !!( pfd->flags & AV_PIX_FMT_FLAG_RGB ) ;
		m.m_big_endian = !!( pfd->flags & AV_PIX_FMT_FLAG_BE ) ;
		m.m_x_shift = pfd->log2_chroma_w ;
		m.m_y_shift = pfd->log2_chroma_h ;

		// parse the components
		unsigned int n = std::min( 4U , static_cast<unsigned int>(pfd->nb_components) ) ;
		for( unsigned int c = 0U ; c < n ; c++ )
		{
			m.m_component[c] = make_component( *frame , *pfd , pfd->comp[c] ) ;
		}

		// order the components as r-g-b or y-u-v
		{
			// TODO dont rely on the pixel format name
			const bool do_shift = format_name[0] == 'a' ; // argb/abgr
			const bool do_swap_1_3 = format_name[0] == 'b' || ( format_name[0] == 'a' && format_name[1] == 'b' ) ; // bgr/abgr
			const bool do_swap_2_3 = format_name[0] == 'y' && format_name[1] == 'v' ; // yvu
			if( do_shift )
			{
				m.m_component[0] = m.m_component[1] ;
				m.m_component[1] = m.m_component[2] ;
				m.m_component[2] = m.m_component[3] ;
			}
			if( do_swap_1_3 )
				std::swap( m.m_component[0] , m.m_component[2] ) ;
			else if( do_swap_2_3 )
				std::swap( m.m_component[1] , m.m_component[2] ) ;
		}

		// copy the chroma shifts into the component structures
		m.m_component[1].x_shift = m.m_x_shift ;
		m.m_component[1].y_shift = m.m_y_shift ;
		m.m_component[2].x_shift = m.m_x_shift ;
		m.m_component[2].y_shift = m.m_y_shift ;

		for( unsigned int c = 0 ; c < 3U ; c++ )
		{
			G_DEBUG( "Gv::AvcReader::init: component " << c << ": " << m.m_component[c] ) ;
		}
	}
}

void Gv::AvcReader::init( const unsigned char * p , size_t n )
{
	// sanity check -- do our own parsing of any nalus that we know how to parse
	if( G::Test::enabled("nalu-check") && n > 5U )
	{
		std::string nalu = std::string(reinterpret_cast<const char*>(p+4) , n-4U ) ;
		unsigned int nalu_type = static_cast<unsigned int>(nalu.at(0U)) & 0x9f ;
		bool ok = true ;
    	static int sps_chroma_format_idc = -1 ;
		if( nalu_type == 7U )
		{
			G_LOG_S( "Gv::AvcReader::init: checking sps nalu" ) ;
			Gr::Avc::Sps sps( nalu ) ;
			ok = sps.valid() ;
			if( ok ) sps_chroma_format_idc = sps.m_chroma_format_idc ;
		}
		else if( nalu_type == 8U && sps_chroma_format_idc != -1 )
		{
			G_LOG_S( "Gv::AvcReader::init: checking pps nalu" ) ;
			Gr::Avc::Pps pps( nalu , sps_chroma_format_idc ) ;
			ok = pps.valid() ;
		}
		if( !ok )
			throw std::runtime_error( "nalu check failed" ) ;
	}

	m_data.m_component[0].data = nullptr ;
	m_data.m_component[1].data = nullptr ;
	m_data.m_component[2].data = nullptr ;
	G_ASSERT( !valid() ) ;

	m_data.m_dx = m_stream.dx() ;
	m_data.m_dy = m_stream.dy() ;

	// wrap the raw payload data in an AVPacket
	AVPacket av_packet ;
	av_init_packet( &av_packet ) ;
	av_packet.size = n ;
	av_packet.data = const_cast<uint8_t*>(p) ;

	// pull out the context and frame pointers
	AVCodecContext * cc = m_stream.m_imp->m_cc ;
	AVFrame * frame = m_stream.m_imp->m_frame ;

	// decode the packet into the frame
#if GCONFIG_LIBAV_NEW_DECODE_FN
	int rc = avcodec_send_packet( cc , &av_packet ) ;
	if( rc < 0 ) G_DEBUG( "Gv::AvcReader::init: avcodec_send_packet failed: " << rc ) ;
	while( rc >= 0 )
	{
		rc = avcodec_receive_frame( cc , frame ) ;
		if( rc == AVERROR(EAGAIN) )
		{
			G_DEBUG( "Gv::AvcReader::init: avcodec_receive_frame eagain" ) ;
			break ;
		}
		else if( rc == AVERROR_EOF )
		{
			throw std::runtime_error( "avcodec_receive_frame eof" ) ;
		}
		else
		{
			// (in the unlikely event of more than one frame per packet just keep the last one)
			interpret_frame( frame , m_data ) ;
			m_data.m_dx = cc->width ;
			m_data.m_dy = cc->height ;
		}
	}
#else
	int done = 0 ;
	ssize_t rc = avcodec_decode_video2( cc , frame , &done , &av_packet ) ;
	if( rc >= 0 && done ) 
	{
		interpret_frame( frame , m_data ) ;
		m_data.m_dx = cc->width ;
		m_data.m_dy = cc->height ;
	}
	else if( rc < 0 ) 
	{
		// (some nalu types do not encode images, so not usually an error)
		G_LOG( "Gv::AvcReader::init: libav error: avcodec_decode_video2 failed: " << rc ) ;
	}
#endif
}

bool Gv::AvcReader::keyframe() const
{
	return !!m_stream.m_imp->m_frame->key_frame ;
}

template <typename Tout>
void fill_imp( Tout out_p , const Gv::AvcReader & reader , int scale , bool monochrome_out )
{
	const int dx = reader.dx() ;
	const int dy = reader.dy() ;
	if( reader.simple() )
	{
		if( monochrome_out )
		{
			for( int y = 0 ; y < dy ; y += scale )
			{
				Gv::AvcReader::SimpleIterator in_p( reader , y , scale ) ;
				for( int x = 0 ; x < dx ; x += scale , ++in_p )
				{
					*out_p++ = in_p.luma() ;
				}
			}
		}
		else
		{
			for( int y = 0 ; y < dy ; y += scale )
			{
				Gv::AvcReader::SimpleIterator in_p( reader , y , scale ) ;
				for( int x = 0 ; x < dx ; x += scale , ++in_p )
				{
					*out_p++ = in_p.r() ;
					*out_p++ = in_p.g() ;
					*out_p++ = in_p.b() ;
				}
			}
		}
	}
	else
	{
		if( monochrome_out )
		{
			for( int y = 0 ; y < dy ; y += scale )
			{
				Gv::AvcReader::Iterator in_p( reader , y , scale ) ;
				for( int x = 0 ; x < dx ; x += scale , ++in_p )
				{
					*out_p++ = in_p.luma() ;
				}
			}
		}
		else
		{
			for( int y = 0 ; y < dy ; y += scale )
			{
				Gv::AvcReader::Iterator in_p( reader , y , scale ) ;
				for( int x = 0 ; x < dx ; x += scale , ++in_p )
				{
					*out_p++ = in_p.r() ;
					*out_p++ = in_p.g() ;
					*out_p++ = in_p.b() ;
				}
			}
		}
	}
}

Gr::ImageType Gv::AvcReader::fill( std::vector<char> & buffer , int scale , bool monochrome )
{
	scale = std::max( 1 , scale ) ;
	Gr::ImageType image_type = Gr::ImageType::raw( Gr::scaled(m_data.m_dx,scale) , Gr::scaled(m_data.m_dy,scale) , monochrome?1:3 ) ;

	buffer.resize( image_type.size() ) ;
	fill_imp( &buffer[0] , *this , scale , monochrome ) ;

	G_ASSERT( image_type.size() == buffer.size() ) ;
	return image_type ;
}

bool Gv::AvcReader::available()
{
	return true ;
}

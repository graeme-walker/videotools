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
// grjpeg_jpeg.cpp
//

#include "gdef.h"
#include "grdef.h"
#include "grjpeg.h"
#include "gstr.h"
#include "groot.h"
#include "gtest.h"
#include "glimits.h"
#include "gassert.h"
#include "glog.h"
#include <jpeglib.h>
#include <limits>
#include <cstdio>
#include <cstdlib> // std::free()

typedef int static_assert_jsample_is_char[sizeof(JSAMPLE)==1?1:-1] ;
typedef int static_assert_joctet_is_char[sizeof(JOCTET)==1?1:-1] ;

namespace
{
	std::string message( j_common_ptr p )
	{
		char buffer[JMSG_LENGTH_MAX] ;
		(p->err->format_message)( p , buffer ) ;
		return G::Str::lower(G::Str::printable(buffer)) ;
	}
	void outputMessage( j_common_ptr p )
	{
		if( G::Log::at( G::Log::s_Debug ) )
			G_DEBUG( "Gr::Jpeg::outputMessage: " << message(p) ) ;
	}
	void error( j_common_ptr p )
	{
		std::string e = message( p ) ;
		//G_WARNING( "Gr::Jpeg::error: " << e ) ;
		throw Gr::Jpeg::Error( e ) ;
	}
}

namespace Gr
{
	class JpegBufferSource ;
	class JpegBufferDestination ;
}

/// \class Gr::JpegBufferSource
/// A libjpeg 'decompression source manager' that reads from Gr::ImageBuffer.
///
class Gr::JpegBufferSource : public jpeg_source_mgr
{
public:
	static JpegBufferSource * install( j_decompress_ptr cinfo , const Gr::ImageBuffer & b ) ;

private:
	explicit JpegBufferSource( const Gr::ImageBuffer & b ) ;
	static void init_source( j_decompress_ptr ) ;
	static boolean fill_buffer( j_decompress_ptr cinfo ) ;
	static void skip_input( j_decompress_ptr cinfo , long n ) ;
	static void term_source( j_decompress_ptr cinfo ) ;
	void fill_buffer_imp() ;
	void skip_input_imp( size_t n ) ;

private:
	const ImageBuffer & m_b ;
	traits::imagebuffer<ImageBuffer>::const_row_iterator m_p ;
} ;

Gr::JpegBufferSource * Gr::JpegBufferSource::install( j_decompress_ptr cinfo , const Gr::ImageBuffer & b )
{
	JpegBufferSource * p = new JpegBufferSource( b ) ;
	cinfo->src = p ;
	cinfo->src->init_source = init_source ;
	cinfo->src->fill_input_buffer = fill_buffer ;
	cinfo->src->skip_input_data = skip_input ;
	cinfo->src->resync_to_restart = jpeg_resync_to_restart ;
	cinfo->src->term_source = term_source ;
	cinfo->src->bytes_in_buffer = 0 ;
	cinfo->src->next_input_byte = reinterpret_cast<const unsigned char *>( &(b.at(0))[0] ) ;
	return p ;
}

Gr::JpegBufferSource::JpegBufferSource( const Gr::ImageBuffer & b ) : 
	m_b(b)  ,
	m_p(imagebuffer::row_begin(m_b))
{
	G_DEBUG( "Gr::JpegBufferSource::ctor: installing " << (void*)(this) ) ;
}

void Gr::JpegBufferSource::init_source( j_decompress_ptr )
{
}

boolean Gr::JpegBufferSource::fill_buffer( j_decompress_ptr cinfo )
{
	JpegBufferSource * src = static_cast<JpegBufferSource*>( cinfo->src ) ;
	G_ASSERT( src != nullptr && src->init_source == init_source ) ;

	src->fill_buffer_imp() ;
	return TRUE ;
}

void Gr::JpegBufferSource::fill_buffer_imp()
{
	if( m_p == imagebuffer::row_end(m_b) )
	{
		static const JOCTET eoi[] = { 0xff , JPEG_EOI , 0 , 0 } ;
		next_input_byte = eoi ;
		bytes_in_buffer = 2 ;
	}
	else
	{
		const char * p = imagebuffer::row_ptr(m_p) ;
		next_input_byte = reinterpret_cast<const unsigned char *>(p) ;
		bytes_in_buffer = imagebuffer::row_size(m_p) ;
		++m_p ;
	}
}

void Gr::JpegBufferSource::skip_input( j_decompress_ptr cinfo , long n )
{
	JpegBufferSource * src = static_cast<JpegBufferSource*>( cinfo->src ) ;
	G_ASSERT( src != nullptr && src->init_source == init_source ) ;

	if( n < 0 || static_cast<unsigned long>(n) > std::numeric_limits<size_t>::max() ) 
		throw Gr::Jpeg::Error( "skip_input error" ) ;
	src->skip_input_imp( static_cast<size_t>(n) ) ;
}

void Gr::JpegBufferSource::skip_input_imp( size_t n )
{
	while( n > bytes_in_buffer )
	{
		n -= bytes_in_buffer ;
		fill_buffer_imp() ;
	}
	next_input_byte += n ;
	bytes_in_buffer -= n ;
}

void Gr::JpegBufferSource::term_source( j_decompress_ptr cinfo )
{
}

// ==

/// \class Gr::JpegBufferDestination
/// A libjpeg 'decompression destination manager' that writes to Gr::ImageBuffer.
///
class Gr::JpegBufferDestination : public jpeg_destination_mgr
{
public:
	static JpegBufferDestination * install( j_compress_ptr cinfo , Gr::ImageBuffer & b ) ;

private:
	explicit JpegBufferDestination( Gr::ImageBuffer & b ) ;
	static void init_destination( j_compress_ptr ) ;
	static boolean empty_output_buffer( j_compress_ptr cinfo ) ;
	static void term_destination( j_compress_ptr ) ;
	void next() ;
	void set() ;
	void close() ;

private:
	ImageBuffer & m_b ;
	traits::imagebuffer<ImageBuffer>::row_iterator m_p ;
	size_t m_buffer_size ;
} ;

Gr::JpegBufferDestination * Gr::JpegBufferDestination::install( j_compress_ptr cinfo , Gr::ImageBuffer & b )
{
	JpegBufferDestination * p = new JpegBufferDestination( b ) ;
	cinfo->dest = p ;
	cinfo->dest->init_destination = init_destination ;
	cinfo->dest->empty_output_buffer = empty_output_buffer ;
	cinfo->dest->term_destination = term_destination ;
	return p ;
}

Gr::JpegBufferDestination::JpegBufferDestination( Gr::ImageBuffer & b ) :
	m_b(b) ,
	m_p(imagebuffer::row_begin(b)) ,
	m_buffer_size(G::limits::file_buffer)
{
	if( G::Test::enabled("tiny-image-buffers") )
		m_buffer_size = 10U ;

	set() ;
}

void Gr::JpegBufferDestination::next()
{
	++m_p ;
	set() ;
}

void Gr::JpegBufferDestination::set()
{
	if( m_p == imagebuffer::row_end(m_b) )
	{
		m_p = imagebuffer::row_add( m_b ) ;
	}

	if( imagebuffer::row_empty(m_p) )
		imagebuffer::row_resize( m_p , m_buffer_size ) ;

	char * p = imagebuffer::row_ptr( m_p ) ;
	next_output_byte = reinterpret_cast<JOCTET*>(p) ;
	free_in_buffer = imagebuffer::row_size( m_p ) ;
}

void Gr::JpegBufferDestination::close()
{
	if( m_p != imagebuffer::row_end(m_b) )
	{
		size_t residue = static_cast<size_t>(free_in_buffer) ;
		G_ASSERT( residue <= imagebuffer::row_size(m_p) ) ;
		residue = std::min( imagebuffer::row_size(m_p) , residue ) ;
		imagebuffer::row_resize( m_p , imagebuffer::row_size(m_p) - residue ) ;
		if( !imagebuffer::row_empty(m_p) ) ++m_p ;
		imagebuffer::row_erase( m_b , m_p ) ;
	}
}

void Gr::JpegBufferDestination::init_destination( j_compress_ptr cinfo )
{
}

boolean Gr::JpegBufferDestination::empty_output_buffer( j_compress_ptr cinfo )
{
	JpegBufferDestination * dst = static_cast<JpegBufferDestination*>( cinfo->dest ) ;
	G_ASSERT( dst != nullptr && dst->init_destination == init_destination ) ;
	dst->next() ;
	return TRUE ;
}

void Gr::JpegBufferDestination::term_destination( j_compress_ptr cinfo )
{
	JpegBufferDestination * dst = static_cast<JpegBufferDestination*>( cinfo->dest ) ;
	G_ASSERT( dst != nullptr && dst->init_destination == init_destination ) ;
	dst->close() ;
}

// ==

bool Gr::Jpeg::available()
{
	return true ;
}

/// \class Gr::JpegReaderImp
/// A private implementation class for Gr::JpegReader.
///
class Gr::JpegReaderImp
{
public:
	JpegReaderImp() ;
	~JpegReaderImp() ;
	void decode( ImageData & , FILE * , int scale , bool monochrome_out ) ;
	void decode( ImageData & , const unsigned char * , size_t , int scale , bool monochrome_out ) ;
	void decode( ImageData & , const ImageBuffer & , int scale , bool monochrome_out ) ;

private:
	JpegReaderImp( const JpegReaderImp & ) ;
	void operator=( const JpegReaderImp & ) ;
	void createBuffers( ImageData & ) ;
	jpeg_decompress_struct * p() ;
	const jpeg_decompress_struct * p() const ;
	j_common_ptr base() ;
	int channels() const ;
	void start( FILE * , int , bool ) ;
	void start( const unsigned char * , size_t , int , bool ) ;
	void start( const ImageBuffer & , int , bool ) ;
	void finish() ;
	void readPixels( ImageData & ) ;
	void startImp() ;
	void configure( int , bool ) ;
	void readGeometry() ;
	void reduce( ImageData & , int , bool ) ;
	static int pre( int ) ;
	static int post( int ) ;

private:
	int m_dx ;
	int m_dy ;
	jpeg_error_mgr m_err ;
	jpeg_decompress_struct m ;
	std::vector<unsigned char> m_line_buffer ;
	unique_ptr<JpegBufferSource> m_buffer_source ;
} ;

// ==

namespace
{
	class FileStream
	{
		public:
			FileStream( const G::Path & path , const char * mode ) :
				m_fp(nullptr)
			{
				{
					G::Root claim_root ;
					m_fp = std::fopen( path.str().c_str() , mode ) ;
				}
				if( m_fp == nullptr )
					throw Gr::Jpeg::Error( "cannot open file" ) ;
			}
			FILE * fp()
			{
				return m_fp ;
			}
			~FileStream()
			{
				std::fclose( m_fp ) ;
			}
		private:
			FileStream( const FileStream & ) ;
			void operator=( const FileStream & ) ;
		private:
			FILE * m_fp ;
	} ;
}

// ==

Gr::JpegReaderImp::JpegReaderImp() :
	m_dx(0) ,
	m_dy(0)
{
	m.err = jpeg_std_error( &m_err ) ;
	m_err.error_exit = error ;
	m_err.output_message = outputMessage ;
	jpeg_create_decompress( &m ) ;
}

Gr::JpegReaderImp::~JpegReaderImp()
{
	jpeg_destroy_decompress( &m ) ;
}

void Gr::JpegReaderImp::decode( ImageData & out , FILE * fp , int scale , bool monochrome_out )
{
	start( fp , pre(scale) , monochrome_out ) ;
	readGeometry() ;
	createBuffers( out ) ;
	readPixels( out ) ;
	reduce( out , post(scale) , monochrome_out ) ;
	finish() ;
}

void Gr::JpegReaderImp::decode( ImageData & out , const unsigned char * p , size_t n , int scale , bool monochrome_out )
{
	start( p , n , pre(scale) , monochrome_out ) ;
	readGeometry() ;
	createBuffers( out ) ;
	readPixels( out ) ;
	reduce( out , post(scale) , monochrome_out ) ;
	finish() ;
}

void Gr::JpegReaderImp::decode( ImageData & out , const ImageBuffer & b , int scale , bool monochrome_out )
{
	start( b , pre(scale) , monochrome_out ) ;
	readGeometry() ;
	createBuffers( out ) ;
	readPixels( out ) ;
	reduce( out , post(scale) , monochrome_out ) ;
	finish() ;
}

int Gr::JpegReaderImp::pre( int scale )
{
	if( scale == 1 )
		return 1 ;
	else if( scale == 2 || scale == 4 || scale == 8 )
		return scale ;
	else if( (scale & 7) == 0 )
		return 8 ;
	else
		return 1 ;
}

int Gr::JpegReaderImp::post( int scale )
{
	if( scale == 1 )
		return 1 ;
	else if( scale == 2 || scale == 4 || scale == 8 )
		return 1 ;
	else if( (scale & 7) == 0 )
		return scale >> 3 ;
	else
		return scale ;
}

void Gr::JpegReaderImp::createBuffers( ImageData & data )
{
	data.resize( m_dx , m_dy , 3 ) ;
}

void Gr::JpegReaderImp::readGeometry()
{
	m_dx = m.output_width ;
	m_dy = m.output_height ;
	if( m.output_components != 3 && m.output_components != 1 )
		throw Jpeg::Error( "unsupported number of channels" ) ;
}

jpeg_decompress_struct * Gr::JpegReaderImp::p()
{
	return &m ;
}

const jpeg_decompress_struct * Gr::JpegReaderImp::p() const
{
	return &m ;
}

j_common_ptr Gr::JpegReaderImp::base()
{
	return reinterpret_cast<j_common_ptr>(p()) ;
}

int Gr::JpegReaderImp::channels() const
{
	return m.output_components ;
}

void Gr::JpegReaderImp::start( FILE * fp , int scale , bool monochrome_out )
{
	jpeg_stdio_src( &m , fp ) ;
	jpeg_read_header( &m , TRUE ) ;
	configure( scale , monochrome_out ) ;
	startImp() ;
}

void Gr::JpegReaderImp::start( const unsigned char * p , size_t n , int scale , bool monochrome_out )
{
	unsigned char * ncp = const_cast<unsigned char*>(p) ;
	jpeg_mem_src( &m , ncp , n ) ;
	jpeg_read_header( &m , TRUE ) ;
	configure( scale , monochrome_out ) ;
	startImp() ;
}

void Gr::JpegReaderImp::start( const ImageBuffer & b , int scale , bool monochrome_out )
{
	m_buffer_source.reset( JpegBufferSource::install(&m,b) ) ;
	jpeg_read_header( &m , TRUE ) ;
	configure( scale , monochrome_out ) ;
	startImp() ;
}

void Gr::JpegReaderImp::configure( int scale , bool monochrome_out )
{
	m.out_color_space = monochrome_out ? JCS_GRAYSCALE : JCS_RGB ;
	m.scale_num = 1 ; // numerator
	m.scale_denom = scale ; // denominator
	//m.dct_method = JDCT_FASTEST ;
}

void Gr::JpegReaderImp::readPixels( ImageData & data_out )
{
	G_ASSERT( m_dx == data_out.dx() && m_dy == data_out.dy() ) ;

	jpeg_decompress_struct * ptr = p() ;
	const unsigned int dy = ptr->output_height ;
	if( data_out.channels() == m.output_components ) // optimisation
	{
		unsigned char ** row_pp = data_out.rowPointers() ;
		for( unsigned int y = 0U ; ptr->output_scanline < dy ; y++ , row_pp++ )
		{
			unsigned char * row_p = *row_pp ;
			jpeg_read_scanlines( ptr , &row_p , 1 ) ;
		}
	}
	else
	{
		m_line_buffer.resize( m.output_width * m.output_components ) ;
		unsigned char * line_buffer_p = &m_line_buffer[0] ;
		for( unsigned int y = 0U ; ptr->output_scanline < dy ; y++ )
		{
			jpeg_read_scanlines( ptr , &line_buffer_p , 1 ) ;
			data_out.copyRowIn( y , &m_line_buffer[0] , m_line_buffer.size() , m.output_components , false/*sic*/ ) ;
		}
		m_line_buffer.clear() ;
	}
}

void Gr::JpegReaderImp::reduce( ImageData & data_out , int post_scale , bool monochrome_out )
{
	G_ASSERT( data_out.channels() == 3 ) ;
	if( post_scale > 1 || monochrome_out )
	{
		data_out.scale( post_scale , monochrome_out , false/*since decoded to greyscale*/ ) ;
	}
}

void Gr::JpegReaderImp::startImp()
{
	jpeg_start_decompress( &m ) ;
}

void Gr::JpegReaderImp::finish()
{
	jpeg_finish_decompress( &m ) ;
}

// ==

Gr::JpegReader::JpegReader( int scale , bool monochrome_out ) :
	m_scale(scale) ,
	m_monochrome_out(monochrome_out)
{
}

void Gr::JpegReader::setup( int scale , bool monochrome_out )
{
	m_scale = scale ;
	m_monochrome_out = monochrome_out ;
}

void Gr::JpegReader::decode( ImageData & out , const G::Path & path )
{
	if( m_imp.get() == nullptr ) m_imp.reset( new JpegReaderImp ) ;
	FileStream file( path , "rb" ) ;
	m_imp->decode( out , file.fp() , m_scale , m_monochrome_out ) ;
}

void Gr::JpegReader::decode( ImageData & out , const unsigned char * p , size_t n )
{
	if( m_imp.get() == nullptr ) m_imp.reset( new JpegReaderImp ) ;
	m_imp->decode( out , p , n , m_scale , m_monochrome_out ) ;
}

void Gr::JpegReader::decode( ImageData & out , const char * p , size_t n )
{
	decode( out , reinterpret_cast<const unsigned char*>(p) , n ) ;
}

void Gr::JpegReader::decode( ImageData & out , const ImageBuffer & b )
{
	if( m_imp.get() == nullptr ) m_imp.reset( new JpegReaderImp ) ;
	m_imp->decode( out , b , m_scale , m_monochrome_out ) ;
}

Gr::JpegReader::~JpegReader()
{
}

// ==

/// \class Gr::JpegWriterImp
/// A private pimple class for Gr::JpegWriter.
///
class Gr::JpegWriterImp
{
public:
	JpegWriterImp( int scale , bool monochrome_out ) ;
	~JpegWriterImp() ;
	void setup( int scale , bool monochrome_out ) ;
	void encode( const ImageData & , const G::Path & path_out ) ;
	void encode( const ImageData & , std::vector<char> & out ) ;
	void encode( const ImageData & , ImageBuffer & out ) ;

private:
	JpegWriterImp( const JpegWriterImp & ) ;
	void operator=( const JpegWriterImp & ) ;
	void init( const ImageData & ) ;
	void compress( const ImageData & ) ;

private:
	int m_scale ;
	bool m_monochrome_out ;
	jpeg_error_mgr m_err ;
	jpeg_compress_struct m ;
	unique_ptr<JpegBufferDestination> m_buffer_destination ;
	std::vector<char> m_line_buffer ;
} ;

Gr::JpegWriterImp::JpegWriterImp( int scale , bool monochrome_out ) :
	m_scale(scale) ,
	m_monochrome_out(monochrome_out)
{
	m.err = jpeg_std_error( &m_err ) ;
	m_err.error_exit = error ;
	m_err.output_message = outputMessage ;
	jpeg_create_compress( &m ) ;
}

void Gr::JpegWriterImp::init( const ImageData & in )
{
	m.image_height = scaled( in.dy() , m_scale ) ;
	m.image_width = scaled( in.dx() , m_scale ) ;
	m.input_components = (in.channels()==1 || m_monochrome_out) ? 1 : 3 ;
	m.in_color_space = (in.channels()==1 || m_monochrome_out) ? JCS_GRAYSCALE : JCS_RGB ;
	jpeg_set_defaults( &m ) ;

	if( G::Test::enabled("jpeg-quality-high") ) jpeg_set_quality( &m , 100 , TRUE ) ;
	if( G::Test::enabled("jpeg-quality-low") ) jpeg_set_quality( &m , 20 , TRUE ) ;
}

Gr::JpegWriterImp::~JpegWriterImp()
{
	jpeg_destroy_compress( &m ) ;
}

void Gr::JpegWriterImp::setup( int scale , bool monochrome_out )
{
	m_scale = scale ;
	m_monochrome_out= monochrome_out ;
}

void Gr::JpegWriterImp::encode( const ImageData & in , const G::Path & path )
{
	if( path == G::Path() ) throw Gr::Jpeg::Error( "empty filename" ) ;
	init( in ) ;
	FileStream file( path.str() , "wb" ) ;
	jpeg_stdio_dest( &m , file.fp() ) ;
	compress( in ) ;
}

void Gr::JpegWriterImp::encode( const ImageData & in , std::vector<char> & out )
{
	init( in ) ;
	unsigned char * p = out.empty() ? nullptr : reinterpret_cast<unsigned char*>(&out[0]) ;
	unsigned long n = out.size() ;
	m.dest = nullptr ; // sic
	jpeg_mem_dest( &m , &p , &n ) ;
	compress( in ) ;
	if( p == reinterpret_cast<unsigned char*>(&out[0]) )
	{
		G_ASSERT( n <= out.size() ) ; 
		out.resize( n ) ;
	}
	else
	{
		G_DEBUG( "Gr::JpegWriterImp::write: copying from libjpeg re-allocation" ) ;
		G_ASSERT( p != nullptr && n > 0 ) ; if( p == nullptr ) throw Jpeg::Error() ;
		out.resize( n ) ;
		std::memcpy( &out[0] , p , n ) ;
		std::free( p ) ;
	}
}

void Gr::JpegWriterImp::encode( const ImageData & in , ImageBuffer & out )
{
	init( in ) ;
	m_buffer_destination.reset( JpegBufferDestination::install(&m,out) ) ;
	compress( in ) ;
}

void Gr::JpegWriterImp::compress( const ImageData & in )
{
	jpeg_start_compress( &m , TRUE ) ;
	if( m_scale == 1 && !m_monochrome_out ) // optimisation
	{
		while( m.next_scanline < m.image_height )
		{
			jpeg_byte * row_pointer = const_cast<jpeg_byte*>(in.rowPointers()[m.next_scanline]) ;
			jpeg_write_scanlines( &m , &row_pointer , 1 ) ;
		}
	}
	else
	{
		m_line_buffer.resize( sizet(in.dx(),m_monochrome_out?1:in.channels()) ) ;
		while( m.next_scanline < m.image_height )
		{
			int y_in = m.next_scanline * m_scale ; G_ASSERT( y_in < in.dy() ) ;
			in.copyRowOut( y_in , m_line_buffer , m_scale , m_monochrome_out ) ;
			jpeg_byte * row_pointer = reinterpret_cast<unsigned char *>(&m_line_buffer[0]) ;
			jpeg_write_scanlines( &m , &row_pointer , 1 ) ;
		}
	}
	jpeg_finish_compress( &m ) ;
}

// ==

Gr::JpegWriter::JpegWriter( int scale , bool monochrome_out ) :
	m_imp(new JpegWriterImp(scale,monochrome_out))
{
}

Gr::JpegWriter::~JpegWriter()
{
}

void Gr::JpegWriter::setup( int scale , bool monochrome_out )
{
	m_imp->setup( scale , monochrome_out ) ;
}

void Gr::JpegWriter::encode( const ImageData & in , const G::Path & path_out )
{
	m_imp->encode( in , path_out ) ;
}

void Gr::JpegWriter::encode( const ImageData & in , std::vector<char> & buffer_out )
{
	m_imp->encode( in , buffer_out ) ;
}

void Gr::JpegWriter::encode( const ImageData & in , ImageBuffer & image_buffer_out ) 
{
	m_imp->encode( in , image_buffer_out ) ;
}


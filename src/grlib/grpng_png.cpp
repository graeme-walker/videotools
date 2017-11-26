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
// grpng_png.cpp
//

#include "gdef.h"
#include "grdef.h"
#include "grpng.h"
#include "groot.h"
#include "gdebug.h"
#include <png.h>
#include <cstring>
#include <cstdio> // std::tmpfile()
#include <algorithm>

typedef int static_assert_png_byte_is_char[sizeof(png_byte)==1?1:-1] ;

// temporary compile-time fix for mac
#ifndef PNG_TRANSFORM_GRAY_TO_RGB
#define PNG_TRANSFORM_GRAY_TO_RGB 0
#endif

#if GCONFIG_HAVE_FMEMOPEN
namespace
{
	FILE * fmemopen_( void * p , size_t n , const char * mode )
	{
		return fmemopen( p , n , mode ) ;
	}
}
#else
namespace
{
	FILE * fmemopen_( void * p , size_t n , const char * )
	{
		FILE * fp = std::tmpfile() ;
		if( fp )
		{
			size_t rc = std::fwrite( p , n , 1U , fp ) ;
			if( rc != 1U )
			{
				std::fclose( fp ) ;
				return nullptr ;
			}
			std::fseek( fp , 0 , SEEK_SET ) ;
		}
		return fp ;
	}
}
#endif

bool Gr::Png::available()
{
	return true ;
}

/// \class Gr::PngImp
/// A private base class that manages png_struct and png_info structures,
/// and handles error callbacks.
///
class Gr::PngImp
{
public:
	struct FileCloser /// RAII class to do std::fclose().
	{
		FILE * m_fp ;
		explicit FileCloser( FILE * fp ) : m_fp(fp) {}
		~FileCloser() { if( m_fp ) std::fclose( m_fp ) ; }
	} ;

public:
	explicit PngImp( bool reader ) ;
	~PngImp() ;
	void pngReset() ;

private:
	bool m_reader ;

protected:
	png_struct * m_png_struct ;
	png_info * m_png_info ;

private:
	PngImp( const PngImp & ) ;
	void operator=( const PngImp & ) ;
	void init_( bool ) ;
	void cleanup_() ;
	static void error_fn( png_struct * , const char * ) ;
	static void warning_fn( png_struct * , const char * ) ;
} ;

/// \class Gr::PngReaderImp
/// A private pimple-pattern class for Gr::PngReader.
///
class Gr::PngReaderImp : private PngImp
{
public:
	typedef PngReader::Map Map ;
	PngReaderImp() ;
	~PngReaderImp() ;
	void decode( ImageData & , const G::Path & path , int scale , bool monochrome_out ) ;
	void decode( ImageData & , const char * data_p , size_t data_size , int scale , bool monochrome_out ) ;
	void decode( ImageData & , const ImageBuffer & , int scale , bool monochrome_out ) ;
	Map tags() const ;

private:
	PngReaderImp( const PngReaderImp & ) ;
	void operator=( const PngReaderImp & ) ;
	void decodeImp( ImageData & , FILE * , const ImageBuffer * , int scale , bool monochrome_out ) ;
	void init_io( png_struct * , FILE * ) ;
	static void read( png_struct * , unsigned char * p , size_t ) ;
	void reset( FILE * fp ) ;
	void reset( const ImageBuffer * bp ) ;

private:
	typedef Gr::traits::imagebuffer<ImageBuffer>::streambuf_type imagebuf ;
	Map m_tags ;
	unique_ptr<imagebuf> m_read_state ;
} ;

/// \class Gr::PngTagsImp
/// A Gr::PngWriter implementation class that holds tags.
///
class Gr::PngTagsImp
{
public:
	typedef std::multimap<std::string,std::string> Map ;
	PngTagsImp() ;
	explicit PngTagsImp( const Map & tags ) ;
	int n() const ;
	png_text * p() const ;

private:
	PngTagsImp( const PngTagsImp & ) ;
	void operator=( const PngTagsImp & ) ;

private:
	std::vector<std::string> m_strings ;
	std::vector<png_text> m_text ;
} ;

/// \class Gr::PngWriterImp
/// A private pimple-pattern class for Gr::PngWriter.
///
class Gr::PngWriterImp : private PngImp
{
public:
	typedef PngWriter::Map Map ;
	typedef PngWriter::Output Output ;
	PngWriterImp( const ImageData & , Map tags ) ;
	void write( const G::Path & path ) ;
	void write( Output & out ) ;

private:
	static void flush_imp( png_struct * ) ;
	static void write_imp( png_struct * p , png_byte * data , png_size_t n ) ;

private:
	const ImageData & m_data ;
	PngTagsImp m_tags ;
} ;

// ==

Gr::PngImp::PngImp( bool reader ) :
	m_reader(reader) ,
	m_png_struct(nullptr) ,
	m_png_info(nullptr)
{
	init_( reader ) ;
}

void Gr::PngImp::init_( bool reader )
{
	m_png_struct = reader ?
		png_create_read_struct( PNG_LIBPNG_VER_STRING , reinterpret_cast<png_voidp>(this) , error_fn , warning_fn ) :
		png_create_write_struct( PNG_LIBPNG_VER_STRING , reinterpret_cast<png_voidp>(this) , error_fn , warning_fn ) ;

	if( m_png_struct == nullptr )
		throw Png::Error( "png_create_struct failed" ) ;

	m_png_info = png_create_info_struct( m_png_struct ) ;
	if( m_png_info == nullptr )
	{
		cleanup_() ;
		throw Png::Error( "png_create_info_struct failed" ) ;
	}
}

void Gr::PngImp::pngReset()
{
	cleanup_() ;
	init_( m_reader ) ;
}

Gr::PngImp::~PngImp()
{
	cleanup_() ;
}

void Gr::PngImp::cleanup_()
{
	if( m_reader )
		png_destroy_read_struct( &m_png_struct , &m_png_info , nullptr ) ;
	else
		png_destroy_write_struct( &m_png_struct , &m_png_info ) ;

	m_png_struct = nullptr ;
	m_png_info = nullptr ;
}

void Gr::PngImp::error_fn( png_struct * p , const char * what )
{
	G_DEBUG( "Gr::PngImp::error: error callback from libpng: " << what ) ;
#if PNG_LIBPNG_VER_MAJOR > 1 || PNG_LIBPNG_VER_MINOR >= 5
	png_longjmp( p , 99 ) ; // cannot throw :-<
#else
	longjmp( p->jmpbuf , 99 ) ; // cannot throw :-<
#endif
}

void Gr::PngImp::warning_fn( png_struct * , const char * what )
{
	G_DEBUG( "Gr::PngImp::warning: warning callback from libpng: " << what ) ;
}

// ==

void Gr::PngInfo::init( std::istream & stream )
{
	unsigned char buffer[31] = { 0 } ;
	stream.read( reinterpret_cast<char*>(buffer) , sizeof(buffer) ) ;
	std::pair<int,int> pair = parse( buffer , stream.gcount() ) ;
	m_dx = pair.first ;
	m_dy = pair.second ;
}

void Gr::PngInfo::init( const unsigned char * p_in , size_t n )
{
	std::pair<int,int> pair = parse( p_in , n ) ;
	m_dx = pair.first ;
	m_dy = pair.second ;
}

// ==

Gr::PngReader::PngReader( int scale , bool monochrome_out ) :
	m_scale(scale) ,
	m_monochrome_out(monochrome_out)
{
}

void Gr::PngReader::setup( int scale , bool monochrome_out )
{
	m_scale = scale ;
	m_monochrome_out = monochrome_out ;
}

void Gr::PngReader::decode( ImageData & out , const G::Path & path )
{
	PngReaderImp imp ;
	imp.decode( out , path , m_scale , m_monochrome_out ) ;
	m_tags = imp.tags() ;
}

void Gr::PngReader::decode( ImageData & out , const char * p , size_t n )
{
	PngReaderImp imp ;
	imp.decode( out , p , n , m_scale , m_monochrome_out ) ;
	m_tags = imp.tags() ;
}

void Gr::PngReader::decode( ImageData & out , const unsigned char * p , size_t n )
{
	decode( out , reinterpret_cast<const char*>(p) , n ) ;
}

void Gr::PngReader::decode( ImageData & out , const ImageBuffer & b )
{
	PngReaderImp imp ;
	imp.decode( out , b , m_scale , m_monochrome_out ) ;
	m_tags = imp.tags() ;
}

Gr::PngReader::~PngReader()
{
}

Gr::PngReader::Map Gr::PngReader::tags() const
{
	return m_tags ;
}

// ==

Gr::PngReaderImp::PngReaderImp() :
	PngImp(true)
{
}

Gr::PngReaderImp::~PngReaderImp()
{
}

void Gr::PngReaderImp::decode( ImageData & out , const char * p , size_t n , int scale , bool monochrome_out )
{
	FILE * fp = fmemopen_( const_cast<char*>(p) , n , "rb" ) ;
	if( fp == nullptr ) 
		throw Png::Error( "fmemopen failed" ) ;
	PngImp::FileCloser closer( fp ) ;
	decodeImp( out , fp , nullptr , scale , monochrome_out ) ;
}

void Gr::PngReaderImp::decode( ImageData & out , const G::Path & path , int scale , bool monochrome_out )
{
	FILE * fp = nullptr ;
	{
		G::Root claim_root ;
		fp = std::fopen( path.str().c_str() , "rb" ) ;
	}
	if( fp == nullptr )
		throw Png::Error( "cannot open png file" , path.str() ) ;
	PngImp::FileCloser closer( fp ) ;
	decodeImp( out , fp , nullptr , scale , monochrome_out ) ;
}

void Gr::PngReaderImp::decode( ImageData & out , const ImageBuffer & image_buffer , int scale , bool monochrome_out )
{
	decodeImp( out , nullptr , &image_buffer , scale , monochrome_out ) ;
}

void Gr::PngReaderImp::init_io( png_struct * png , FILE * fp )
{
	if( fp != nullptr )
		png_init_io( png , fp ) ;
	else
		png_set_read_fn( png , this , &PngReaderImp::read ) ;
}

void Gr::PngReaderImp::decodeImp( ImageData & out , FILE * fp , const ImageBuffer * bp , int scale , bool monochrome_out )
{
	if( fp == nullptr && bp == nullptr )
		return ;

	if( bp != nullptr )
		m_read_state.reset( new imagebuf(*bp) ) ;

	const int transforms =
		PNG_TRANSFORM_STRIP_16 | // chop 16-bits down to eight
		PNG_TRANSFORM_STRIP_ALPHA | // discard alpha channel
		PNG_TRANSFORM_PACKING | // expand 1, 2 and 4 bits to eight
		PNG_TRANSFORM_EXPAND | // set_expand()
		PNG_TRANSFORM_GRAY_TO_RGB ; // expand greyscale to RGB

	// get the image size and allocate storage
	{
		bool ok = false ;
		int dx = 0 ;
		int dy = 0 ;
		if( ! setjmp( png_jmpbuf(m_png_struct) ) )
		{
			try
			{
				init_io( m_png_struct , fp ) ;
				png_read_info( m_png_struct , m_png_info ) ;
				dx = png_get_image_width( m_png_struct , m_png_info ) ;
				dy = png_get_image_height( m_png_struct , m_png_info ) ;
				ok = true ;
			}
			catch(...) // setjmp/longjmp
			{
			}
		}
		if( !ok || dx <= 0 || dy <= 0 )
			throw Png::Error( "png_read_info failed" ) ;
		out.resize( dx , dy , 3 ) ;
		pngReset() ; // PngImp::pngReset()
		reset( fp ) ;
		reset( bp ) ;

	}

	// decode
	{
		bool ok = false ;
		png_text * text_p = nullptr ;
		int text_n = 0 ;
		png_byte ** out_pp = out.rowPointers() ;
		if( ! setjmp( png_jmpbuf(m_png_struct) ) )
		{
			try
			{
				init_io( m_png_struct , fp ) ;
				png_set_rows( m_png_struct , m_png_info , out_pp ) ; // use our storage
				png_read_png( m_png_struct , m_png_info , transforms , nullptr ) ;
				png_get_text( m_png_struct , m_png_info , &text_p , &text_n ) ;
				int depth = png_get_bit_depth( m_png_struct , m_png_info ) ;
				ok = depth == 8 ;
			}
			catch(...) // setjmp/longjmp
			{
			}
		}
		if( !ok )
			throw Png::Error( "png_read_png failed" ) ;
		for( int i = 0 ; i < text_n ; i++ )
		{
			if( text_p && text_p[i].compression == PNG_TEXT_COMPRESSION_NONE )
				m_tags.insert( Map::value_type(text_p[i].key,text_p[i].text) ) ;
		}
	}

	// scale the output (optimisation opportunity here to not do the colourspace
	// stuff if the image was monochrome to start with)
	//
	out.scale( scale , monochrome_out , true ) ; 
}

void Gr::PngReaderImp::read( png_struct * png , unsigned char * p , size_t n )
{
	PngReaderImp * imp = static_cast<PngReaderImp*>( png_get_io_ptr( png ) ) ;
	if( imp == nullptr || imp->m_read_state.get() == nullptr || p == nullptr )
		png_error( png , "read error" ) ;

	size_t rc = static_cast<size_t>( imp->m_read_state->sgetn( reinterpret_cast<char*>(p) , n ) ) ;
	if( rc != n )
		png_error( png , "read error" ) ;
}

Gr::PngReader::Map Gr::PngReaderImp::tags() const
{
	return m_tags ;
}

void Gr::PngReaderImp::reset( FILE * fp )
{
	int rc = fp == nullptr ? 0 : std::fseek( fp , 0 , SEEK_SET ) ;
	if( rc != 0 )
		throw Png::Error( "fseek error" ) ;
}

void Gr::PngReaderImp::reset( const ImageBuffer * bp )
{
	if( bp != nullptr )
		m_read_state.reset( new imagebuf(*bp) ) ;
}

// ==

Gr::PngTagsImp::PngTagsImp()
{
}

Gr::PngTagsImp::PngTagsImp( const Map & tags )
{
	m_text.reserve( tags.size() ) ;
	Map::const_iterator const end = tags.end() ;
	for( Map::const_iterator p = tags.begin() ; p != end ; ++p )
	{
		if( (*p).first.length() == 0U || (*p).first.length() > 78 )
			throw Png::Error( "invalid tag key" ) ;
		static png_text zero ;
		png_text text( zero ) ;
		text.compression = PNG_TEXT_COMPRESSION_NONE ;
		m_strings.push_back( (*p).first + '\0' ) ;
		text.key = const_cast<char*>(m_strings.back().data()) ;
		m_strings.push_back( (*p).second + '\0' ) ;
		text.text = const_cast<char*>(m_strings.back().data()) ;
		m_text.push_back( text ) ;
	}
}

int Gr::PngTagsImp::n() const
{
	return m_text.size() ;
}

png_text * Gr::PngTagsImp::p() const
{
	typedef std::vector<png_text> Vector ;
	Vector & text = const_cast<Vector&>(m_text) ;
	return m_text.size() ? &text[0] : nullptr ;
}

// ==

Gr::PngWriter::PngWriter( const ImageData & data , Map tags ) :
	m_imp(new PngWriterImp(data,tags))
{
}

Gr::PngWriter::~PngWriter()
{
	delete m_imp ;
}

void Gr::PngWriter::write( const G::Path & path )
{
	m_imp->write( path ) ;
}

void Gr::PngWriter::write( Output & out )
{
	m_imp->write( out ) ;
}

// ==

Gr::PngWriterImp::PngWriterImp( const ImageData & data , Map tags_in ) :
	PngImp(false) ,
	m_data(data) ,
	m_tags(tags_in)
{
	bool ok = false ;
	{
		png_uint_32 dx = static_cast<png_uint_32>( m_data.dx() ) ;
		png_uint_32 dy = static_cast<png_uint_32>( m_data.dy() ) ;
		png_byte ** row_pointers = const_cast<ImageData&>(m_data).rowPointers() ;
		png_text * tags_p = m_tags.p() ;
		int tags_n = m_tags.n() ;

		if( ! setjmp( png_jmpbuf(m_png_struct) ) )
		{
			try
			{
				png_set_IHDR( m_png_struct , m_png_info , dx , dy , 8 , 
					data.channels() == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY ,
					PNG_INTERLACE_NONE , PNG_COMPRESSION_TYPE_DEFAULT , PNG_FILTER_TYPE_DEFAULT ) ;
				png_set_rows( m_png_struct , m_png_info , row_pointers ) ;
				if( tags_n != 0 )
					png_set_text( m_png_struct , m_png_info , tags_p , tags_n ) ;
				ok = true ;
			}
			catch(...) // setjmp/longjmp
			{
			}
		}
	}
	if( !ok )
		throw Png::Error( "png_set_rows failed" ) ;
}

void Gr::PngWriterImp::write( const G::Path & path )
{
	FILE * fp = nullptr ;
	{
		G::Root claim_root ;
		fp = std::fopen( path.str().c_str() , "wb" ) ;
	}
	if( fp == nullptr )
		throw Png::Error( "cannot create output" , path.str() ) ;
	PngImp::FileCloser closer( fp ) ;

	bool ok = false ;
	{
		if( ! setjmp( png_jmpbuf(m_png_struct) ) )
		{
			try
			{
				png_init_io( m_png_struct , fp ) ;
				png_write_png( m_png_struct , m_png_info , PNG_TRANSFORM_PACKING , nullptr ) ;
				ok = true ;
			}
			catch(...) // setjmp/longjmp
			{
			}
		}
	}
	if( !ok )
		throw Png::Error( "png_write_png failed" ) ;
}

void Gr::PngWriterImp::write( Gr::PngWriter::Output & out )
{
	bool ok = false ;
	{
		if( ! setjmp( png_jmpbuf(m_png_struct) ) )
		{
			try
			{
				png_set_write_fn( m_png_struct , reinterpret_cast<void*>(&out) , write_imp , flush_imp ) ;
				png_write_png( m_png_struct , m_png_info , PNG_TRANSFORM_PACKING , nullptr ) ;
				ok = true ;
			}
			catch(...) // setjmp/longjmp
			{
			}
		}
	}
	if( !ok )
		throw Png::Error( "png_write_png failed" ) ;
}

void Gr::PngWriterImp::write_imp( png_struct * p , png_byte * data , png_size_t n )
{
	Output * out = reinterpret_cast<Output*>( png_get_io_ptr(p) ) ;
	(*out)( data , n ) ;
}

void Gr::PngWriterImp::flush_imp( png_struct * )
{
}


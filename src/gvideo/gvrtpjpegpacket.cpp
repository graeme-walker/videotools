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
// gvrtpjpegpacket.cpp
//

#include "gdef.h"
#include "gvrtpjpegpacket.h"
#include "gassert.h"
#include "ghexdump.h"
#include <algorithm>
#include <stdexcept>

size_t Gv::RtpJpegPacket::smallest()
{
	return 8U ;
}

Gv::RtpJpegPacket::RtpJpegPacket( const char * begin , const char * end ) :
	m_p(begin) ,
	m_n(end-begin)
{
	G_ASSERT( m_n >= smallest() ) ;

	if( payloadOffset() > m_n )
		m_reason = "invalid payload offset" ;

	// we do not support types defined at session-setup
	if( type_is_dynamic() )
		m_reason = "dynamic type field" ;

	// in practice only two types are defined by RFC 2435 (although the older RFC 2035 defines more)
	if( type_base() != 0U && type_base() != 1U )
		m_reason = "unexpected type field" ;

	// TODO support in-band quantisation tables -- easy to do, but difficult to test
	if( q_is_special() )
		m_reason = "unexpected quantisation field: support for in-band tables not implemented" ;

	// (for types 0 and 1 the type-specific field is used for interlacing info)
	if( ts() != 0 )
		G_WARNING_ONCE( "ignoring rtp jpeg interlacing" ) ; // TODO deal with interlacing
}

bool Gv::RtpJpegPacket::valid() const
{
	return m_reason.empty() ;
}

std::string Gv::RtpJpegPacket::reason() const
{
	return m_reason ;
}

unsigned int Gv::RtpJpegPacket::ri() const
{
	if( type_has_restart_markers() )
		return make_word( &m_p[8U] ) ;
	else
		return 0U ;
}

unsigned int Gv::RtpJpegPacket::rc() const
{
	if( type_has_restart_markers() )
		return make_word( &m_p[10U] ) & 0x3fff ;
	else
		return 0U ;
}

std::string Gv::RtpJpegPacket::str() const
{
	std::ostringstream ss ;
	ss << "ts=" << ts() << " fo=" << fo() << " t=" << type() << " q=" << q() << " dx=" << dx() << " dy=" << dy() << " ri=" << ri() << " rc=" << rc() ;
	return ss.str() ;
}

unsigned int Gv::RtpJpegPacket::ts() const
{
	const unsigned int p0 = static_cast<unsigned char>(m_p[0]) ;
	return p0 ;
}

unsigned long Gv::RtpJpegPacket::fo() const
{
	const unsigned int p1 = static_cast<unsigned char>(m_p[1]) ;
	const unsigned int p2 = static_cast<unsigned char>(m_p[2]) ;
	const unsigned int p3 = static_cast<unsigned char>(m_p[3]) ;
	return make_dword( 0U , p1 , p2 , p3 ) ;
}

unsigned int Gv::RtpJpegPacket::type() const
{
	const unsigned int p4 = static_cast<unsigned char>(m_p[4]) ;
	return p4 ;
}

bool Gv::RtpJpegPacket::type_is_dynamic() const
{
	return type() >= 128U ; // ie. defined at session setup
}

bool Gv::RtpJpegPacket::type_has_restart_markers() const
{
	return type() >= 64U && type() <= 127U ; // ie. with restart marker header
}

unsigned int Gv::RtpJpegPacket::type_base() const
{
	return type_has_restart_markers() ? (type()-64U) : type() ;
}

unsigned int Gv::RtpJpegPacket::q() const
{
	const unsigned int p5 = static_cast<unsigned char>(m_p[5]) ;
	return p5 ;
}

bool Gv::RtpJpegPacket::q_is_special() const
{
	return !!( q() & 0x80 ) ; // ie. with quantisation tables
}

unsigned int Gv::RtpJpegPacket::dx() const
{
	const unsigned int p6 = static_cast<unsigned char>(m_p[6]) ;
	return p6 * 8U ;
}

unsigned int Gv::RtpJpegPacket::dy() const
{
	const unsigned int p7 = static_cast<unsigned char>(m_p[7]) ;
	return p7 * 8U ;
}

unsigned int Gv::RtpJpegPacket::make_word( const char * p )
{
	unsigned int p0 = static_cast<unsigned char>(p[0]) ;
	unsigned int p1 = static_cast<unsigned char>(p[1]) ;
	return (p0<<8) | p1 ;
}

unsigned long Gv::RtpJpegPacket::make_dword( unsigned long c3 , unsigned long c2 , unsigned long c1 , unsigned long c0 )
{
	return ( c3 << 24 ) | ( c2 << 16 ) | ( c1 << 8 ) | ( c0 << 0 ) ;
}

Gv::RtpJpegPacket::Payload Gv::RtpJpegPacket::payload() const
{
	Payload p ;
	p.offset = payloadOffset() ;
	p.size = payloadSize() ;
	p.begin = payloadBegin() ;
	p.end = payloadEnd() ;
	return p ;
}

unsigned int Gv::RtpJpegPacket::payloadOffset() const
{
	size_t header_size = 8U ;
	if( type_has_restart_markers() )
	{
		// allow for a Restart Marker header -- see sections 3.1.3 and 3.1.7
		header_size += 4U ; 
	}
	if( q_is_special() && fo() == 0UL )
	{ 
		// allow for a Quantization Table header and table data -- see
		// sections 3.1.4 and 3.1.8
		unsigned int qtl = make_word( &m_p[header_size+2U] ) ; 
		header_size += ( 4U + qtl ) ;
	}
	return header_size ;
}

size_t Gv::RtpJpegPacket::payloadSize() const
{
	G_ASSERT( payloadOffset() <= m_n ) ;
	return m_n - payloadOffset() ;
}

const char * Gv::RtpJpegPacket::payloadBegin() const
{
	return m_p + payloadOffset() ;
}

const char * Gv::RtpJpegPacket::payloadEnd() const
{
	return m_p + m_n ;
}

// ==

// From RFC 2435 Appendix A...

static const int table_rfc[128] = 
{
	// luma
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99,
	// chroma
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
} ;

// see rejected erratum 4094

static const int table_erratum[128] = 
{
	// luma
	16, 11, 12, 14, 12, 10, 16, 14,
	13, 14, 18, 17, 16, 19, 24, 40,
	26, 24, 22, 22, 24, 49, 35, 37,
	29, 40, 58, 51, 61, 60, 57, 51,
	56, 55, 64, 72, 92, 78, 64, 68,
	87, 69, 55, 56, 80, 109, 81, 87,
	95, 98, 103, 104, 103, 62, 77, 113,
	121, 112, 100, 120, 92, 101, 103, 99 ,
	// chroma
	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
} ;

static const int table_x[128] = 
{
	// luma
	13, 9, 10, 11, 10, 8, 13, 11,
	10, 11, 14, 14, 13, 15, 19, 32,
	21, 19, 18, 18, 19, 39, 28, 30,
	23, 32, 46, 41, 49, 48, 46, 41,
	45, 44, 51, 58, 74, 62, 51, 54,
	70, 55, 44, 45, 64, 87, 65, 70,
	76, 78, 82, 83, 82, 50, 62, 90,
	97, 90, 80, 96, 74, 81, 82, 79,
	// chroma
	14, 14, 14, 19, 17, 19, 38, 21,
	21, 38, 79, 53, 45, 53, 79, 79,
	79, 79, 79, 79, 79, 79, 79, 79,
	79, 79, 79, 79, 79, 79, 79, 79,
	79, 79, 79, 79, 79, 79, 79, 79,
	79, 79, 79, 79, 79, 79, 79, 79,
	79, 79, 79, 79, 79, 79, 79, 79,
	79, 79, 79, 79, 79, 79, 79, 79,
} ;

template <typename T>
inline
T limit( T min , T value , T max )
{
	return value < min ? min : ( value > max ? max : value ) ;
}

template <typename Titerator>
static void make_table( int q_in , Titerator out , const int * p , const int * end = nullptr )
{
	if( end == nullptr ) end = p + 128 ;
	int factor = limit( 1 , q_in , 99 ) ;
	int q = q_in < 50 ? (q_in/factor) : (200-factor*2) ;
	for( ; p != end ; ++p )
	{
		int qq = ((*p) * q + 50) / 100 ;
		*out++ = limit( 1 , qq , 255 ) ;
	}
}

template <typename Titerator>
static void make_table( Titerator out , const int * p , const int * end = nullptr )
{
	if( end == nullptr ) end = p + 128 ;
	for( ; p != end ; ++p )
	{
		*out++ = limit( 1 , *p , 255 ) ;
	}
}

namespace
{
	struct Table 
	{ 
		Table( int q , int fudge )
		{
			if( fudge == 0 ) // erratum table
			{
				m_key = key( q , fudge ) ;
				make_table( q , std::back_inserter(m_data) , table_erratum ) ;
			}
			else if( fudge == 1 ) // rfc table
			{
				m_key = key( q , fudge ) ;
				make_table( q , std::back_inserter(m_data) , table_rfc ) ;
			}
			else // ad-hoc pre-scaled table
			{
				G_ASSERT( fudge == 2 ) ;
				m_key = key( 0 , fudge ) ; // ignore q
				make_table( std::back_inserter(m_data) , table_x ) ;
			}
		}
		const unsigned char * p() const
		{
			return &m_data[0] ;
		}
		static int key( int q , int fudge )
		{
			if( fudge == 2 ) q = 0 ;
			return q + fudge*1000 ;
		}
		int key() const
		{
			return m_key ;
		}
		private: int m_key ; 
		private: std::vector<unsigned char> m_data ;
	} ;
	struct Match
	{
		explicit Match( int k ) : m_k(k) {}
		bool operator()( const Table & t ) const
		{
			return m_k == t.key() ;
		}
		int m_k ;
	} ;
}

static const unsigned char * table( int q , int fudge )
{
	static std::vector<Table> tables ; // cache, keyed by q and fudge factor
	std::vector<Table>::iterator p = std::find_if( tables.begin() , tables.end() , Match(Table::key(q,fudge)) ) ;
	if( p == tables.end() )
	{
		tables.push_back( Table(q,fudge) ) ;
		return tables.back().p() ;
	}
	else
	{
		return (*p).p() ;
	}
}

static const unsigned char * luma_table( int q , int fudge )
{
	return table( q , fudge ) ;
}

static const unsigned char * chroma_table( int q , int fudge )
{
	return table(q,fudge) + 64 ;
}

// From RFC 2435 Appendix B...

static const unsigned char lum_dc_codelens[] = {
	0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static const unsigned char lum_dc_symbols[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static const unsigned char lum_ac_codelens[] = {
	0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

static const unsigned char lum_ac_symbols[] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa,
};

static const unsigned char chm_dc_codelens[] = {
	0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

static const unsigned char chm_dc_symbols[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static const unsigned char chm_ac_codelens[] = {
	0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

static const unsigned char chm_ac_symbols[] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa,
};

template <typename T>
static T make_quant_header( T p , const unsigned char * qt , int table_no )
{
	*p++ = 0xff ;
	*p++ = 0xdb ; // DQT
	*p++ = 0 ; // length msb
	*p++ = 67 ; // length lsb
	*p++ = table_no ;
	p = std::copy( qt , qt+64 , p ) ;
	return p ;
}

template <typename T>
static T make_huffman_header( T p , const unsigned char * codelens , int ncodelens ,
	const unsigned char * symbols , int nsymbols , int table_no , int table_class )
{
	G_ASSERT( ( 3 + ncodelens + nsymbols ) <= 255 ) ;

	*p++ = 0xff ;
	*p++ = 0xc4 ; // DHT
	*p++ = 0 ; // length msb
	*p++ = 3 + ncodelens + nsymbols ; // length lsb
	*p++ = ( table_class << 4 ) | table_no ;
	p = std::copy( codelens , codelens+ncodelens , p ) ;
	p = std::copy( symbols , symbols+nsymbols , p ) ;
	return p ;
}

template <typename T>
static T make_dri_header( T p , unsigned int dri )
{
	if( dri != 0U )
	{
		*p++ = 0xff ;
		*p++ = 0xdd ; // DRI
		*p++ = 0x0 ; // length msb
		*p++ = 4 ; // length lsb
		*p++ = dri >> 8 ; // dri msb
		*p++ = dri & 0xff ; // dri lsb
	}
	return p ;
}

template <typename T>
static T make_app0( T p )
{
	*p++ = 0xff ;
	*p++ = 0xe0 ; // APP0
	*p++ = 0 ; // length msb
	*p++ = 16 ; // length lsb
	*p++ = 0x4a ; // 'J'
	*p++ = 0x46 ; // 'F'
	*p++ = 0x49 ; // 'I'
	*p++ = 0x46 ; // 'F'
	*p++ = 0 ;
	*p++ = 0x01 ; // version major
	*p++ = 0x02 ; // version minor
	*p++ = 0 ; // pixel density units none
	*p++ = 0 ; // pixel density x msb
	*p++ = 1 ; // pixel density x lsb
	*p++ = 0 ; // pixel density y msb
	*p++ = 1 ; // pixel density y lsb
	*p++ = 0 ; // thumbnail dx
	*p++ = 0 ; // thumbnail dy
	return p ;
}

unsigned char comp( unsigned char n )
{
	// component ids can be 0,1,2 or 1,2,3 or 
	// whatever -- use 1,2,3 for easy diffing
	return n + 1U ;
}

template <typename T>
static T make_headers( T p , unsigned int type , unsigned int w_pixels , unsigned int h_pixels , const unsigned char * lqt ,
	const unsigned char * cqt , unsigned int dri )
{
	*p++ = 0xff ;
	*p++ = 0xd8 ; // SOI

	p = make_app0( p ) ;

	p = make_quant_header( p , lqt , 0 ) ; // or use in-band tables if q_is_special()
	p = make_quant_header( p , cqt , 1 ) ; // or use in-band tables if q_is_special()
	p = make_dri_header( p , dri ) ;

	*p++ = 0xff ;
	*p++ = 0xc0 ; // SOF
	*p++ = 0 ; // length msb
	*p++ = 17 ; // length lsb
	*p++ = 8 ; // 8-bit precision
	*p++ = h_pixels >> 8 ; // height msb
	*p++ = h_pixels ; // height lsb
	*p++ = w_pixels >> 8 ; // width msb
	*p++ = w_pixels ; // width lsb
	*p++ = 3 ; // number of components
	*p++ = comp(0) ; // comp 0
	*p++ = type == 0U ? 0x21 : 0x22 ; // hsamp=2,vsamp=1 : hsamp=2,vsamp=2
	*p++ = 0 ; // quant table 0
	*p++ = comp(1) ; // comp 1
	*p++ = 0x11 ; // hsamp=1,vsamp=1
	*p++ = 1 ; // quant table 1
	*p++ = comp(2) ; // comp 2
	*p++ = 0x11 ; // hsamp=1,vsamp=1
	*p++ = 1 ; // quant table 1

	p = make_huffman_header( p , lum_dc_codelens, sizeof(lum_dc_codelens), lum_dc_symbols, sizeof(lum_dc_symbols), 0, 0 ) ;
	p = make_huffman_header( p , lum_ac_codelens, sizeof(lum_ac_codelens), lum_ac_symbols, sizeof(lum_ac_symbols), 0, 1 ) ;
	p = make_huffman_header( p , chm_dc_codelens, sizeof(chm_dc_codelens), chm_dc_symbols, sizeof(chm_dc_symbols), 1, 0 ) ;
	p = make_huffman_header( p , chm_ac_codelens, sizeof(chm_ac_codelens), chm_ac_symbols, sizeof(chm_ac_symbols), 1, 1 ) ;

	*p++ = 0xff ;
	*p++ = 0xda ; // SOS
	*p++ = 0 ; // length msb
	*p++ = 12 ; // length lsb
	*p++ = 3 ; // 3 components
	*p++ = comp(0) ; // comp 0
	*p++ = 0 ; // huffman table 0
	*p++ = comp(1) ; // comp 1
	*p++ = 0x11 ; // huffman table 1
	*p++ = comp(2) ; // comp 2
	*p++ = 0x11 ; // huffman table 1
	*p++ = 0 ; // first DCT coeff
	*p++ = 63 ; // last DCT coeff
	*p++ = 0 ; // sucessive approximation

	return p ;
}

// ==

Gv::RtpJpegPacket::iterator_t Gv::RtpJpegPacket::generateHeader( iterator_t out , const RtpJpegPacket & packet , int fudge )
{
	return make_headers( out , packet.type_base() , packet.dx() , packet.dy() , 
		luma_table(packet.q(),fudge) , chroma_table(packet.q(),fudge) , packet.ri() ) ;
}

/// \file gvrtpjpegpacket.cpp

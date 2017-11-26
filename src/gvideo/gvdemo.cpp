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
// gvdemo.cpp
//

#include "gdef.h"
#include "gvdemo.h"
#include "gtime.h"
#include "gassert.h"
#include <stdexcept>

namespace data
{
	#include "gvdemodata.h"
}

/// \class Gv::DemoImp
/// A pimple-pattern implementation class for Gv::Demo.
///
class Gv::DemoImp
{
public:
	typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
	DemoImp( int , int , Gv::Timezone ) ;
	void drawScene( CaptureBuffer & b , const CaptureBufferScale & scale ) ;
	void drawItem( CaptureBuffer & b , const CaptureBufferScale & scale ,
		const char * image , size_t image_dx , size_t image_dy , int offset_x , int offset_y ,
		triple_t fg , bool draw_bg = false , size_t lhs_blank = 0U ) ;
	int hex_value( char c ) ;
	void jitter() ;
	void still() ;

private:
	struct Out
	{
		typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
		Out( Gv::CaptureBuffer & b , const Gv::CaptureBufferScale & scale , 
			size_t image_dx , size_t image_dy , size_t offset_x , size_t offset_y , 
			triple_t fg , triple_t bg , bool draw_bg , size_t lhs_blank ) ;
		void operator()( bool b , size_t ) ;
		static void clear( Gv::CaptureBuffer & , triple_t ) ;
		unsigned char * m_row ;
		unsigned char * m_p ;
		unsigned char * m_lhs ;
		unsigned char * m_rhs ;
		size_t m_linesize ;
		size_t m_image_dx ;
		size_t m_offset_x_3 ;
		size_t m_image_x ;
		bool m_draw_bg ;
		triple_t m_fg ;
		triple_t m_bg ;
	} ;

private:
	Gv::Timezone m_tz ;
	std::time_t m_blink ;
	size_t m_cars_pos ;
	int m_j[12] ;
	triple_t m_bg ;
} ;

// ==

Gv::Demo::Demo( int dx , int dy , const std::string & /*dev_config*/ , Gv::Timezone tz ) :
	m_imp(new DemoImp(dx,dy,tz))
{
}

Gv::Demo::~Demo()
{
	delete m_imp ;
}

bool Gv::Demo::init()
{
	return true ; // rgb
}

void Gv::Demo::fillBuffer( CaptureBuffer & b , const CaptureBufferScale & scale )
{
	if( scale.m_dx != 640 || scale.m_dy != 480 || scale.m_buffersize != (scale.m_dx*scale.m_dy*3U) )
		throw std::runtime_error( "unexpected frame buffer size for demo image" ) ;
	m_imp->drawScene( b , scale ) ;
}

// ==

Gv::DemoImp::DemoImp( int , int , Gv::Timezone tz ) :
	m_tz(tz) ,
	m_blink(0) ,
	m_cars_pos(0U) ,
	m_bg(triple_t(255U,240U,240U))
{
}

void Gv::DemoImp::jitter()
{
	for( int i = 0 ; i < 12 ; i++ )
		m_j[i] = std::rand() % 5 ;
}

void Gv::DemoImp::still()
{
	for( int i = 0 ; i < 12 ; i++ )
		m_j[i] = 0 ;
}

void Gv::DemoImp::drawScene( CaptureBuffer & b , const CaptureBufferScale & scale )
{
	Out::clear( b , m_bg ) ;
	G::EpochTime now = G::DateTime::now() + m_tz.seconds() ;
	G::Time time( now ) ;
	triple_t c[] = { triple_t(0,0,70) , triple_t(50,0,80) , triple_t(70,0,100) } ;

	// traffic (first)
	{
		drawItem( b , scale , data::cars_data , data::cars_dx , data::cars_dy , 
			270+m_cars_pos , 141 , c[2] , false , m_cars_pos < 212U ? (212U-m_cars_pos) : 0U ) ;
		m_cars_pos++ ;
		if( m_cars_pos == 420U )
			m_cars_pos = 0U ;
	}

	// base image (inc. normal face, clock surround)
	{
		drawItem( b , scale , data::base_data , data::base_dx , data::base_dy , 65 , 10 , c[0] ) ;
	}

	// fingers
	{
		int finger_x = 176 ;
		int finger_y = 376 ;
		still() ;
		if( (now.s % 9) == 0 )
			jitter() ;
		drawItem( b , scale , data::finger_1_data , data::finger_1_dx , data::finger_1_dy , finger_x+m_j[0] , finger_y+m_j[1] , c[0] ) ;
		drawItem( b , scale , data::finger_2_data , data::finger_2_dx , data::finger_2_dy , finger_x+m_j[2] , finger_y+m_j[3] , c[0] ) ;
		drawItem( b , scale , data::finger_3_data , data::finger_3_dx , data::finger_3_dy , finger_x+m_j[4] , finger_y+m_j[5] , c[0] ) ;
		drawItem( b , scale , data::finger_4_data , data::finger_4_dx , data::finger_4_dy , finger_x+m_j[6] , finger_y+m_j[7] , c[0] ) ;
		drawItem( b , scale , data::finger_5_data , data::finger_5_dx , data::finger_5_dy , finger_x+m_j[8] , finger_y+m_j[9] , c[0] ) ;
		drawItem( b , scale , data::finger_6_data , data::finger_6_dx , data::finger_6_dy , finger_x+m_j[10] , finger_y+m_j[11] , c[0] ) ;
	}

	// cuckoo
	{
		if( time.minutes() == 0 && time.seconds() < 3 )
			drawItem( b , scale , data::bird_data , data::bird_dx , data::bird_dy , 7 , 120 , c[1] ) ;
	}

	// blink
	{
		if( m_blink == 0 || now.s == m_blink )
		{
			if( now.us < 150000 )
				drawItem( b , scale , data::blink_data , data::blink_dx , data::blink_dy , 256 , 112 , c[0] , true ) ;
			else
				m_blink = now.s + 5 ;
		}
	}

	// grimace
	{
		if( (now.s % 19) < 2 )
			drawItem( b , scale , data::grimace_data , data::grimace_dx , data::grimace_dy , 245 , 160 , c[0] , true ) ;
	}

	// clock hands
	{
		int minute_index = time.minutes() ;
		int hour_index = ((time.hours() % 12)*5) + time.minutes()/12 ;
		G_ASSERT( sizeof(data::minute_hand_data)/sizeof(data::minute_hand_data[0]) == 60 ) ;
		G_ASSERT( sizeof(data::hour_hand_data)/sizeof(data::hour_hand_data[0]) == 60 ) ;
		minute_index %= 60 ; hour_index %= 60 ; // just in case
		drawItem( b , scale , data::minute_hand_data[minute_index] , data::minute_hand_dx , data::minute_hand_dy , 97 , 42 , c[0] ) ;
		drawItem( b , scale , data::hour_hand_data[hour_index] , data::hour_hand_dx , data::hour_hand_dy , 97 , 42 , c[0] ) ;
	}
}

void Gv::DemoImp::drawItem( CaptureBuffer & b , const CaptureBufferScale & scale ,
	const char * image_rle , size_t image_dx , size_t image_dy , int offset_x , int offset_y ,
	triple_t fg , bool draw_bg , size_t lhs_blank )
{
	Out out( b , scale , image_dx , image_dy , offset_x , offset_y , fg , m_bg , draw_bg , lhs_blank ) ;

	for( const char * p = image_rle ; p[0] && p[1] ; p += 2 )
	{
		int n = (hex_value(p[0])<<4) + hex_value(p[1]) ;
		bool v = !( n & 0x80 ) ;
		size_t length = n & 0x7f ;
		out( !v , length ) ;
	}
}

int Gv::DemoImp::hex_value( char c )
{
	if( c >= '0' && c <= '9' ) 
		return c - '0' ;
	else if( c >= 'A' && c <= 'F' )
		return (c - 'A') + 10 ;
	else
		return 0 ;
}

// ==

Gv::DemoImp::Out::Out( Gv::CaptureBuffer & b , const Gv::CaptureBufferScale & scale , 
	size_t image_dx , size_t image_dy , size_t offset_x , size_t offset_y , 
	triple_t fg , triple_t bg , bool draw_bg , size_t lhs_blank ) :
		m_linesize(scale.m_linesize) ,
		m_image_dx(image_dx) ,
		m_offset_x_3(offset_x*3U) ,
		m_image_x(0U) ,
		m_draw_bg(draw_bg) ,
		m_fg(fg) ,
		m_bg(bg)
{
	G_ASSERT( offset_y < scale.m_dy ) ;
	m_row = b.begin() + ( offset_y * m_linesize ) ;
	m_p = m_row + m_offset_x_3 ;
	m_lhs = m_p + ( lhs_blank * 3U ) ;
	m_rhs = m_row + m_linesize ;
}

void Gv::DemoImp::Out::clear( Gv::CaptureBuffer & b , triple_t bg )
{
	const unsigned char * const end = b.end() ;
	for( unsigned char * p = b.begin() ; p < end ; p += 3 )
	{
		p[0] = bg.r() ;
		p[1] = bg.g() ;
		p[2] = bg.b() ;
	}
}

void Gv::DemoImp::Out::operator()( bool b , size_t n )
{
	if( m_draw_bg )
	{
		for( size_t i = 0U ; i < n && m_p < m_rhs ; i++ , m_p += 3 )
		{
			if( m_p > m_lhs )
			{
				m_p[0] = b ? m_fg.r() : m_bg.r() ;
				m_p[1] = b ? m_fg.g() : m_bg.g() ;
				m_p[2] = b ? m_fg.b() : m_bg.b() ;
			}
		}
	}
	else if( b )
	{
		for( size_t i = 0U ; i < n && m_p < m_rhs ; i++ , m_p += 3 )
		{
			if( m_p > m_lhs )
			{
				m_p[0] = m_fg.r() ;
				m_p[1] = m_fg.g() ;
				m_p[2] = m_fg.b() ;
			}
		}
	}
	else
	{
		m_p += n ; m_p += n ; m_p += n ;
		if( m_p > m_rhs )
			m_p = m_rhs ;
	}

	m_image_x += n ; // (requires rle runs to break at rhs)
	if( m_image_x >= m_image_dx )
	{
		m_image_x = 0U ;
		m_lhs += m_linesize ;
		m_rhs += m_linesize ;
		m_row += m_linesize ;
		m_p = m_row + m_offset_x_3 ;
	}
}


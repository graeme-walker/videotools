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
// gvbars.cpp
//

#include "gdef.h"
#include "gvbars.h"
#include "grcolourspace.h"
#include "gstr.h"
#include <algorithm>

namespace
{
	typedef Gr::ColourSpace::triple<unsigned char> triple_t ;
	enum Colour { c_black , c_white , c_yellow , c_cyan , c_green , c_magenta , 
		c_red , c_blue , c_orange , c_purple , c__max } ;
}

namespace Gv
{
	class BarsGenerator ;
}

/// \class Gv::BarsGenerator
/// A implementation class for Gv::BarsImp.
///
class Gv::BarsGenerator
{
public:
	static BarsGenerator make( int dx , int dy , int type ) ;
		// Factory function.

	void move() ;
		// Animates the image.

	Colour colour( int x , int y ) const ;
		// Returns the pixel colour.

private:
	int colours() const ;
	template <typename T> BarsGenerator( bool in_rows , int dx , int dy , int speed , T c_begin , T c_end ) ;

private:
	bool m_in_rows ;
	int m_dx ;
	int m_dy ;
	int m_speed ;
	int m_offset ;
	std::vector<Colour> m_colours ;
} ;

/// \class Gv::BarsImp
/// A pimple-pattern implementation class for Gv::Bars.
///
class Gv::BarsImp
{
public:
	BarsImp( int dx , int dy , const std::string & type ) ;
		// Constructor.

	void fillBuffer( CaptureBuffer & b , const CaptureBufferScale & ) ;
		// Fills the buffer with the current frame.

private:
	int m_dx ;
	int m_dy ;
	std::vector<triple_t> m_colour_yuv ;
	BarsGenerator m_generator ;
} ;

// ==

namespace
{
	int type( const std::string & config )
	{
		return G::Str::splitMatch(config,"horizontal",";") ? 1 : 0 ;
	}
}

Gv::BarsImp::BarsImp( int dx , int dy , const std::string & config ) :
	m_dx(dx) ,
	m_dy(dy) ,
	m_generator(BarsGenerator::make(m_dx,m_dy,type(config)))
{
	m_colour_yuv.resize( c__max , triple_t(0,0,0) ) ;
	m_colour_yuv[c_black] = Gr::ColourSpace::yuv_int( triple_t(0,0,0) ) ;
	m_colour_yuv[c_white] = Gr::ColourSpace::yuv_int( triple_t(255,255,255) ) ;
	m_colour_yuv[c_yellow] = Gr::ColourSpace::yuv_int( triple_t(255,255,0) ) ;
	m_colour_yuv[c_cyan] = Gr::ColourSpace::yuv_int( triple_t(0,255,255) ) ;
	m_colour_yuv[c_green] = Gr::ColourSpace::yuv_int( triple_t(0,255,0) ) ;
	m_colour_yuv[c_magenta] = Gr::ColourSpace::yuv_int( triple_t(255,0,255) ) ;
	m_colour_yuv[c_red] = Gr::ColourSpace::yuv_int( triple_t(255,0,0) ) ;
	m_colour_yuv[c_blue] = Gr::ColourSpace::yuv_int( triple_t(0,0,255) ) ;
	m_colour_yuv[c_orange] = Gr::ColourSpace::yuv_int( triple_t(255,128,0) ) ;
	m_colour_yuv[c_purple] = Gr::ColourSpace::yuv_int( triple_t(150,10,150) ) ;
}

void Gv::BarsImp::fillBuffer( Gv::CaptureBuffer & b , const CaptureBufferScale & scale )
{
	G_ASSERT( int(scale.m_dx) == m_dx && int(scale.m_dy) == m_dy ) ;
	m_generator.move() ;

	unsigned char * p_row = b.begin() ;
	for( int y = 0 ; y < m_dy ; ++y , p_row += (m_dx*2) )
	{
		unsigned char * p = p_row ;
		for( int x = 0 ; x < m_dx ; x++ , p += 2 )
		{
			triple_t yuv = m_colour_yuv.at(m_generator.colour(x,y)) ;
			if( (p+1) >= b.end() ) return ; // just in case
			p[0] = yuv.y() ;
			p[1] = ( (long)p & 2 ) ? yuv.v() : yuv.u() ;
		}
	}
}

// ==

Gv::Bars::Bars( int dx , int dy , const std::string & config ) :
	m_imp(new BarsImp(dx,dy,config))
{
}

Gv::Bars::~Bars()
{
	delete m_imp ;
}

bool Gv::Bars::init()
{
	return false ; // yuyv
}

void Gv::Bars::fillBuffer( Gv::CaptureBuffer & b , const Gv::CaptureBufferScale & scale )
{
	m_imp->fillBuffer( b , scale ) ;
}

// ==

template <typename T>
Gv::BarsGenerator::BarsGenerator( bool in_rows , int dx , int dy , int speed , T c_begin , T c_end ) :
	m_in_rows(in_rows) ,
	m_dx(dx) ,
	m_dy(dy) ,
	m_speed(speed) ,
	m_offset(0) ,
	m_colours(c_begin,c_end)
{
}

void Gv::BarsGenerator::move()
{
	m_offset += m_speed ;
	if( ( m_in_rows && m_offset == m_dy ) || m_offset == m_dx )
		m_offset = 0 ;
}

Colour Gv::BarsGenerator::colour( int x , int y ) const
{
	return 
		m_in_rows ? 
			m_colours.at( (colours()*(y+m_offset)/m_dy) % colours() ) :
			m_colours.at( (colours()*(x+m_offset)/m_dx) % colours() ) ;
}

int Gv::BarsGenerator::colours() const 
{ 
	return static_cast<int>(m_colours.size()) ; 
}

Gv::BarsGenerator Gv::BarsGenerator::make( int dx , int dy , int type ) 
{
	if( type == 0 )
	{
		Colour colours[] = { c_black , c_white , c_yellow , c_cyan , c_green , c_magenta , c_red , c_blue } ;
		return BarsGenerator( false , dx , dy , 1 , colours , colours+8 ) ;
	}
	else
	{
		Colour colours[] = { c_red , c_orange , c_yellow , c_green , c_blue , c_purple } ;
		return BarsGenerator( true , dx , dy , 1 , colours , colours+6 ) ;
	}
}


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
// gxcanvas.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxcanvas.h"
#include "gxdisplay.h"
#include "gxwindow.h"
#include "gxcolourmap.h"
#include "glog.h"
#include "gtest.h"
#include "gassert.h"
#include "gstr.h"
#include "grcolour16.h"
#include "grglyph.h"
#include <stdexcept>
#include <utility>

GX::Canvas::Canvas( GX::Window & window , int dx , int dy , int colours , bool server_side ) :
	m_window(window) ,
	m_dx(dx>0?dx:window.dx()) ,
	m_dy(dy>0?dy:window.dy()) ,
	m_colours(colours) ,
	m_client_side(!server_side) ,
	m_aspect_top(7) ,
	m_aspect_bottom(10) ,
	m_pixmap(m_window.windowDisplay(),m_window,m_dx,m_dy) ,
	m_image(m_window.windowDisplay(),m_window,m_dx,m_dy) ,
	m_context(m_window.windowDisplay(),m_window) ,
	m_stereo_mode(normal)
{
	if( G::Test::enabled("gxcanvas-server-side") ) m_client_side = false ;
	if( G::Test::enabled("gxcanvas-client-side") ) m_client_side = true ;
	if( G::Test::enabled("gxcanvas-palette") ) colours = m_colours = 16 ;
	G_ASSERT( colours == 0 || colours == 16 || colours == 256 ) ; // rgb,bios,greyscale
	if( m_colours != 0 )
	{
		m_colour_map.reset( new GX::ColourMap( m_window.windowDisplay() , m_colours == 256 ) ) ;
		m_window.install( *m_colour_map.get() ) ;
	}
}

GX::Canvas::~Canvas() 
{ 
}

int GX::Canvas::dx() const
{
	return m_dx ;
}

int GX::Canvas::dy() const
{
	return m_dy ;
}

int GX::Canvas::colours() const
{
	return m_colours ;
}

std::pair<unsigned int,unsigned int> GX::Canvas::aspect() const
{
	return std::make_pair( m_aspect_top , m_aspect_bottom ) ;
}

Gr::Colour GX::Canvas::colour( unsigned int n ) const 
{
	G_ASSERT( m_colours != 0 && static_cast<int>(n) < m_colours ) ;
	return Gr::Colour( n , 0U , 0U ) ;
}

Gr::Colour GX::Canvas::black() const
{
	return m_colours == 0 ? Gr::Colour() : colour(0U) ;
}

Gr::Colour GX::Canvas::white() const
{
	return m_colours == 0 ? Gr::Colour(255,255,255) : (m_colours==16?colour(1U):colour(255U)) ;
}

namespace
{
	struct out
	{
		out( GX::Canvas & canvas , int x , int y , Gr::Colour c , Gr::Colour b ) : m_canvas(canvas) , m_x(x) , m_y(y) , m_c(c) , m_b(b) {}
		void operator()( int x , int y , bool b )
		{
			m_canvas.point( x + m_x , m_canvas.dy() - ( y + m_y ) , b ? m_c : m_b ) ;
		}
		GX::Canvas & m_canvas ;
		int m_x ;
		int m_y ;
		Gr::Colour m_c ;
		Gr::Colour m_b ;
	} ;
}

void GX::Canvas::text( const std::string & s , int x , int y , Gr::Colour c )
{
	out o( *this , x , y , c , black() ) ;
	Gr::Glyph::output( s , o ) ;
}

unsigned long GX::Canvas::pixel( const Gr::Colour & c_in ) const
{
	return pixel( c_in , m_stereo_mode ) ;
}

unsigned long GX::Canvas::pixel( const Gr::Colour & c_in , Canvas::Stereo stereo_mode ) const
{
	Gr::Colour c = c_in ;
	if( stereo_mode == Canvas::left )
	{
		static Gr::Colour red_0( 200 , 0 , 0 ) ;
		static Gr::Colour red_16( 12 , 0 , 0 ) ;
		static Gr::Colour red_256( 210 , 0 , 0 ) ;
		c = m_colours == 0 ? red_0 : ( m_colours == 16 ? red_16 : red_256 ) ;
	}
	else if( stereo_mode == Canvas::right )
	{
		static Gr::Colour green_0( 0 , 200 , 0 ) ;
		static Gr::Colour green_16( 10 , 0 , 0 ) ;
		static Gr::Colour green_256( 120 , 0 , 0 ) ;
		c = m_colours == 0 ? green_0 : ( m_colours == 16 ? green_16 : green_256 ) ;
	}

	G_ASSERT( (m_colour_map.get()==nullptr) == (m_colours==0) ) ;
	if( m_colours != 0 && ( c.g() || c.b() ) )
		G_WARNING_ONCE( "GX::Canvas::pixel: invalid colour: green and blue components ignored" ) ;

	return m_colour_map.get() ? m_colour_map->get(c.r()) : c.cc() ;
}

bool GX::Canvas::fastable() const
{
	return 
		m_stereo_mode != Canvas::left && m_stereo_mode != Canvas::right && 
		m_colour_map.get() == nullptr &&
		m_client_side && m_image.fastable() ;
}

void GX::Canvas::point( int x , int y , Gr::Colour c )
{
	if( m_client_side )
	{
		bool pixel_or = m_stereo_mode == Canvas::left || m_stereo_mode == Canvas::right ;
		m_image.drawPoint( x , m_dy-1-y , pixel(c) , pixel_or ) ;
	}
	else
	{
		m_context.setForeground( pixel(c) ) ;
		m_pixmap.drawPoint( m_context , x , m_dy-1-y ) ;
	}
}

void GX::Canvas::line( int x0 , int y0 , int x1 , int y1 , Gr::Colour c )
{
	if( m_client_side )
	{
		bool pixel_or = m_stereo_mode == Canvas::left || m_stereo_mode == Canvas::right ;
		m_image.drawLine( x0 , m_dy-1-y0 , x1 , m_dy-1-y1 , pixel(c) , pixel_or ) ;
	}
	else
	{
		m_context.setForeground( pixel(c) ) ;
		m_pixmap.drawLine( m_context , x0 , m_dy-1-y0 , x1 , m_dy-1-y1 ) ; // GX::Drawable::drawLine()
	}
}

void GX::Canvas::lineAcross( int y , int x_first , int x_last , const Gr::Colour & c )
{
	if( m_client_side )
		m_image.drawLineAcross( x_first , x_last , m_dy-1-y , pixel(c) ) ;
	else
		line( x_first , y , x_last , y , c ) ;
}

void GX::Canvas::lineDown( int x , int y_first , int y_last , const Gr::Colour & c )
{
	if( m_client_side )
		m_image.drawLineDown( x , m_dy-1-y_first , m_dy-1-y_last , pixel(c) ) ;
	else
		line( x , y_first , x , y_last , c ) ;
}

Gr::Colour GX::Canvas::readPoint( int x , int y ) const
{
	unsigned long pixel = 0UL ;
	if( m_client_side )
	{
		pixel = m_image.readPoint( x , m_dy-1-y ) ;
	}
	else
	{
		G_WARNING( "Canvas::readPoint: slow implementation" ) ;
		unique_ptr<GX::Image> image_ptr ;
		m_pixmap.readImage( image_ptr ) ;
		pixel = image_ptr.get()->readPoint( x , m_dy-1-y ) ;
	}

	G_ASSERT( (m_colour_map.get()==nullptr) == (m_colours==0) ) ;
	if( m_colour_map.get() != nullptr )
	{
		G_ASSERT( m_colour_map->size() == static_cast<unsigned int>(m_colours) ) ;
		pixel %= m_colour_map->size() ;
		return Gr::Colour( m_colour_map->find(pixel) , 0U , 0U ) ;
	}
	else
	{
		Gr::Colour::cc_t cc = pixel ;
		return Gr::Colour::from( cc ) ;
	}
}

void GX::Canvas::clear( bool to_white )
{
	if( m_client_side )
	{
		m_image.clear( to_white ) ;
	}
	else
	{
		unsigned long pixel = to_white ? white().cc() : black().cc() ;
		m_context.setForeground( pixel ) ;
		m_pixmap.drawRectangle( m_context , 0 , 0 , m_dx , m_dy ) ;
	}
}

void GX::Canvas::blit()
{
	if( m_client_side )
		m_image.blit( m_window , 0 , 0 , m_dx , m_dy , 0 , 0 ) ; // XPutImage()
	else
		m_pixmap.blit( m_window , 0 , 0 , m_dx , m_dy , 0 , 0 ) ; // XCopyArea()
}

void GX::Canvas::stereo( Canvas::Stereo mode )
{
	m_stereo_mode = mode ;
}

/// \file gxcanvas.cpp

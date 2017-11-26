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
// gvmask.cpp
// 

#include "gdef.h"
#include "gvmask.h"
#include "groot.h"
#include "gfile.h"
#include "grpnm.h"
#include "grscaler.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm> // std::min/max
#include <utility>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdexcept>

Gv::Mask::Mask( int dx , int dy , const std::string & path , bool create ) :
	m_dx(dx) ,
	m_dy(dy) ,
	m_path(path) ,
	m_empty(true) ,
	m_map(dx*dy) ,
	m_map_current(dx*dy) ,
	m_down(false) ,
	m_down_x(0) ,
	m_down_y(0) ,
	m_down_shift(false) ,
	m_file_time(0)
{
	G_ASSERT( m_dx > 0 && m_dy > 0 ) ;
	if( m_path != G::Path() )
	{
		// create the file if it doesnt exist
		findOrCreate( create ) ;

		// open the file
		std::ifstream file ;
		open( file ) ;

		// read the pnm data
		Gr::ImageData image_data ;
		Gr::PnmInfo info = read( file , image_data ) ;

		// subsample or supersample to convert from file_size to m_dx/dy
		sample( image_data , info ) ;
	}
}

void Gv::Mask::sample( const Gr::ImageData & image_data , const Gr::PnmInfo & info )
{
	for( Gr::Scaler y_scaler(m_dy,info.dy()) ; !!y_scaler ; ++y_scaler )
	{
		for( Gr::Scaler x_scaler(m_dx,info.dx()) ; !!x_scaler ; ++x_scaler )
		{
			// black pixels in the mask are masked out, but note that black
			// pixels are "1" in a pnm file, albeit parsed to zero rgb values
			bool masked = 0 == image_data.r( x_scaler.second() , y_scaler.second() ) ;
			if( masked ) m_empty = false ;
			m_map[ y_scaler.first() * m_dx + x_scaler.first() ] = masked ;
		}
	}
}

void Gv::Mask::findOrCreate( bool create )
{
	bool exists = false ;
	{
		G::Root claim_root ;
		exists = G::File::exists( m_path ) ;
	}
	G_DEBUG( "Gv::Mask::ctor: exists=" << exists ) ;
	if( !exists && !create )
		throw std::runtime_error( "cannot open mask file [" + m_path.str() + "]" ) ;
	else if( !exists )
		write( m_path ) ;
}

void Gv::Mask::open( std::ifstream & file )
{
	G::Root claim_root ;
	m_file_time = G::File::time( m_path , G::File::NoThrow() ) ;
	file.open( m_path.str().c_str() ) ;
}

Gr::PnmInfo Gv::Mask::read( std::ifstream & file , Gr::ImageData & image_data )
{
	Gr::PnmInfo info ;
	try
	{
		Gr::PnmReader reader ;
		reader.decode( image_data , file ) ;
		info = reader.info() ;
		G_DEBUG( "Gv::Mask::ctor: P" << info.pn() ) ;
	}
	catch( std::exception & )
	{
		G_ASSERT( !info.valid() ) ;
	}
	if( !info.valid() || ( info.pn() != 1 && info.pn() != 4 ) )
		throw std::runtime_error( "invalid mask file format: [" + m_path.str() + "] is not a pgm bitmap" ) ;
	return info ;
}

bool Gv::Mask::empty() const
{
	return m_empty ;
}

G::EpochTime Gv::Mask::time() const
{
	return m_file_time ;
}

void Gv::Mask::write( const G::Path & path ) const
{
	G_DEBUG( "Gv::Mask::write: writing [" << path << "]" ) ;
	std::ofstream f ;
	{
		G::Root claim_root ;
		f.open( path.str().c_str() , std::ios_base::trunc ) ;
	}
	if( !f.good() ) 
		throw std::runtime_error( "cannot write mask file [" + path.str() + "]" ) ;

	f << "P1\n" ;
	f << m_dx << " " << m_dy << "\n" ;
	for( int y = 0 ; y < m_dy ; y++ )
	{
		const char * sep = "" ;
		for( int x = 0 ; x < m_dx ; x++ , sep = " " )
		{
			f << sep << (m_map[y*m_dx+x]?"1":"0") ; // masked-out = black = "1"
		}
		f << "\n" ;
	}
	f << "\n" << std::flush ;
	if( !f.good() )
		throw std::runtime_error( "cannot write mask file [" + path.str() + "]" ) ;
}

int Gv::Mask::clip_x( int x ) const
{
	return std::min( std::max(0,x) , m_dx-1 ) ;
}

int Gv::Mask::clip_y( int y ) const
{
	return std::min( std::max(0,y) , m_dy-1 ) ;
}

void Gv::Mask::down( int x , int y , bool shift , bool control )
{
	m_down = true ;
	m_down_x = m_move_x = clip_x( x ) ;
	m_down_y = m_move_y = clip_y( y ) ;
	m_down_shift = shift ;
}

void Gv::Mask::move( int x , int y )
{
	if( m_down )
	{
		fill( m_map_current , m_down_x , m_down_y , m_move_x , m_move_y , false ) ;
		m_move_x = clip_x(x) ;
		m_move_y = clip_y(y) ;
		fill( m_map_current , m_down_x , m_down_y , m_move_x , m_move_y , true ) ;
	}
}

bool Gv::Mask::fill( std::vector<bool> & map , int x0 , int y0 , int x1 , int y1 , bool b )
{
	bool changed = false ;
	for( int x = std::min(x0,x1) ; x < std::max(x0,x1) ; x++ )
	{
		for( int y = std::min(y0,y1) ; y < std::max(y0,y1) ; y++ )
		{
			size_t offset = y*m_dx+x ;
			if( !changed && (map[offset]!= b) ) changed = true ;
			map[offset] = b ;
		}
	}
	return changed ;
}

void Gv::Mask::up( int up_x , int up_y , bool shift , bool control )
{
	m_down = false ;
	m_map_current.assign( m_map_current.size() , false ) ;
	if( fill( m_map , m_down_x , m_down_y , clip_x(up_x) , clip_y(up_y) , !m_down_shift ) && !m_down_shift )
		m_empty = false ;
}

bool Gv::Mask::update()
{
	if( m_path == G::Path() ) 
	{
		return false ;
	}
	else
	{
		G::EpochTime new_time = G::File::time( m_path , G::File::NoThrow() ) ;
		if( m_file_time == G::EpochTime(0) || new_time != m_file_time )
		{
			Gr::ImageData image_data ;
			Gr::PnmInfo info ;
			try
			{
				std::ifstream file ;
				{
					G::Root claim_root ;
					file.open( m_path.str().c_str() ) ;
				}
				info = read( file , image_data ) ;
			}
			catch( std::exception & e )
			{
				return false ;
			}
			sample( image_data , info ) ;
			m_file_time = new_time ;
			return true ;
		}
		else
		{
			return false ;
		}
	}
}

/// \file gvmask.cpp

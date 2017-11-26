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
///
/// \file gvmask.h
///

#ifndef GV_MASK__H
#define GV_MASK__H

#include "gdef.h"
#include "gtime.h"
#include "grimagedata.h"
#include "grpnm.h"
#include <string>
#include <vector>
#include <istream>

namespace Gv
{
	class Mask ;
}

/// \class Gv::Mask
/// Implements a binary mask over an image that can be edited by mouse actions, 
/// and that can be stored on disk.
/// 
class Gv::Mask
{
public:
	Mask( int dx , int dy , const std::string & file = std::string() , bool create = false ) ;
		///< Constructor, optionally reading the mask from an existing
		///< file. If the file is a different size from the parameters 
		///< then its contents are resampled to fit.

	bool empty() const ;
		///< Returns true if empty.

	void write( const G::Path & filename ) const ;
		///< Writes to file.

	bool update() ;
		///< Updates the mask from the file. Returns true if updated.

	void scale( int factor ) ;
		///< Scales down.

	void up( int x , int y , bool shift , bool control ) ;
		///< Called on mouse-up. Commits any down()/move() edits.

	void move( int x , int y ) ;
		///< Called on mouse-move. Modifies the current edit.

	void down( int x , int y , bool shift , bool control ) ;
		///< Called on mouse-down. Starts an edit.

	bool masked( int x , int y ) const ;
		///< Returns true if the pixel is masked.

	bool masked( size_t ) const ;
		///< Optimised overload, ignoring the current edit.

	G::EpochTime time() const ;
		///< Returns the timestamp on the mask file at 
		///< construction, not affected by any calls
		///< to write().

private:
	Mask( const Mask & ) ;
	void operator=( const Mask & ) ;
	int clip_x( int ) const ;
	int clip_y( int ) const ;
	bool fill( std::vector<bool>& , int , int , int , int , bool ) ;
	void sample( const Gr::ImageData & image_data , const Gr::PnmInfo & info ) ;
	void findOrCreate( bool ) ;
	void open( std::ifstream & file ) ;
	Gr::PnmInfo read( std::ifstream & file , Gr::ImageData & image_data ) ;

private:
	int m_dx ;
	int m_dy ;
	G::Path m_path ;
	bool m_empty ;
	std::vector<bool> m_map ;
	std::vector<bool> m_map_current ;
	bool m_down ;
	int m_down_x ;
	int m_down_y ;
	bool m_down_shift ;
	int m_move_x ;
	int m_move_y ;
	G::EpochTime m_file_time ;
} ;

inline
bool Gv::Mask::masked( size_t offset ) const
{
	return m_map[offset] ;
}

inline
bool Gv::Mask::masked( int x , int y ) const
{
	const size_t offset = y * m_dx + x ;
	if( m_down && m_down_shift )
		return m_map[offset] && !m_map_current[offset] ;
	else
		return m_map[offset] || m_map_current[offset] ;
}

#endif

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
/// \file gvoverlay.h
///

#ifndef GV_OVERLAY__H
#define GV_OVERLAY__H

#include "gdef.h"
#include "gvtimezone.h"
#include "gdatetime.h"
#include "grpng.h"
#include <vector>
#include <string>

namespace Gv
{
	class Overlay ;
}

namespace Gr
{
	class Glyph ;
}

/// \class Gv::Overlay
/// A class that can add a text overlay to an image.
/// 
class Gv::Overlay
{
public:
	explicit Overlay( Gr::ImageData & out ) ;
		///< Constructor.

	void write( int x , int y , const std::string & format , const Gv::Timezone & ) ;
		///< Writes the string at the given position, with text-substitution
		///< for "%t".

	void timestamp( int x , int y , const Gv::Timezone & ) ;
		///< Writes a timestamp at the given position.

private:
	Overlay( const Overlay & ) ;
	void operator=( const Overlay & ) ;
	static std::string zone( const Gv::Timezone & ) ;
	static std::string timestamp( G::EpochTime , const Gv::Timezone & ) ;
	static std::string datetime( G::EpochTime , const Gv::Timezone & ) ;
	static std::string date( G::EpochTime , const Gv::Timezone & ) ;
	static std::string time( G::EpochTime , const Gv::Timezone & ) ;
	friend class Gr::Glyph ;
	void operator()( int x , int y , bool b ) ;

private:
	Gr::ImageData & m_data ;
	int m_x0 ;
	int m_y0 ;
} ;

#endif

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
// gxwindowmap.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxwindowmap.h"
#include "gxerror.h"

GX::WindowMap * GX::WindowMap::m_this = nullptr ;

GX::WindowMap::WindowMap()
{
	if( m_this == nullptr )
		m_this = this ;
}

GX::WindowMap::~WindowMap()
{
	if( m_this == this )
		m_this = nullptr ;
}

GX::WindowMap * GX::WindowMap::instance()
{
	return m_this ;
}

void GX::WindowMap::add( GX::Window & w )
{
	m_map.insert( Map::value_type(w.x(),&w) ) ;
}

void GX::WindowMap::remove( GX::Window & w )
{
	m_map.erase( w.x() ) ;
}

GX::Window & GX::WindowMap::find( ::Window w ) const
{
	Map::const_iterator p = m_map.find( w ) ;
	if( p == m_map.end() )
		throw NotFound() ;
	return *((*p).second) ;
}

/// \file gxwindowmap.cpp

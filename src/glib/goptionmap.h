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
/// \file goptionmap.h
///

#ifndef G_OPTION_MAP_H
#define G_OPTION_MAP_H

#include "gdef.h"
#include <string>
#include <map>
#include <algorithm>

namespace G
{
	class OptionMap ;
}

/// \class G::OptionMap
/// A map-like container for command-line options and their values.
/// Normally populated by G::OptionParser.
/// 
class G::OptionMap
{
public:
	typedef std::multimap<std::string,OptionValue> Map ;
	typedef Map::value_type value_type ;
	typedef Map::iterator iterator ;
	typedef Map::const_iterator const_iterator ;

public:
	OptionMap() ;
		///< Default constructor for an empty map.

	void insert( const Map::value_type & ) ;
		///< Inserts the key/value pair into the map. The ordering of values in the
		///< map with the same key is normally the order of insertion (but this 
		///< depends on the underlying multimap implementation for c++98).

	const_iterator begin() const ;
		///< Returns the begin iterator.

	const_iterator end() const ;
		///< Returns the off-the-end iterator.

	const_iterator find( const std::string & ) const ;
		///< Finds the map entry with the given key.

	void clear() ;
		///< Clears the map.

	bool contains( const std::string & ) const ;
		///< Returns true if the map contains the given key, but ignoring un-valued() 
		///< 'off' options.

	size_t count( const std::string & key ) const ;
		///< Returns the number of times the key appears in the multimap.

	std::string value( const std::string & key ) const ;
		///< Returns the value of the valued() option identified by the given key.
		///< Multiple matching values are concatenated with a comma separator
		///< (normally in the order of insertion).

private:
	std::string join( Map::const_iterator , Map::const_iterator ) const ;

private:
	Map m_map ;
} ;

inline
G::OptionMap::OptionMap()
{
}

inline
void G::OptionMap::insert( const Map::value_type & value )
{
	m_map.insert( value ) ;
}

inline
G::OptionMap::const_iterator G::OptionMap::begin() const
{
	return m_map.begin() ;
}

inline
G::OptionMap::const_iterator G::OptionMap::end() const
{
	return m_map.end() ;
}

inline
G::OptionMap::const_iterator G::OptionMap::find( const std::string & key ) const
{
	return m_map.find( key ) ;
}

inline
void G::OptionMap::clear()
{
	m_map.clear() ;
}

inline
bool G::OptionMap::contains( const std::string & key ) const
{
	const Map::const_iterator end = m_map.end() ;
	for( Map::const_iterator p = m_map.find(key) ; p != end && (*p).first == key ; ++p )
	{
		if( (*p).second.valued() || !(*p).second.is_off() )
			return true ;
	}
	return false ;
}

inline
size_t G::OptionMap::count( const std::string & key ) const
{
	size_t n = 0U ;
	const Map::const_iterator end = m_map.end() ;
	for( Map::const_iterator p = m_map.find(key) ; p != end && (*p).first == key ; ++p )
		n++ ;
	return n ;
}

inline
std::string G::OptionMap::value( const std::string & key ) const
{
	Map::const_iterator p = m_map.find( key ) ;
	if( p == m_map.end() || !(*p).second.valued() )
		return std::string() ;
	else if( count(key) == 1U )
		return (*p).second.value() ;
	else
		return join( p , std::upper_bound(p,m_map.end(),*p,m_map.value_comp()) ) ;
}

inline
std::string G::OptionMap::join( Map::const_iterator p , Map::const_iterator end ) const
{
	std::string result ;
	for( const char * sep = "" ; p != end ; ++p )
	{
		if( (*p).second.valued() )
		{
			result.append( sep ) ; sep = "," ;
			result.append( (*p).second.value() ) ;
		}
	}
	return result ;
}

#endif

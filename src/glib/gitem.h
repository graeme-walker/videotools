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
/// \file gitem.h
///

#ifndef G_ITEM__H
#define G_ITEM__H

#include "gdef.h"
#include "gexception.h"
#include "glog.h"
#include <map>
#include <vector>
#include <string>

namespace G
{
	class Item ;
}

/// \class G::Item
/// A variant class holding a string, an item map keyed by name, or an ordered 
/// list of items. The definition is recursive, so items can be nested. The 
/// string representation is curiously json-like.
/// 
class G::Item
{
public:
	G_EXCEPTION( Error , "item error" ) ;
	G_EXCEPTION( ParseError , "item error: parsing failed" ) ;
	enum Type { t_string , t_list , t_map } m_type ;

	static Item list() ;
		///< Factory function for a list item.

	static Item map() ;
		///< Factory function for a map item.

	static Item string() ;
		///< Factory function for a string item.

	static Item parse( const std::string & ) ;
		///< Parses the string representation. Throws on error.

	static bool parse( const std::string & , Item & out ) ;
		///< A non-throwing overload to parse the string representation.
		///< Returns false on error.

	explicit Item( Type type = t_string ) ;
		///< Constructor.

	explicit Item( const std::string & s ) ;
		///< Constructor for a string item.

	Type type() const ;
		///< Returns the item type (string, list or map).

	const Item & operator[]( const std::string & k ) const ;
		///< Indexing operator for a map item. Throws if no such item.

	Item & operator[]( const std::string & k ) ;
		///< Indexing operator for a map item. Creates an empty string 
		///< item if no such item initially.

	Item & operator[]( size_t i ) ;
		///< Indexing operator for a list item. Throws if out of range.

	const Item & operator[]( size_t i ) const ;
		///< Indexing operator for a list item. Throws if out of range.

	void add( const std::string & s ) ;
		///< Adds a string item to this list item.

	void add( Item i ) ;
		///< Adds an item to this list item.

	void add( const std::string & k , const std::string & s ) ;
		///< Adds a string item to this map item. Replaces any pre-existing item with that key.

	void add( const std::string & k , const Item & i ) ;
		///< Adds an item to this map item. Replaces any pre-existing item with that key.

	bool has( const std::string & k ) const ;
		///< Returns true if this map item has the named key.

	void remove( const std::string & k ) ;
		///< Removes an item from this map item.

	Item keys() const ;
		///< Returns the keys of this map item as a list of string items.

	size_t size() const ;
		///< Returns the size of this map-or-list item.

	bool empty() const ;
		///< Returns true if an empty item.

	std::string operator()() const ;
		///< Returns the value of this string item.

	void out( std::ostream & s , int indent = -1 ) const ;
		///< Does output streaming, using a json-like format.

	void in( std::istream & s ) ;
		///< Does input parsing. Throws on error.

	std::string str( int indent = -1 ) const ;
		///< Returns a string representation, using a json-like format.

private:
	void check( Type type ) const ;
	void checkNot( Type type ) const ;
	void check() const ;
	void need( const std::string & k ) const ;
	void need( size_t i ) const ;
	void update( const std::string & k ) ;
	static bool less( const Item & a , const Item & b ) ;
	void clear( Type type = t_string ) ;
	void read( std::istream & , char eos ) ;
	void readn( std::istream & , char ) ;
	static bool escaped( const std::string & s ) ;
	static void unescape( std::string & s ) ;
	static void unescape( std::string & s , char bs , const char * map_in , const char * map_out ) ;

private:
	typedef std::map<std::string,Item> Map ;
	std::string m_string ;
	std::vector<Item> m_list ;
	Map m_map ;
	std::vector<std::string> m_keys ; // for key ordering
} ;

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Item & item )
	{
		item.out( stream ) ;
		return stream ;
	}
	inline
	std::istream & operator>>( std::istream & stream , Item & item )
	{
		item.in( stream ) ;
		return stream ;
	}
}

#endif

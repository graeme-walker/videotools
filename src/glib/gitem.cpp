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
// gitem.cpp
//

#include "gdef.h"
#include "gitem.h"
#include "gstr.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

G::Item G::Item::list()
{
	return Item(t_list) ;
}

G::Item G::Item::map()
{
	return Item(t_map) ;
}

G::Item G::Item::string()
{
	return Item(t_string) ;
}

G::Item G::Item::parse( const std::string & s )
{
	std::istringstream ss ;
	ss.str( s ) ;
	Item item ;
	ss >> item ;
	return item ;
}

bool G::Item::parse( const std::string & s , Item & out )
{
	try
	{
		std::istringstream ss ;
		ss.str( s ) ;
		ss >> out ;
		return true ;
	}
	catch( std::exception & )
	{
		return false ;
	}
}

std::string G::Item::str( int indent ) const
{
	std::ostringstream ss ;
	out( ss , indent ) ;
	return ss.str() ;
}

G::Item::Item( Type type ) :
	m_type(type)
{
}

G::Item::Item( const std::string & s ) :
	m_type(t_string) ,
	m_string(s)
{
}

G::Item::Type G::Item::type() const
{
	return m_type ;
}

const G::Item & G::Item::operator[]( const std::string & k ) const
{
	check( t_map ) ; need( k ) ;
	return (*m_map.find(k)).second ;
}

G::Item & G::Item::operator[]( const std::string & k )
{
	check( t_map ) ; update( k ) ;
	return m_map[k] ;
}

G::Item & G::Item::operator[]( size_t i )
{
	check( t_list ) ; need( i ) ;
	return m_list.at(i) ;
}

void G::Item::add( const std::string & s )
{
	add( Item(s) ) ;
}

void G::Item::add( Item i )
{
	check( t_list ) ;
	m_list.push_back( i ) ;
}

void G::Item::add( const std::string & k , const std::string & s )
{
	add( k , Item(s) ) ;
}

void G::Item::add( const std::string & k , const Item & i )
{
	check( t_map ) ; update( k ) ;
	m_map[k] = i ;
}

void G::Item::remove( const std::string & k )
{
	Map::iterator p = m_map.find( k ) ;
	if( p != m_map.end() ) 
	{
		m_map.erase( p ) ;
		m_keys.erase( std::remove(m_keys.begin(),m_keys.end(),k) , m_keys.end() ) ;
	}
}

void G::Item::update( const std::string & k )
{
	if( std::find(m_keys.begin(),m_keys.end(),k) == m_keys.end() )
		m_keys.push_back( k ) ;
}

bool G::Item::has( const std::string & k ) const
{
	check( t_map ) ;
	return std::find(m_keys.begin(),m_keys.end(),k) != m_keys.end() ;
}

G::Item G::Item::keys() const
{
	check( t_map ) ;
	Item out( t_list ) ;
	out.m_list.reserve( m_keys.size() ) ;
	for( std::vector<std::string>::const_iterator p = m_keys.begin() ; p != m_keys.end() ; ++p )
		out.add( *p ) ;
	return out ;
}

size_t G::Item::size() const
{
	checkNot( t_string ) ;
	return std::max(m_list.size(),m_map.size()) ;
}

bool G::Item::empty() const
{
	if( m_type == t_list ) 
		return m_list.empty() ;
	else if( m_type == t_map) 
		return m_map.empty() ;
	else 
		return m_string.empty() ;
}

const G::Item & G::Item::operator[]( size_t i ) const
{
	check( t_list ) ; need( i ) ;
	return m_list.at(i) ;
}

std::string G::Item::operator()() const
{
	check( t_string ) ;
	return m_string ;
}

void G::Item::checkNot( Type type ) const
{
	if( m_type == type )
		throw Error("type mismatch") ;
	check() ;
}

void G::Item::check( Type type ) const
{
	if( m_type != type )
		throw Error("type mismatch") ;
	check() ;
}

void G::Item::check() const
{
	if( m_type == t_map && m_keys.size() != m_map.size() )
		throw Error("internal error") ;
	if( m_map.size() != 0U && m_list.size() != 0U )
		throw Error("internal error") ;
}

void G::Item::need( const std::string & k ) const
{
	if( m_map.find(k) == m_map.end() )
		throw Error("item not found",k) ;
}

void G::Item::need( size_t i ) const
{
	if( i >= m_list.size() )
		throw Error("invalid index") ;
}

bool G::Item::less( const Item & a , const Item & b )
{
	return a.m_string < b.m_string ;
}

namespace
{
	std::string sp0( int indent )
	{
		if( indent < 0 ) return " " ;
		return "\n" + std::string(indent*2,' ') ;
	}
	std::string sp( int indent )
	{
		if( indent < 0 ) return ", " ;
		return ",\n" + std::string(indent*2,' ') ;
	}
	std::string spx( int indent )
	{
		if( indent < 0 ) return " " ;
		if( indent ) indent-- ;
		return "\n" + std::string(indent*2,' ') ;
	}
}

void G::Item::out( std::ostream & s , int indent ) const
{
	if( m_type == t_string )
	{
		static const std::string specials_in( "\r\n\t" , 4U ) ; // includes nul
		static const std::string specials_out( "rnt0" ) ;
		s << "\"" << G::Str::escaped(m_string,'\\',specials_in,specials_out) << "\"" ;
	}
	else if( m_type == t_list )
	{
		s << "[" ;
		std::string sep = sp0(indent) ;
		for( size_t i = 0U ; i < m_list.size() ; i++ , sep = sp(indent) )
		{
			s << sep ;
			m_list.at(i).out( s , indent+(indent>0) ) ;
		}
		s << spx(indent) << "]" ;
	}
	else if( m_type == t_map )
	{
		s << "{" ;
		std::string sep = sp0(indent) ;
		for( std::vector<std::string>::const_iterator p = m_keys.begin() ; p != m_keys.end() ; ++p , sep = sp(indent) )
		{
			s << sep << "\"" << *p << "\" : " ;
			if( m_map.find(*p) != m_map.end() ) // always true
				(*m_map.find(*p)).second.out( s , indent+(indent>0) ) ;
		}
		s << spx(indent) << "}" ;
	}
}

void G::Item::clear( Type type )
{
	m_string.clear() ;
	m_map.clear() ;
	m_keys.clear() ;
	m_list.clear() ;
	m_type = type ;
}

void G::Item::read( std::istream & stream , char c )
{
	for(;;)
	{
		std::string part ;
		if( !std::getline(stream,part,c) )
			throw Error() ; // (eof is an error)
		m_string.append( part ) ;
		if( !escaped(m_string) )
			break ;
		m_string.append( 1U , c ) ;
	}
	G::Str::unescape( m_string , '\\' , "rnt0" , "\r\n\t" ) ;
}

bool G::Item::escaped( const std::string & s )
{
	typedef std::string::const_iterator ptr_t ;
	bool e = false ;
	for( ptr_t p = s.begin() ; p != s.end() ; ++p )
	{
		if( *p == '\\' )
			e = !e ;
		else
			e = false ;
	}
	return e ;
}

void G::Item::readn( std::istream & s , char c0 )
{
	m_string = std::string( 1U , c0 ) ;
	for(;;)
	{
		char c = '\0' ;
		c = s.get() ;
		if( c >= '0' && c <= '9' )
		{
			m_string.append( 1U , c ) ;
		}
		else
		{
			s.unget() ;
			break ;
		}
	}
}

void G::Item::in( std::istream & stream )
{
	stream.setf( std::ios_base::skipws ) ;
	clear() ;
	char c ;
	stream >> c ;
	if( !stream.good() )
	{
	}
	else if( c == '"' || c == '\'' )
	{
		m_type = t_string ;
		read( stream , c ) ;
	}
	else if( c >= '0' && c <= '9' )
	{
		m_type = t_string ;
		readn( stream , c ) ;
	}
	else if( c == '[' )
	{
		m_type = t_list ;
		for(;;)
		{
			m_list.push_back( Item() ) ;
			stream >> m_list.back() ;
			stream >> c ;
			if( c == ']' ) break ;
			if( c != ',' ) throw ParseError() ;
		}
	}
	else if( c == '{' )
	{
		m_type = t_map ;
		for(;;)
		{
			stream >> c ;
			if( c == '}' ) break ;
			if( c != '"' && c != '\'' ) throw ParseError() ;
			Item key( t_string ) ;
			key.read( stream , c ) ;
			if( key.m_string.empty() ) throw ParseError() ;
			stream >> c ;
			if( c != ':' ) throw ParseError() ;
			Item value ;
			stream >> value ;
			update(key.m_string) ;
			m_map[key.m_string] = value ;
			stream >> c ;
			if( c == '}' ) break ;
			if( c != ',' ) throw ParseError() ;
		}
	}
	else
	{
		throw ParseError() ;
	}
}

/// \file gitem.cpp

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
// gurl.cpp
//

#include "gdef.h"
#include "gurl.h"
#include "gstr.h"
#include "gassert.h"
#include <map>
#include <string>

G::Url::Url()
{
	init( std::string() ) ;
}

G::Url::Url( const std::string & url )
{
	init( url ) ;
}

std::string G::Url::str() const
{
	return join( m_protocol , m_authorisation , address() , m_path , m_params , m_anchor ) ;
}

std::string G::Url::join( const std::string & protocol , const std::string & authorisation ,
	const std::string & address , const std::string & path , const std::string & params ,
	const std::string & anchor )
{
	std::string result ;
	result.reserve( 6U + protocol.size() + authorisation.size() + 
		address.size() + path.size() + params.size() + anchor.size() ) ;

	if( !protocol.empty() )
		result.append( protocol + "://" ) ;

	if( !authorisation.empty() )
		result.append( authorisation + "@" ) ;

	if( !address.empty() )
		result.append( address ) ;

	result.append( path ) ;
	if( path.empty() )
		result.append( "/" ) ;

	if( !params.empty() )
		result.append( "?" + params ) ;

	if( !anchor.empty() )
		result.append( "#" + anchor ) ;

	return result ;
}

std::string G::Url::summary() const
{
	std::ostringstream ss ;
	ss << protocol() << "://" << address() ;
	if( path() != "/" )
		ss << path() ;
	return ss.str() ;
}

void G::Url::init( std::string url )
{
	typedef std::string::size_type pos_t ;
	const pos_t npos = std::string::npos ;

	pos_t pos = 0U ;

	// "protocol://user:pwd@host:port/path1/path2.ext?key1=value1&key2#anchor".

	// remove and save the protocol
	if( (pos = url.find("://")) != npos )
	{
		m_protocol = G::Str::head( url , pos ) ;
		url = G::Str::tail( url , pos+2U ) ;
	}

	// remove and save the authorisation and address
	pos = url.find("/") ;
	{
		m_address = G::Str::head( url , pos , url ) ;
		url = G::Str::tail( url , pos ) ;
		if( (pos = m_address.find("@")) != npos )
		{
			m_authorisation = G::Str::head( m_address , pos ) ;
			m_address = G::Str::tail( m_address , pos ) ;
		}
		if( m_address.find('[') == 0U && m_address.find(']') != std::string::npos )
		{
			size_t closepos = m_address.find(']') ;
			size_t colonpos = m_address.rfind(':') ;
			if( colonpos != std::string::npos && colonpos < closepos ) 
				colonpos = std::string::npos ;
			m_host = G::Str::head( m_address , colonpos , m_address ) ;
			m_port = G::Str::tail( m_address , colonpos ) ;
			G::Str::removeAll( m_host , '[' ) ;
			G::Str::removeAll( m_host , ']' ) ;
		}
		else
		{
			size_t colonpos = m_address.rfind(':') ;
			m_host = G::Str::head( m_address , colonpos , m_address ) ;
			m_port = G::Str::tail( m_address , colonpos ) ;
		}
		if( m_host.empty() ) 
		{
			m_port.clear() ;
			m_address.clear() ;
		}
	}

	// remove and save the path, leave just the parameters and anchor
	pos = url.find("?") ;
	{
		m_path = "/" + G::Str::head( url , pos , url ) ;
		url = G::Str::tail( url , pos , "" ) ;

		m_path = G::Str::unique( m_path , '/' , '/' ) ;
		if( m_path.size() > 1U && m_path.at(m_path.size()-1U) == '/' )
			m_path.resize( m_path.size() - 1U ) ;
	}
	G_ASSERT( !m_path.empty() ) ;

	// remove and save the anchor, leave just the parameters
	if( (pos = url.find("#")) != npos )
	{
		m_anchor = G::Str::tail( url , pos ) ;
		url = G::Str::head( url , pos ) ;
	}

	// save the params string verbatim
	m_params = url ;

	// decode the params into a multimap
	G::StringArray parts ;
	G::Str::splitIntoTokens( m_params , parts , "&" ) ;
	for( G::StringArray::iterator p = parts.begin() ; p != parts.end() ; ++p )
	{
		pos = (*p).find("=") ;
		std::string key = decode( G::Str::head(*p,pos,*p) ) ;
		std::string value = decode( G::Str::tail(*p,pos) ) ;
		m_params_map.insert( Map::value_type(key,value) ) ;
	}
}

std::string G::Url::request() const
{
	return path() + (m_params.empty()?"":"?") + m_params ;
}

std::string G::Url::parameters() const
{
	return m_params ;
}

G::Url::Map G::Url::pmap() const
{
	return m_params_map ;
}

std::string G::Url::protocol() const
{
	return m_protocol ;
}

std::string G::Url::host() const
{
	return m_host ;
}

std::string G::Url::port( const std::string & default_ ) const
{
	return m_port.empty() ? default_ : m_port ;
}

std::string G::Url::address() const
{
	return m_address ;
}

std::string G::Url::address( const std::string & default_port ) const
{
	G_ASSERT( !default_port.empty() ) ;
	if( default_port.empty() ) return host() + ":0" ;
	return G::Str::join( ":" , host() , port(default_port) ) ;
}

bool G::Url::has( const std::string & key ) const
{
	return m_params_map.find(key) != m_params_map.end() ;
}

std::string G::Url::parameter( std::string key , std::string default_ ) const
{
	Map::const_iterator p = m_params_map.find(key) ;
	if( p == m_params_map.end() )
		return default_ ;
	else
		return (*p).second ; // first one
}

std::string G::Url::path() const
{
	return m_path ;
}

std::string G::Url::authorisation() const
{
	return m_authorisation ;
}

std::string G::Url::anchor() const
{
	return m_anchor ;
}

size_t G::Url::colonpos( const std::string & address )
{
	// find trailing colon outside of any ipv6 square backets
	const std::string::size_type npos = std::string::npos ;
	if( address.empty() ) return npos ;
	size_t colon = address.rfind( ":" ) ;
	if( colon == npos ) return npos ;
	size_t end = address.at(0U) == '[' ? address.rfind("]") : npos ;
	return ( end != npos && colon < end ) ? npos : colon ;
}

std::string G::Url::decode( const std::string & s )
{
	std::string result ;
	result.reserve( s.size() ) ;
	size_t n = s.size() ;
	for( size_t i = 0U ; i < n ; i++ )
	{
		if( s.at(i) == '%' && (i+2U) < n )
		{
			static std::string map = "00112233445566778899aAbBcCdDeEfF" ;
			char c1 = s.at(++i) ;
			char c2 = s.at(++i) ;
			size_t n1 = map.find(c1) ; 
			size_t n2 = map.find(c2) ; 
			if( n1 == std::string::npos ) n1 = 0U ; // moot
			if( n2 == std::string::npos ) n2 = 0U ;
			unsigned int x = ((n1>>1)<<4) | (n2>>1) ;
			result.append( 1U , static_cast<char>(x) ) ;
		}
		else if( s.at(i) == '+' )
		{
			result.append( 1U , ' ' ) ;
		}
		else
		{
			result.append( 1U , s.at(i) ) ;
		}
	}
	return result ;
}

std::string G::Url::encode( const std::string & s , bool plus_for_space )
{
	std::string result ;
	result.reserve( s.size() ) ;
	size_t sn = s.size() ;
	for( size_t i = 0U ; i < sn ; i++ )
	{
		char c = s.at( i ) ;
		if( ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || 
			c == '-' || c == '_' || c == '.' || c == '~' )
		{
			result.append( 1U , c ) ;
		}
		else if( c == ' ' && plus_for_space )
		{
			result.append( 1U , '+' ) ;
		}
		else
		{
			static const char * map = "0123456789ABCDEF" ;
			unsigned int n = static_cast<unsigned char>(c) ;
			result.append( 1U , '%' ) ;
			result.append( 1U , map[(n>>4)%16U] ) ;
			result.append( 1U , map[n%16U] ) ;
		}
	}
	return result ;
}

/// \file gurl.cpp

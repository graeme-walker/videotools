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
// gvsdp.cpp
//

#include "gdef.h"
#include "gvsdp.h"
#include "gstr.h"
#include "gdebug.h"

Gv::Sdp::Sdp( const std::vector<std::string> & lines )
{
	//if( lines.size() <= 1 ) throw Error( "no data" ) ;
	//if( lines.at(0U) != "v=0" ) throw Error( "invalid version" ) ;

	typedef std::vector<std::string> StringArray ;
	for( StringArray::const_iterator p = lines.begin() ; p != lines.end() ; ++p )
	{
		G_DEBUG( "Sdp::ctor: [" << *p << "]" ) ;
		if( (*p).length() <= 2U ) throw Error() ;
		if( (*p).at(1U) != '=' ) throw Error() ;

		if( (*p).at(0U) == 't' || (*p).at(0U) == 'r' )
		{
			if( (*p).at(0U) == 't' )
				m_time.push_back( Map() ) ;
			m_time.back().insert( pair(*p) ) ;
		}
		else if( (*p).at(0U) == 'm' )
		{
			m_media.push_back( Map() ) ;
			m_media.back().insert( pair(*p) ) ;
		}
		else if( m_media.empty() )
		{
			m_session.insert( pair(*p) ) ;
		}
		else
		{
			m_media.back().insert( pair(*p) ) ;
		}
	}
}

Gv::Sdp::Map::value_type Gv::Sdp::pair( const std::string & line )
{
	return Map::value_type( line.substr(0U,1U) , line.substr(2U) ) ;
}

std::string Gv::Sdp::originator() const
{
	return value( "o" ) ;
}

std::string Gv::Sdp::name() const
{
	return value( "s" ) ;
}

Gv::Sdp::Map Gv::Sdp::attributes() const
{
	return attributes( m_session ) ;
}

std::string Gv::Sdp::attributes( const std::string & sep ) const
{
	return str( attributes(m_session) , sep ) ;
}

size_t Gv::Sdp::timeCount() const
{
	return m_time.size() ;
}

size_t Gv::Sdp::mediaCount() const
{
	return m_media.size() ;
}

std::string Gv::Sdp::mediaType( size_t index ) const
{
	return value( m_media.at(index) , "m" ) ;
}

std::string Gv::Sdp::mediaTitle( size_t index ) const
{
	return value( m_media.at(index) , "i" , "undefined" ) ;
}

std::string Gv::Sdp::mediaConnection( size_t index ) const
{
	return value( m_media.at(index) , m_session , "c" ) ;
}

Gv::Sdp::Map Gv::Sdp::mediaAttributes( size_t index ) const
{
	return attributes( m_media.at(index) ) ;
}

std::string Gv::Sdp::mediaAttributes( size_t index , const std::string & sep ) const
{
	return str( attributes(m_media.at(index)) , sep ) ;
}

std::string Gv::Sdp::mediaAttribute( size_t index , const std::string & key ) const
{
	return value( mediaAttributes(index) , key ) ;
}

std::string Gv::Sdp::mediaAttribute( size_t index , const std::string & key , const std::string & default_ ) const
{
	return value( mediaAttributes(index) , key , default_ ) ;
}

// ==

Gv::Sdp::Map Gv::Sdp::attributes( const Map & map )
{
	Map result ;
	const std::string a( "a" ) ;
	for( Map::const_iterator p = map.find(a) ; p != map.end() && (*p).first == a ; ++p )
	{
		std::string v = (*p).second ;
		std::string::size_type pos = v.find(":") ;
		result.insert( Map::value_type( G::Str::head(v,pos,v) , G::Str::tail(v,pos) ) ) ;
	}
	return result ;
}

std::string Gv::Sdp::str( const Map & map , const std::string & sep )
{
	std::string result ;
	for( Map::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		if( p != map.begin() ) result.append( sep ) ;
		result.append( (*p).first ) ;
		if( !(*p).second.empty() )
		{
			result.append( ":" ) ;
			result.append( (*p).second ) ;
		}
	}
	return result ;
}

std::string Gv::Sdp::value( const std::string & key ) const
{
	return value( m_session , key ) ;
}

std::string Gv::Sdp::value( const Map & map_1 , const Map & map_2 , const std::string & key )
{
	Map::const_iterator p = map_1.find( key ) ;
	if( p == map_1.end() )
	{
		p = map_2.find( key ) ;
		if( p == map_2.end() )
			throw Error( "no such value" ) ;
	}
	return (*p).second ;
}

std::string Gv::Sdp::value( const Map & map , const std::string & key , const std::string & default_ )
{
	Map::const_iterator p = map.find( key ) ;
	return p == map.end() ? default_ : (*p).second ;
}

std::string Gv::Sdp::value( const Map & map , const std::string & key )
{
	Map::const_iterator p = map.find( key ) ;
	if( p == map.end() )
		throw Error( "no such value" ) ;
	return (*p).second ;
}

std::string Gv::Sdp::fmtp() const
{
	std::string result ;
	for( size_t i = 0 ; i < m_media.size() ; i++ )
	{
		if( mediaType(i).find("video") == 0U && mediaType(i).find("RTP/AVP") != std::string::npos )
		{
			std::string value = mediaAttribute( i , "fmtp" , std::string() ) ;
			if( !value.empty() )
			{
				result = G::Str::printable( G::Str::tail(value,value.find(" "),value) ) ; // without the initial payload type number
				break ;
			}
		}
	}
	return result ;
}

/// \file gvsdp.cpp

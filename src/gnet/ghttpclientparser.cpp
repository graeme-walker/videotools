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
// ghttpclientparser.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "ghttpclientprotocol.h"
#include "gassert.h"
#include "gstr.h"
#include "gurl.h"
#include "gbase64.h"
#include "gmd5.h"
#include "glimits.h"
#include "ghexdump.h"
#include "glog.h"
#include <algorithm> // std::search()
#include <stdexcept>

GNet::HttpClientParser::HttpClientParser( const std::string & protocol ) :
	m_sep(std::string::npos)
{
	G_ASSERT( protocol.empty() || protocol.size() == 4U ) ;
	m_http = protocol.empty() ? "HTTP/1." : (protocol+"/1.") ;
	m_data.reserve( 100000U ) ;
}

void GNet::HttpClientParser::apply( const char * p , size_t n )
{
	m_data.append( p , n ) ;
	if( m_data.size() > G::limits::net_file_limit )
		throw std::runtime_error( "receive buffer size limit exceeded" ) ;

	// look for a blank line separator and parse the header when found
	if( m_sep == std::string::npos )
	{
		m_sep = m_data.find( "\r\n\r\n" ) ;
		if( m_sep != std::string::npos )
		{
			m_hlist = headerInfo( m_data ) ;
			m_response_status = responseStatusInfo( m_http , m_data ) ;
		}
	}
}

GNet::HttpClientParser::List GNet::HttpClientParser::headerInfo( const std::string & data )
{
	List result ;
	typedef std::string::size_type pos_t ;
	const pos_t npos = std::string::npos ;
	for( pos_t pos = 0U ;; pos += 2U )
	{
		pos_t next_pos = data.find( "\r\n" , pos ) ;
		if( next_pos == npos || pos == next_pos ) break ;
		result.push_back( HeaderInfo(data.substr(pos,next_pos-pos)) ) ;
		pos = next_pos ;
	}
	return result ;
}

void GNet::HttpClientParser::clear()
{
	m_data.clear() ;
	m_sep = std::string::npos ;
	m_hlist.clear() ;
	m_response_status = ResponseStatusInfo() ;
}

GNet::HttpClientParser::ResponseStatusInfo GNet::HttpClientParser::responseStatusInfo( const std::string & http , 
	const std::string & data )
{
	// "HTTP/1.x NNN reason...\r\n"

	ResponseStatusInfo result ;

	const size_t npos = std::string::npos ;
	size_t pos1 = data.find(' ') ;
	size_t pos2 = (pos1==npos||(pos1+1U)==data.size()) ? npos : data.find(" ",pos1+1U) ;
	size_t pos3 = (pos2==npos||(pos2+1U)==data.size()) ? npos : data.find("\n",pos2+1U) ;
	if( data.find(http) == 0U && pos1 != npos && pos2 != npos && pos3 != npos )
	{
		std::string s1 = G::Str::trimmed( data.substr(pos1,pos2-pos1) , " " ) ;
		std::string s2 = G::Str::trimmed( data.substr(pos2,pos3-pos2) , " \r\n" ) ;
		result.value = ( s1.empty() || !G::Str::isUInt(s1) ) ? -1 : G::Str::toInt(s1) ;
		result.ok = result.value >= 200 && result.value < 300 ;
		result.unauthorised = result.value == 401 && s2 == "Unauthorized" ;
		result.retry = result.value == 503 ? 1U : 0U ; // TODO parse out Retry-After value
	}
	return result ;
}

int GNet::HttpClientParser::responseValue() const
{
	return m_response_status.value ;
}

bool GNet::HttpClientParser::responseUnauthorised() const
{
	return m_response_status.unauthorised ;
}

bool GNet::HttpClientParser::responseOk() const
{
	return m_response_status.ok ;
}

bool GNet::HttpClientParser::responseRetry() const
{
	return m_response_status.retry > 0U ;
}

bool GNet::HttpClientParser::gotHeaders() const
{
	return m_sep != std::string::npos ;
}

size_t GNet::HttpClientParser::headerCount() const
{
	return m_hlist.size() ;
}

std::string GNet::HttpClientParser::responseSummary() const
{
	const size_t npos = std::string::npos ;
	size_t start = m_data.find(" ") ;
	size_t end = m_data.find_first_of("\n\r") ;
	std::string s = ( start != npos && end != npos && end > start ) ? m_data.substr(start+1U,end-start) : std::string() ;
	return G::Str::printable( G::Str::trimmed( s , G::Str::ws() ) ) ;
}

GNet::HttpClientParser::HeaderInfo::HeaderInfo( const std::string & line ) :
	m_line(line)
{
	size_t pos = line.find( ":" ) ;
	m_lhs = G::Str::trimmed( G::Str::head(m_line,pos,m_line) , " " ) ;
	m_rhs = G::Str::trimmed( G::Str::tail(m_line,pos,std::string()) , " " ) ;
	m_value = G::Str::trimmed( G::Str::head(m_rhs,m_rhs.find(";"),m_rhs) , " " ) ;
	G::Str::splitIntoTokens( m_rhs , m_tokens , ";, " ) ;
}

bool GNet::HttpClientParser::HeaderInfo::match( const std::string & key ) const
{
	return G::Str::imatch( key , m_lhs ) ;
}

int GNet::HttpClientParser::HeaderInfo::value( int default_ ) const
{
	return 
		!m_value.empty() && G::Str::isUInt(m_value) ?
			G::Str::toInt(m_value) : 
			default_ ;
}

GNet::HttpClientParser::List::const_iterator GNet::HttpClientParser::hfind( const List & hlist , 
	const std::string & key )
{
	for( List::const_iterator p = hlist.begin() ; p != hlist.end() ; ++p )
	{
		if( (*p).match(key) )
			return p ;
	}
	return hlist.end() ;
}

GNet::HttpClientParser::List::const_iterator GNet::HttpClientParser::hfind( const List & hlist , size_t index )
{
	if( index >= hlist.size() ) throw std::range_error( "invalid http header index" ) ;
	return hlist.begin() + index ;
}

std::vector<size_t> GNet::HttpClientParser::headers( const std::string & key ) const
{
	std::vector<size_t> result ;
	size_t i = 0U ;
	for( List::const_iterator p = m_hlist.begin() ; p != m_hlist.end() ; ++p , i++ )
	{
		if( (*p).match(key) )
			result.push_back( i ) ;
	}
	return result ;
}

std::string GNet::HttpClientParser::header( const std::string & key , const std::string & default_ ) const
{
	List::const_iterator p = hfind( m_hlist , key ) ;
	return p == m_hlist.end() ? default_ : (*p).m_rhs ;
}

std::string GNet::HttpClientParser::header( size_t index , const std::string & default_ ) const
{
	List::const_iterator p = hfind( m_hlist , index ) ;
	return p == m_hlist.end() ? default_ : (*p).m_rhs ;
}

int GNet::HttpClientParser::headerValue( const std::string & key , int default_ ) const
{
	List::const_iterator p = hfind( m_hlist , key ) ;
	return p == m_hlist.end() ? default_ : (*p).value(default_) ;
}

int GNet::HttpClientParser::headerValue( size_t index , int default_ ) const
{
	List::const_iterator p = hfind( m_hlist , index ) ;
	return p == m_hlist.end() ? default_ : (*p).value(default_) ;
}

size_t GNet::HttpClientParser::headerContentLength() const
{
	return static_cast<size_t>( headerValue("Content-Length",0) ) ;
}

std::string GNet::HttpClientParser::headerWord( const std::string & key , const std::string & default_ ) const
{
	List::const_iterator p = hfind( m_hlist , key ) ;
	return p == m_hlist.end() || (*p).m_tokens.empty() ? default_ : (*p).m_tokens.at(0U) ;
}

std::string GNet::HttpClientParser::headerWord( size_t index , const std::string & default_ ) const
{
	List::const_iterator p = hfind( m_hlist , index ) ;
	return p == m_hlist.end() || (*p).m_tokens.empty() ? default_ : (*p).m_tokens.at(0U) ;
}

std::string GNet::HttpClientParser::headerAttribute( const std::string & key , const std::string & attribute_key , 
	const std::string & default_ ) const
{
	List::const_iterator p = hfind( m_hlist , key ) ;
	return p == m_hlist.end() ?  default_ : headerAttribute( p , attribute_key , default_ ) ;
}

std::string GNet::HttpClientParser::headerAttribute( size_t index , const std::string & attribute_key , 
	const std::string & default_ ) const
{
	List::const_iterator p = hfind( m_hlist , index ) ;
	return p == m_hlist.end() ?  default_ : headerAttribute( p , attribute_key , default_ ) ;
}

std::string GNet::HttpClientParser::headerAttribute( List::const_iterator p , const std::string & attribute_key ,
	const std::string & default_ ) const
{
	const G::StringArray & attributes = (*p).m_tokens ;
	for( size_t i = 0U ; i < attributes.size() ; i++ )
	{
		if( attributes[i].find(attribute_key+"=") == 0U )
		{
			std::string result = attributes[i].substr(attribute_key.length()+1U) ;
			if( result.find("\"") == 0U && result.length() >= 2U && 
				(result.rfind("\"")+1U) == result.length() )
			{
				result = result.substr(1U,result.length()-2U) ;
			}
			return result ;
		}
	}
	return default_ ;
}

std::string GNet::HttpClientParser::headerContentType() const
{
	return headerWord( "Content-Type" ) ;
}

bool GNet::HttpClientParser::headerMultipart() const
{
	std::string content_type = headerContentType() ;
	bool multipart = content_type.find("multipart") == 0U ;
	return multipart ;
}

std::string GNet::HttpClientParser::headerMultipartBoundary() const
{
	if( !headerMultipart() ) return std::string() ;
	return headerAttribute( "Content-Type" , "boundary" ) ;
}

bool GNet::HttpClientParser::gotBody() const
{
	return
		m_sep != std::string::npos &&
		m_data.size() >= ( m_sep + 4U + headerContentLength() ) ;
}

std::string GNet::HttpClientParser::body() const
{
	return std::string( bodyData() , bodySize() ) ;
}

const char * GNet::HttpClientParser::bodyData() const
{
	return m_sep == std::string::npos ? nullptr : ( m_data.data() + m_sep + 4U ) ;
}

size_t GNet::HttpClientParser::bodySize() const
{
	return m_sep == std::string::npos ? 0U : ( m_data.size() - m_sep - 4U ) ;
}

void GNet::HttpClientParser::clearPart()
{
	PartInfo part_info = partInfo() ;
	m_data.erase( part_info.start , part_info.headersize+part_info.bodysize ) ;
}

bool GNet::HttpClientParser::gotPart() const
{
	PartInfo part_info = partInfo() ;
	return part_info.start != 0U && m_data.size() >= (part_info.start+part_info.headersize+part_info.bodysize) ;
}

GNet::HttpClientParser::PartInfo GNet::HttpClientParser::partInfo() const
{
	PartInfo part_info ;
	std::string::size_type npos = std::string::npos ;
	std::string::size_type pos1 = m_data.find( "\r\n--" + headerMultipartBoundary() + "\r\n" ) ;
	std::string::size_type pos2 = G::Str::ifind( m_data , "\nContent-Length: " , pos1 ) ;
	std::string::size_type pos2a = G::Str::ifind( m_data , "\nContent-Type: " , pos1 ) ;
	std::string::size_type pos3 = m_data.find( "\r\n" , pos2 ) ;
	std::string::size_type pos3a = m_data.find( "\r\n" , pos2a ) ;
	std::string::size_type pos4 = m_data.find( "\r\n\r\n" , pos2 ) ;
	if( pos1 != npos && pos2 != npos && pos2a != npos && 
		pos3 != npos && pos3a != npos && pos4 != npos && 
		pos4 >= pos3 && pos4 >= pos3a )
	{
		std::string cl = m_data.substr( pos2+17U , pos3-pos2-17U ) ;
		if( !cl.empty() && G::Str::isUInt(cl) )
		{
			part_info.start = pos1 + 2U ;
			part_info.headersize = pos4 - part_info.start + 4U ;
			part_info.bodysize = G::Str::toUInt(cl) ;
			part_info.type = m_data.substr( pos2a+15U , pos3a-pos2a-15U ) ;
		}
	}
	return part_info ;
}

const char * GNet::HttpClientParser::partData() const
{
	PartInfo part_info = partInfo() ;
	return m_data.data() + part_info.start + part_info.headersize ;
}

size_t GNet::HttpClientParser::partSize() const
{
	PartInfo part_info = partInfo() ;
	return part_info.bodysize ;
}

std::string GNet::HttpClientParser::partType() const
{
	PartInfo part_info = partInfo() ;
	return part_info.type ;
}

// ==

GNet::HttpClientParser::ResponseStatusInfo::ResponseStatusInfo()
{
	ok = false ;
	value = -1 ;
	unauthorised = false ;
	retry = 0U ;
}

/// \file ghttpclientparser.cpp

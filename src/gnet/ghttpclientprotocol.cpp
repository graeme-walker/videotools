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
// ghttpclientprotocol.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "ghttpclientprotocol.h"
#include "gstr.h"
#include "gurl.h"
#include "gbase64.h"
#include "gmd5.h"
#include "ghexdump.h"
#include "glog.h"

GNet::HttpClientProtocol::HttpClientProtocol( Callback & callback , const G::Url & url ) :
	m_callback(callback) ,
	m_url(url) ,
	m_tried_auth(false)
{
}

void GNet::HttpClientProtocol::start()
{
	sendGet() ;
}

void GNet::HttpClientProtocol::apply( const char * p , std::string::size_type n )
{
	G_DEBUG( "HttpClientProtocol::apply: " << n << " bytes\n" << G::hexdump<16>(p,p+n) ) ;
	m_parser.apply( p , n ) ;
	processData() ;
}

void GNet::HttpClientProtocol::processData()
{
	G_DEBUG( "HttpClientProtocol::apply: "
		<< "have=" << m_parser.bodySize() << " "
		<< "want=" << m_parser.headerContentLength() << " "
		<< "multipart=" << m_parser.headerMultipart() << " "
		<< "ok=" << m_parser.responseOk() << " "
		<< "got-headers=" << m_parser.gotHeaders() << " "
		<< "got-body=" << m_parser.gotBody() << " "
	) ;

	if( !m_parser.gotHeaders() )
	{
		; // wait for more
	}
	else if( m_parser.responseUnauthorised() && m_url.authorisation().empty() )
	{
		throw Auth( "authorisation required" ) ;
	}
	else if( m_parser.responseUnauthorised() && m_parser.gotBody() && m_tried_auth )
	{
		throw Fail( "authorisation failed" ) ;
	}
	else if( m_parser.responseUnauthorised() && m_parser.gotBody() )
	{
		m_tried_auth = true ;
		sendGetWithAuth() ;
		m_parser.clear() ;
	}
	else if( m_parser.responseRetry() )
	{
		throw Retry( "service unavailable" ) ; // 503
	}
	else if( m_parser.responseOk() && m_parser.headerMultipart() && m_parser.gotPart() )
	{
		processPart() ;
		m_parser.clearPart() ;
	}
	else if( m_parser.responseOk() && m_parser.headerMultipart() )
	{
		; // wait for more
	}
	else if( m_parser.responseOk() && m_parser.gotBody() )
	{
		processBody() ;
		m_parser.clear() ;
	}
	else if( m_parser.responseOk() )
	{
		; // wait for more
	}
	else
	{
		throw Fail( "unexpected http response: [" + G::Str::printable(m_parser.responseSummary()) + "]" ) ;
	}
}

void GNet::HttpClientProtocol::sendGet()
{
	std::string get = "GET " + m_url.request() + " HTTP/1.1\r\n" + "\r\n" ;
	G_LOG( "GNet::HttpClientProtocol::sendGet: sending [" << G::Str::printable(get) << "]" ) ;
	m_callback.sendHttpRequest( get ) ;
}

std::string GNet::HttpClientProtocol::authorisation( const GNet::HttpClientParser & parser , 
	const G::Url & url , std::string command , bool long_uri )
{
	if( command.empty() ) command = "GET" ;

	std::string auth = url.authorisation() ;
	std::string::size_type pos = auth.find(":") ;
	std::string user = G::Str::head( auth , pos ) ;
	std::string pwd = G::Str::tail( auth , pos ) ;

	std::string result ;
	typedef std::vector<size_t> List ;
	List hlist = parser.headers( "WWW-Authenticate" ) ;
	for( List::iterator p = hlist.begin() ; p != hlist.end() ; ++p )
	{
		size_t index = *p ;
		std::string auth_type = parser.headerWord( index ) ;
		std::string realm = parser.headerAttribute( index , "realm" ) ;
		std::string nonce = parser.headerAttribute( index , "nonce" ) ;
		std::string opaque = parser.headerAttribute( index , "opaque" ) ;
		if( auth_type == "Basic" )
		{
			result = "Authorization: Basic " + G::Base64::encode(user+":"+pwd,"") ;
			break ;
		}
		else if( auth_type == "Digest" )
		{
			// RFC-2069
			std::string uri = long_uri ? (url.protocol()+"://"+url.address()+url.path()) : url.path() ;
			std::string ha1 = hash( user + ":" + realm + ":" + pwd ) ;
			std::string ha2 = hash( command + ":" + uri ) ;
			std::string auth_response = hash( ha1 + ":" + nonce + ":" + ha2 ) ;
			result = std::string() +
				"Authorization: Digest " + 
					"username=\"" + user + "\", " +
					"realm=\"" + realm + "\", " +
					"nonce=\"" + nonce + "\", " +
					"uri=\"" + uri + "\", " +
					"response=\"" + auth_response + "\"" +
					(opaque.empty()?"":", opaque=\"") + opaque + (opaque.empty()?"":"\"") ;
			break ;
		}
	}
	if( result.empty() )
		throw Fail( "no supported authentication mechanism" ) ;
	return result ;
}

void GNet::HttpClientProtocol::sendGetWithAuth()
{
	std::string get = "GET " + m_url.request() + " HTTP/1.1\r\n" + 
		authorisation(m_parser,m_url) +
		"\r\n\r\n" ;

	G_LOG( "GNet::HttpClientProtocol::sendGetWithAuth: sending [" << G::Str::printable(get) << "]" ) ;
	m_callback.sendHttpRequest( get ) ;
}

std::string GNet::HttpClientProtocol::hash( const std::string & s )
{
	return G::Md5::printable( G::Md5::digest( s ) ) ;
}

void GNet::HttpClientProtocol::processBody()
{
	m_callback.onHttpBody( std::string() , m_parser.header("Content-Type") , m_parser.bodyData() , m_parser.bodySize() ) ;
}

void GNet::HttpClientProtocol::processPart()
{
	m_callback.onHttpBody( m_parser.header("Content-Type") , m_parser.partType() , m_parser.partData() , m_parser.partSize() ) ;
}

// ==

GNet::HttpClientProtocol::Callback::~Callback()
{
}

/// \file ghttpclientprotocol.cpp

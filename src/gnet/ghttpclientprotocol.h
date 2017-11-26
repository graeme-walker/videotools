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
/// \file ghttpclientprotocol.h
///

#ifndef G_HTTP_CLIENT_PROTOCOL__H
#define G_HTTP_CLIENT_PROTOCOL__H

#include "gdef.h"
#include "gnet.h"
#include "ghttpclientparser.h"
#include "gexception.h"
#include "gurl.h"
#include "gstrings.h"
#include <string>

namespace GNet
{
	class HttpClientProtocol ;
}

/// \class GNet::HttpClientProtocol
/// A protocol driver for an http client. Supports "multipart" responses, 
/// with each part is delivered separately as if a self-contained body. Also
/// supports digest authentication, with the authentication secrets being 
/// supplied in the URL.
/// 
class GNet::HttpClientProtocol
{
public:
	G_EXCEPTION( Auth , "http client protocol" ) ;
	G_EXCEPTION( Fail , "http client protocol error" ) ;
	G_EXCEPTION( Retry , "http client protocol" ) ;

	struct Callback /// A callback interface for GNet::HttpClientProtocol to send data and deliver content.
	{
		virtual void sendHttpRequest( const std::string & ) = 0 ;
			///< Called by the HttpClientProtocol when it needs to send 
			///< a http request string.

		virtual void onHttpBody( const std::string & outer_content_type , const std::string & content_type , 
			const char * body , size_t body_size ) = 0 ;
				///< Called on receipt of a complete response, or a multipart part.
				///< The first parameter is the empty string for a single-part
				///< response, and the outer content-type for a multipart part.

		virtual ~Callback() ;
			///< Destructor.
	} ;

	HttpClientProtocol( Callback & , const G::Url & url ) ;
		///< Constructor.

	void start() ;
		///< Assembles a GET request (based on the constructor url) and asks the 
		///< callback interface to send it.

	void apply( const char * , size_t ) ;
		///< To be called on receipt of data. Throws Auth if authentication is required
		///< but not supplied; throws Retry for a service-unavailble response; throws 
		///< Fail on error.

	static std::string authorisation( const GNet::HttpClientParser & , const G::Url & , 
		std::string get = std::string() , bool = false ) ;
			///< Returns an "Authorization" header for adding to a "GET" request.

private:
	HttpClientProtocol( const HttpClientProtocol & ) ;
	void operator=( const HttpClientProtocol & ) ;
	void processData() ;
	void processBody() ;
	void processPart() ;
	static std::string hash( const std::string & ) ;
	void sendGet() ;
	void sendGetWithAuth() ;

private:
	Callback & m_callback ;
	G::Url m_url ;
	HttpClientParser m_parser ;
	bool m_tried_auth ;
} ;

#endif

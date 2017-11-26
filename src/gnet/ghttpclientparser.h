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
/// \file ghttpclientparser.h
///

#ifndef G_HTTP_CLIENT_PARSER__H
#define G_HTTP_CLIENT_PARSER__H

#include "gdef.h"
#include "gnet.h"
#include "gstrings.h"
#include <string>
#include <utility>
#include <map>

namespace GNet
{
	class HttpClientParser ;
}

/// \class GNet::HttpClientParser
/// A parser for HTTP responses received from a remote server. Also works 
/// for RTSP.
/// 
/// The response is structured as a status line, followed by an optional 
/// set of headers, then a blank line, and then the optional body.
/// 
/// Data is added incrementally with apply(), and gotHeaders() and gotBody() 
/// indicate how complete the response is.
/// 
/// The body can be multi-part, as indicated by headerMultipart(). Multi-part 
/// streams can be be unending, so individual parts should normally be 
/// cleared once they have been processed (see gotPart(), partData() 
/// and clearPart()).
/// 
class GNet::HttpClientParser
{
public:
	explicit HttpClientParser( const std::string & protocol = std::string() ) ;
		///< Constructor. The protocol defaults to "HTTP".

	void apply( const char * p , size_t n ) ;
		///< Adds some data.

	void clear() ;
		///< Clears the contents, returning the object to a newly-constructed
		///< state. This is preferable to destruction and construction because
		///< the internal receive buffer can be large.

	bool gotHeaders() const ;
		///< Returns true if headers are complete.

	size_t headerCount() const ;
		///< Returns the number of headers.

	bool responseOk() const ;
		///< Returns true for a 2xx response.

	bool responseRetry() const ;
		///< Returns true for a 503 response.

	bool responseUnauthorised() const ;
		///< Returns true for a "401 Unauthorized" response.

	int responseValue() const ;
		///< Returns the response value (eg. 200), or minus one.

	std::vector<size_t> headers( const std::string & header_key ) const ;
		///< Returns the indexes for the headers with the given key.

	std::string header( const std::string & header_key , const std::string & default_ = std::string() ) const ;
		///< Returns the value of the given header. If the header
		///< is repeated then only the first one is relevant.

	std::string header( size_t index , const std::string & default_ = std::string() ) const ;
		///< Overload using the header index.

	int headerValue( const std::string & header_key , int default_ = -1 ) const ;
		///< Returns the integer value of a numeric header.

	int headerValue( size_t header_index , int default_ = -1 ) const ;
		///< Overload using the header index.

	std::string headerWord( const std::string & header_key , const std::string & default_ = std::string() ) const ;
		///< Returns the first part of the header with the given key.

	std::string headerWord( size_t header_index , const std::string & default_ = std::string() ) const ;
		///< Overload using the header index.

	std::string headerAttribute( const std::string & header_key , const std::string & attribute_key , 
		const std::string & default_ = std::string() ) const ;
			///< Returns a named attribute of the specified header.

	std::string headerAttribute( size_t header_index , const std::string & attribute_key , 
		const std::string & default_ = std::string() ) const ;
			///< Overload using the header index.

	std::string headerContentType() const ;
		///< Returns the value of the "Content-Type" header.
		
	size_t headerContentLength() const ;
		///< Returns the value of the "Content-Length" header.
		///< Returns zero on error.

	void clearPart() ;
		///< Clears the current multipart body part.

	bool gotBody() const ;
		///< Returns true if the body is complete.

	std::string body() const ;
		///< Returns the body data.
		///< Precondition: gotBody()

	const char * bodyData() const ;
		///< Returns the body data.
		///< Precondition: gotBody()

	size_t bodySize() const ;
		///< Returns the body size.
		///< Precondition: gotBody()

	bool headerMultipart() const ;
		///< Returns true if the main body is of type "multipart".

	bool gotPart() const ;
		///< Returns true if a multipart part is complete.

	std::string partType() const ;
		///< Returns the content-type of the part.
		///< Precondition: gotPart()

	const char * partData() const ;
		///< Returns the part data.
		///< Precondition: gotPart()

	size_t partSize() const ;
		///< Returns the part size.
		///< Precondition: gotPart()

	std::string responseSummary() const ;
		///< Returns a summary of the response for debugging and error reporting.

private:
	struct ResponseStatusInfo
	{
		ResponseStatusInfo() ;
		bool ok ;
		int value ;
		bool unauthorised ;
		unsigned int retry ; // 503, Retry-After
	} ;
	struct HeaderInfo
	{
		std::string m_line ; // "Foo: bar; one,two three=x"
		std::string m_lhs ; // "Foo"
		std::string m_rhs ; // "bar; one,two three=x"
		std::string m_value ; // "bar"
		G::StringArray m_tokens ; // "one", "two", "three=x"
		explicit HeaderInfo( const std::string & line_ ) ;
		bool match( const std::string & key ) const ; // m_lhs, case-insensitive
		int value( int default_ ) const ; // m_value as int
	} ;
	struct PartInfo
	{
		PartInfo() : start(0U) , headersize(0U) , bodysize(0U) {}
		size_t start ; // dash-dash-boundary position in main data buffer
		size_t headersize ; // header size, including dash-dash-boundary and blank line
		size_t bodysize ; // payload size
		std::string type ; // content-type
	} ;
	typedef std::vector<HeaderInfo> List ;
	static List::const_iterator hfind( const List & , const std::string & ) ;
	static List::const_iterator hfind( const List & , size_t ) ;
	static List headerInfo( const std::string & ) ;
	static ResponseStatusInfo responseStatusInfo( const std::string & , const std::string & ) ;
	PartInfo partInfo() const ;
	std::string headerMultipartBoundary() const ;
	std::string headerAttribute( List::const_iterator , const std::string & , const std::string & ) const ;

private:
	std::string m_data ;
	std::string m_http ;
	std::string::size_type m_sep ;
	List m_hlist ;
	ResponseStatusInfo m_response_status ;
} ;

#endif

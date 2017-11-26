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
/// \file gurl.h
///

#ifndef G_URL_H
#define G_URL_H

#include "gdef.h"
#include "gstr.h"
#include <map>
#include <string>

namespace G
{
	class Url ;
}

/// \class G::Url
/// A simple parser for URLs.
/// Eg:
/// \code
/// "protocol://user:pwd@host:port/path1/path2.ext?key1=/value%31&key2#anchor"
/// \endcode
/// \see rfc3986
/// 
class G::Url
{
public:
	typedef std::multimap<std::string,std::string> Map ;

	Url() ;
		///< Default constructor for a url with a path of "/".

	explicit Url( const std::string & ) ;
		///< Constructor.

	std::string protocol() const ;
		///< Returns the protocol part eg. "http".

	std::string address() const ;
		///< Returns the address part, which might include the port,
		///< and which might use ipv6 square brackets.

	std::string address( const std::string & default_port ) const ;
		///< Returns the transport address formed from host() and 
		///< port(default_port). This never has ipv6 square brackets.
		///< Precondition: !default_port.empty()

	std::string host() const ;
		///< Returns the hostname or network address part. Any square 
		///< brackets used to delimit an ipv6 network address are 
		///< not returned.

	std::string port( const std::string & default_ = std::string() ) const ;
		///< Returns the port or service-name, or the specified default 
		///< if none. It is often appropriate to pass protocol() as the 
		///< default.

	std::string path() const ;
		///< Returns the path part, including the leading slash.

	std::string request() const ;
		///< Returns the path and parameters, suitable for a GET request.

	std::string parameters() const ;
		///< Returns the parameters string.

	Map pmap() const ;
		///< Returns the decode()d parameters as a multimap.

	bool has( const std::string & key ) const ;
		///< Returns true if the named parameter is present.

	std::string parameter( std::string key , std::string default_ = std::string() ) const ;
		///< Returns the decode()d value of the named parameter, 
		///< or a default value.

	std::string authorisation() const ;
		///< Returns the "user:pwd" part.

	std::string anchor() const ;
		///< Returns the "#anchor" part.

	std::string str() const ;
		///< Returns the string representation. Returns "/" if default constructed.

	std::string summary() const ;
		///< Returns a summary of the url for logging purposes, specifically
		///< excluding username/password but also excluding url parameters
		///< in case they also contains authentication secrets or tokens.

	static std::string decode( const std::string & ) ;
		///< Does url-decoding (rfc3986 2.1).

	static std::string encode( const std::string & , bool plus_for_space = true ) ;
		///< Does url-encoding.

	static std::string join( const std::string & protocol , const std::string & authorisation ,
		const std::string & address , const std::string & path , const std::string & params ,
		const std::string & anchor = std::string() ) ;
			///< Returns a concatenation of the given url parts, with
			///< the correct separators inserted.

private:
	void init( std::string ) ;
	static size_t colonpos( const std::string & ) ;
	static bool ipv6( const std::string & ) ;

private:
	std::string m_protocol ;
	std::string m_authorisation ;
	std::string m_address ;
	std::string m_host ;
	std::string m_port ;
	std::string m_path ;
	std::string m_params ;
	std::string m_anchor ;
	Map m_params_map ;
} ;

#endif

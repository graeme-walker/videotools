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
/// \file gvsdp.h
///

#ifndef GV_SDP__H
#define GV_SDP__H

#include "gdef.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <map>

namespace Gv
{
	class Sdp ;
}

/// \class Gv::Sdp
/// A parser for a Session Description Protocol block, with a MIME type of 
/// "application/sdp". This is used by RTSP, but is also relevant to other 
/// session setup protocols such as SIP.
/// 
/// The SDP block contains information in one "session" section, one or more 
/// "timing" sections, and zero or more "media" sections. Each section is a 
/// list of key-value pairs, with non-unique keys, and where the key is a 
/// single letter. The "a" values are "attributes", having an attribute name 
/// and an optional attribute value with a colon separator.
/// 
/// \see RFC 4566
/// 
class Gv::Sdp
{
public:
	G_EXCEPTION( Error , "session description error" ) ;
	typedef std::multimap<std::string,std::string> Map ;

	explicit Sdp( const std::vector<std::string> & sdp_lines ) ;
		///< Constructor taking a list of text lines.

	std::string originator() const ;
		///< Returns the session originator ("o=").

	std::string name() const ;
		///< Returns the session name ("s=").

	Map attributes() const ;
		///< Returns the session attributes as a multimap ("a=key:value").

	std::string attributes( const std::string & sep ) const ;
		///< Returns the session attributes as a sep-separated string.

	size_t timeCount() const ;
		///< Returns the number of timeValue()s.

	time_t timeValue( size_t index = 0U ) ;
		///< Returns the index-th time value.

	size_t mediaCount() const ;
		///< Returns the number of mediaType()s.

	std::string mediaType( size_t index = 0U ) const ;
		///< Returns the type of the index-th media ("m=").

	std::string mediaTitle( size_t index = 0U ) const ;
		///< Returns the title of the index-th media ("i=").

	std::string mediaConnection( size_t index = 0U ) const ;
		///< Returns the connection of the index-th media ("c=").

	Map mediaAttributes( size_t index = 0U ) const ;
		///< Returns the attributes of the index-th media as a multimap ("a=").

	std::string mediaAttributes( size_t index , const std::string & sep ) const ;
		///< Returns the attributes of the index-th media as a string.

	std::string mediaAttribute( size_t index , const std::string & key ) const ;
		///< Returns the specified attribute of the index-th media ("a=key").
		///< Throws if no such key.

	std::string mediaAttribute( size_t index , const std::string & key , const std::string & default_ ) const ;
		///< Returns the specified attribute of the index-th media ("a=key").
		///< Returns the default if no such key.

	std::string fmtp() const ;
		///< A convenience method that returns the value of the "fmtp" attribute 
		///< of the first video "RTP/AVP" medium. Returns the empty string on error.

private:
	std::string value( const std::string & ) const ;
	static std::string value( const Map & , const std::string & , const std::string & ) ;
	static std::string value( const Map & , const std::string & ) ;
	static std::string value( const Map & , const Map & , const std::string & ) ;
	static Map::value_type pair( const std::string & line ) ;
	static Map attributes( const Map & ) ;
	static std::string str( const Map & , const std::string & sep ) ;

private:
	Map m_session ;
	std::vector<Map> m_time ;
	std::vector<Map> m_media ;
} ;

#endif

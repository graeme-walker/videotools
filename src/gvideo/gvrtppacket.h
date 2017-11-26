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
/// \file gvrtppacket.h
///

#ifndef GV_RTPPACKET__H
#define GV_RTPPACKET__H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace Gv
{
	class RtpPacket ;
}

/// \class Gv::RtpPacket
/// An RTP packet parser, as per RFC 3550 (section 5). An RTP packet contains a 
/// header, an optional list of sources (eg. separate cameras with their own 
/// sequence numbers), and a payload.
/// 
/// Payload formats are defined wrt. a profile sepecification, eg. RFC 3551 for 
/// the "RTP/AVP" profile. The profile defines mappings from a payload type
/// to a payload encoding.
/// 
/// See also RFC 2435 for the JPEG profile.
/// 
class Gv::RtpPacket
{
public:
	G_EXCEPTION( Error , "rtp packet error" ) ;

	RtpPacket( const char * p , size_t n ) ;
		///< Constructor. Beware that the data is not copied so it must
		///< remain valid for payload methods (begin(), end() etc).
		///< Precondition: n >= smallest()

	static size_t smallest() ; 
		///< Returns the smallest valid packet size.

	bool valid() const ;
		///< Returns true if a valid packet.

	std::string reason() const ;
		///< Returns the in-valid() reason.

	std::string str() const ;
		///< Returns a one-line summary of header fields.

	std::string hexdump() const ;
		///< Returns a complete hex dump.

	///< header...

	bool marker() const ;
		///< Returns the marker bit. The interpretation is
		///< defined by the profile.

	unsigned int seq() const ;
		///< Returns the sequence number.

	unsigned long timestamp() const ;
		///< Returns the timestamp.

	unsigned long src() const ;
		///< Returns the source id.

	///< payload...

	unsigned int type() const ;
		///< Returns the payload type.

	bool typeJpeg() const ;
		///< Returns true if the payload type is 26. 

	bool typeDynamic() const ;
		///< Returns true if the payload type is in the dynamic range,
		///< 96 to 127.

	const char * begin() const ;
		///< Returns the payload begin iterator.

	const char * end() const ;
		///< Returns the payload end iterator.

	size_t offset() const ;
		///< Returns the payload offset.

	size_t size() const ;
		///< Returns the payload size.

	const unsigned char * ubegin() const ;
		///< Returns the payload begin iterator.

	const unsigned char * uend() const ;
		///< Returns the payload end iterator.

private:
	RtpPacket( const RtpPacket & ) ;
	void operator=( const RtpPacket & ) ;
	static unsigned int make_word( const unsigned char * p ) ;
	static unsigned long make_dword( const unsigned char * p ) ;

private:
	const unsigned char * m_p ;
	size_t m_n ;
	unsigned int m_version ;
	bool m_padding ;
	bool m_extension ;
	unsigned int m_cc ;
	bool m_marker ;
	unsigned int m_pt ;
	unsigned int m_seq ;
	unsigned long m_timestamp ;
	unsigned long m_src_id ;
	unsigned int m_ehl ;
	unsigned int m_padsize ;
	size_t m_header_size ;
	size_t m_payload_size ;
	std::string m_reason ;
} ;

#endif

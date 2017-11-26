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
/// \file gvrtpjpegpacket.h
///

#ifndef GV_RTPJPEGPACKET__H
#define GV_RTPJPEGPACKET__H

#include "gdef.h"
#include <string>
#include <vector>

namespace Gv
{
	class RtpJpegPacket ;
}

/// \class Gv::RtpJpegPacket
/// An RTP payload parser for the jpeg payload type.
/// \see RFC 2435
/// 
class Gv::RtpJpegPacket
{
public:
	struct Payload /// A descriptor for the payload within an RTP JPEG packet.
	{ 
		unsigned int offset ;
		size_t size ;
		const char * begin ; 
		const char * end ;
	} ;

	static size_t smallest() ;
		///< The smallest parsable packet.

	RtpJpegPacket( const char * begin , const char * end ) ;
		///< Constructor taking in an RTP payload with Payload Type of
		///< JPEG/90000. The data should start with the RTP JPEG header defined
		///< in RFC-2435 section 3.1 and end with the JPEG entropy-coded 
		///< scan data (see section 3.1.9).
		///< Precondition: (end-begin) >= smallest().

	bool valid() const ;
		///< Returns true if a valid packet.

	std::string reason() const ;
		///< Returns the reason if in-valid().

	std::string str() const ;
		///< Returns a summary of the packet header for debugging purposes.

	///< header...

	unsigned int ts() const ; 
		///< Returns the "type-specific" value.

	unsigned long fo() const ;
		///< Returns the fragment offset.

	unsigned int type() const ;
		///< Returns the type.

	bool type_is_dynamic() const ;
		///< Returns true if type() indicates that the type is defined
		///< out-of-band by the session setup process.

	bool type_has_restart_markers() const ;
		///< Returns true if type() indicates the presence of a restart marker header.

	unsigned int type_base() const ;
		///< Returns type() with the special bit reset.

	unsigned int q() const ;
		///< Returns the Q value. Values of 128 or more indicate that
		///< the packet holds in-band quantisation tables.

	bool q_is_special() const ;
		///< Returns true if q() indicates in-band quantisation tables.

	unsigned int dx() const ; 
		///< Returns the image width in pixels, not MCUs, ie. including the x8.

	unsigned int dy() const ;
		///< Returns the image height in pixels, not MCUs, ie. including the x8.

	unsigned int ri() const ; 
		///< Returns the restart interval (JFIF DRI).

	unsigned int rc() const ; 
		///< Returns the restart count.

	Payload payload() const ;
		///< Returns the JPEG entropy-coded image scan data, using pointers into 
		///< the buffer that was passed in to the constructor. 
		///< 
		///< Use generateHeader() to create a fully-fledged JFIF structure that can be 
		///< passed to Gr::JpegReader.

	typedef std::insert_iterator<std::vector<char> > iterator_t ;

	static iterator_t generateHeader( iterator_t out , const RtpJpegPacket & , int fudge = 0 ) ;
		///< Generates the start of a JFIF buffer; the rest of the JFIF buffer is a 
		///< simple copy of all the payloads that make up the frame. The JFIF output 
		///< is emited via a vector back-insert iterator and the iterator's final 
		///< value is retured.

	unsigned int payloadOffset() const ;
		///< Returns payload().offset.

	size_t payloadSize() const ;
		///< Returns payload().size.

	const char * payloadBegin() const ;
		///< Returns payload().begin.

	const char * payloadEnd() const ;
		///< Returns payload().end.

private:
	RtpJpegPacket( const RtpJpegPacket & ) ;
	void operator=( const RtpJpegPacket & ) ;
	static unsigned int make_word( const char * p ) ;
	static unsigned long make_dword( unsigned long , unsigned long , unsigned long , unsigned long ) ;

private:
	std::string m_reason ;
	const char * m_p ;
	size_t m_n ;
} ;

#endif

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
/// \file gvrtpavcpacket.h
///

#ifndef GV_RTPAVCPACKET__H
#define GV_RTPAVCPACKET__H

#include "gdef.h"
#include "gassert.h"
#include "gvrtppacket.h"
#include <string>
#include <deque>

namespace Gv
{
	class RtpAvcPacket ;
	class RtpAvcPacketStream ;
} ;

/// \class Gv::RtpAvcPacket
/// An RTP payload parser for the "H264" payload type. Note that the RTP payload 
/// type for H264 is not fixed; it is communicated out-of-band by eg. RTSP.
/// 
/// The RTP payload for AVC can either be exactly one AVC NALU, more than one NALU,
/// or just a NALU fragment (ie. single, aggregated or fragmented). The first byte
/// of the RTP payload is cunningly defined so that is can double-up as the
/// first byte of a NALU, ie. the NALU header byte. If the first byte is a valid 
/// NALU type (ie. 1-23 in the low bits) then the RTP payload is a single NALU 
/// and there is no structure added by the RTP-AVC layer.
/// 
/// Note that AVC NALUs are composed of a single header byte followed by byte-stuffed 
/// RBSP data (see Gr::Avc). They are often chained together using a four-byte 
/// 00-00-00-01 start code and a three-byte 00-00-01 separator.
/// 
/// \see RFC-6184
/// 
class Gv::RtpAvcPacket
{
public:

	enum Type { 
		SINGLE_NALU_1 = 1 , // Single Network Abstraction Layer Unit
		// ...
		SINGLE_NALU_23 = 23 , // Single Network Abstraction Layer Unit
		STAP_A = 24 , // Single Time Aggregation Packet without DONs
		STAP_B = 25 , // Single Time Aggregation Packet with DONs
		MTAP16 = 26 , // Multi Time Aggregation Packet
		MTAP24 = 27 , // Multi Time Aggregation Packet
		FU_A = 28 , // Fragmentation Unit without DONs
		FU_B = 29 , // Fragmentation Unit with DONs
	} ;

	static size_t smallest() ;
		///< The smallest parsable packet.

	RtpAvcPacket( const unsigned char * begin , const unsigned char * end ) ;
		///< Constructor taking in an RTP payload with Payload Type of
		///< H264/90000. The data should start with the RTP H264 header defined
		///< in RFC-6184 section 5.2.
		///< Precondition: (end-begin) >= smallest().

	explicit RtpAvcPacket( const std::string & data ) ;
		///< Constructor overload taking a string packet.

	const char * reason() const ;
		///< Returns the in-valid() reason, or nullptr.

	bool valid() const ;
		///< Returns true if a valid packet.

	std::string str( bool more = false ) const ;
		///< Returns a summary of the packet header for debugging purposes.

	unsigned int type() const ;
		///< Returns the RTP-AVC packet type, matching the Type enum.
		///< This is also the NALU type iff the value is in the
		///< range 1..23 (ie. if type_is_single()).

	unsigned int nri() const ;
		///< Returns the NALU nal_ref_idc value, in the range 0..3.
		///< This indicates the discardability of the packet; zero
		///< indicates that the packet is discardable, and 3 that
		///< it is not.

	bool type_is_fu() const ;
		///< Returns true if type() is FU_A or FU_B.

	bool type_is_single() const ;
		///< Returns true if type() is SINGLE_NALU_x.

	bool type_is_sequence_parameter_set() const ;
		///< Returns true if type() is SINGLE_NALU_7.

	bool type_is_picture_parameter_set() const ;
		///< Returns true if type() is SINGLE_NALU_8.

	bool fu_start() const ;
		///< Returns true for the first FU packet in the fragmented NALU.
		///< Precondition: fu()

	bool fu_end() const ;
		///< Returns true for the last FU packet in the fragmented NALU.
		///< Precondition: fu()

	unsigned int fu_type() const ;
		///< Returns the type of the fragmented NALU.
		///< Precondition: fu()

	size_t payloadSize() const ;
		///< Returns the RTP-AVC payload size.

	char payloadFirst() const ;
		///< Returns the first RTP-AVC payload byte.
		///< 
		///< This byte is split across two bytes in the raw data of an 
		///< initial fragmentated packet, but to copy the raw data just 
		///< to fix up the first byte would be too expensive.

	const char * payloadBegin() const ;
		///< Returns the RTP-AVC payload pointer, but the first byte
		///< may be wrong so use payloadFirst() to overwrite it after
		///< copying.
		///< 
		///< Note that payloadBegin()/payloadEnd() data will only be 
		///< a NALU if type_is_single(), and RBSP byte stuffing is
		///< never used.

	const char * payloadEnd() const ;
		///< Returns the RTP-AVC payload end pointer.

private:
	RtpAvcPacket( const RtpAvcPacket & ) ;
	void operator=( const RtpAvcPacket & ) ;
	static unsigned int make_word( const unsigned char * p ) ;
	static unsigned long make_dword( unsigned long , unsigned long , unsigned long , unsigned long ) ;
	unsigned int payloadOffset() const ;

private:
	const unsigned char * m_p ;
	size_t m_n ;
} ;

/// \class Gv::RtpAvcPacketStream
/// A class that accumulates RTP-AVC packets and serves up AVC NALUs.
/// Note that there is no one-to-one correspondence between RTP-AVC
/// packets and NALUs; NALUs can be fragmented over multiple
/// packets, and multiple NULUs can be aggregated into one packet.
/// 
class Gv::RtpAvcPacketStream
{
public:
	RtpAvcPacketStream() ;
		///< Default constructor.

	bool add( const RtpPacket & ) ;
		///< Adds a packet.

	bool add( const RtpPacket & , const RtpAvcPacket & ) ;
		///< Adds a packet.

	bool more() const ;
		///< Returns true if NALUs are available.

	std::string nalu() ;
		///< Extracts a NALU.

private:
	RtpAvcPacketStream( const RtpAvcPacketStream & ) ;
	void operator=( const RtpAvcPacketStream & ) ;
	bool contiguous() const ;
	void commit() ;
	void clear() ;

private:
	unsigned long m_timestamp ;
	std::vector<unsigned int> m_seq_list ;
	std::string m_buffer ;
	std::deque<std::string> m_list ;
} ;

#endif

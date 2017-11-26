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
/// \file gvrtppacketstream.h
///

#ifndef GV_RTP_PACKET_STREAM__H
#define GV_RTP_PACKET_STREAM__H

#include "gdef.h"
#include "gassert.h"
#include "gvrtppacket.h"
#include "gvrtpavcpacket.h"
#include "gvrtpjpegpacket.h"
#include <vector>
#include <deque>

namespace Gv
{
	class RtpPacketStream ;
} ;

/// \class Gv::RtpPacketStream
/// A class that accumulates RTP-AVC or RTP-JPEG packets and serves 
/// up AVC NALUs or JPEG JFIFs.
/// 
/// Note that there is no one-to-one correspondence between RTP-AVC
/// packets and NALUs; NALUs can be fragmented over multiple
/// packets, and multiple NULUs can be aggregated into one packet.
/// JPEG images can also be fragmented over multiple packets.
/// 
class Gv::RtpPacketStream
{
public:
	explicit RtpPacketStream( int jpeg_fudge_factor ) ;
		///< Constructor.

	bool add( const RtpPacket & , const RtpAvcPacket & ) ;
		///< Adds a AVC packet.

	bool add( const RtpPacket & , const RtpJpegPacket & ) ;
		///< Adds a JPEG packet.

	bool more() const ;
		///< Returns true if NALUs or JFIFs are available.

	std::vector<char> get() ;
		///< Extracts a NALU or JFIF.

private:
	RtpPacketStream( const RtpPacketStream & ) ;
	void operator=( const RtpPacketStream & ) ;
	bool contiguous() const ;
	void commit( const RtpAvcPacket & ) ;
	void commit( const RtpJpegPacket & ) ;
	void clear() ;

private:
	int m_jpeg_fudge_factor ;
	unsigned long m_timestamp ;
	std::vector<unsigned int> m_seq_list ;
	std::vector<char> m_buffer ;
	std::deque<std::vector<char> > m_list ;
} ;

#endif

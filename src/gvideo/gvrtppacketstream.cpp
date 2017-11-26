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
// gvrtppacketstream.cpp
//

#include "gdef.h"
#include "gvrtppacketstream.h"
#include "gravc.h"
#include "ghexdump.h"
#include "gassert.h"

// TODO denial-of-service limits
// TODO elide jpeg and avc code

Gv::RtpPacketStream::RtpPacketStream( int jpeg_fudge_factor ) :
	m_jpeg_fudge_factor(jpeg_fudge_factor) ,
	m_timestamp(0)
{
}

bool Gv::RtpPacketStream::add( const RtpPacket & rtp_packet , const RtpJpegPacket & jpeg_packet )
{
	const size_t fragment_offset = jpeg_packet.fo() ;
	const bool last_fragment = rtp_packet.marker() ;
	const bool first_fragment = fragment_offset == 0U ;
	const bool is_single = first_fragment && last_fragment ;

	bool used = false ;
	if( is_single )
	{
		clear() ;
		m_buffer.insert( m_buffer.end() , jpeg_packet.payloadBegin() , jpeg_packet.payloadEnd() ) ;
		m_seq_list.push_back( rtp_packet.seq() ) ;
		commit( jpeg_packet ) ;
		used = true ;
	}
	else
	{
		if( first_fragment )
		{
			clear() ;
			m_timestamp = rtp_packet.timestamp() ;
			m_buffer.insert( m_buffer.end() , jpeg_packet.payloadBegin() , jpeg_packet.payloadEnd() ) ;
			m_seq_list.push_back( rtp_packet.seq() ) ;
			used = true ;
		}
		else if( rtp_packet.timestamp() != m_timestamp || fragment_offset != m_buffer.size() )
		{
			G_WARNING( "Gv::RtpServer::processRtpJpegPacket: ignoring out-of-sequence packet: " << rtp_packet.str() ) ;
			clear() ;
			used = false ;
		}
		else
		{
			m_buffer.insert( m_buffer.end() , jpeg_packet.payloadBegin() , jpeg_packet.payloadEnd() ) ;
			m_seq_list.push_back( rtp_packet.seq() ) ;
			if( last_fragment )
			{
				if( contiguous() )
				{
					commit( jpeg_packet ) ;
					clear() ;
					used = true ;
				}
				else
				{
					G_WARNING( "Gv::RtpServer::processRtpJpegPacket: ignoring frame with missing packets" ) ;
					clear() ;
					used = false ;
				}
			}
		}
	}
	return used ;
}

bool Gv::RtpPacketStream::add( const RtpPacket & rtp_packet , const RtpAvcPacket & avc_packet )
{
	bool used = false ;
	if( avc_packet.type_is_single() )
	{
		clear() ;
		m_buffer.insert( m_buffer.end() , avc_packet.payloadBegin() , avc_packet.payloadEnd() ) ;
		m_buffer.at(0U) = avc_packet.payloadFirst() ;
		commit( avc_packet ) ;
		used = true ;
	}
	else if( avc_packet.type_is_fu() )
	{
		if( avc_packet.fu_start() )
		{
			clear() ;
			m_timestamp = rtp_packet.timestamp() ;
			m_buffer.insert( m_buffer.end() , avc_packet.payloadBegin() , avc_packet.payloadEnd() ) ;
			m_buffer.at(0U) = avc_packet.payloadFirst() ;
			m_seq_list.push_back( rtp_packet.seq() ) ;
			used = true ;
		}
		else if( rtp_packet.timestamp() != m_timestamp )
		{
            G_WARNING( "Gv::RtpServer::processRtpAvcPacket: ignoring out-of-sequence packet: " << rtp_packet.str() ) ;
            clear() ;
			used = false ;
		}
		else
		{
			size_t pos = m_buffer.size() ;
			m_buffer.insert( m_buffer.end() , avc_packet.payloadBegin() , avc_packet.payloadEnd() ) ;
			m_buffer.at(pos) = avc_packet.payloadFirst() ;
			m_seq_list.push_back( rtp_packet.seq() ) ;
			if( avc_packet.fu_end() )
			{
				if( contiguous() )
				{
					commit( avc_packet ) ;
					clear() ;
					used = true ;
				}
				else
				{
					G_WARNING( "Gv::RtpServer::processRtpAvcPacket: ignoring frame with missing packets" ) ;
					clear() ;
					used = false ;
				}
			}
		}
	}
	else
	{
		G_WARNING( "Gv::RtpServer::processRtpAvcPacket: ignoring rtp-avc aggregation packet: not implemented" ) ;
		used = false ;
	}
	return used ;
}

bool Gv::RtpPacketStream::contiguous() const
{
    unsigned int old = 0U ;
    bool first = true ;
    bool ok = true ;
    for( std::vector<unsigned int>::const_iterator p = m_seq_list.begin() ; ok && p != m_seq_list.end() ; ++p , first = false )
    {
        ok = first || *p == (old+1U) || ( *p == 0U && old != 0U ) ;
        old = *p ;
    }
    return ok ;
}

void Gv::RtpPacketStream::commit( const RtpJpegPacket & jpeg_packet )
{
	// add JFIF header and trailer - arbitrarily use the last packet for some of the header values
	RtpJpegPacket::generateHeader( std::inserter(m_buffer,m_buffer.begin()) , jpeg_packet , m_jpeg_fudge_factor ) ;
	m_buffer.push_back( 0xff ) ; 
	m_buffer.push_back( 0xd9 ) ; // EOI

	m_list.push_back( std::vector<char>() ) ;
	m_buffer.swap( m_list.back() ) ;
	G_ASSERT( m_buffer.empty() ) ;
}

void Gv::RtpPacketStream::commit( const RtpAvcPacket & )
{
	// add NALU four-byte 00-00-00-01 start-code
	const std::string & start_code = Gr::Avc::Rbsp::_0001() ;
	m_buffer.insert( m_buffer.begin() , start_code.begin() , start_code.end() ) ;

	m_list.push_back( std::vector<char>() ) ;
	m_buffer.swap( m_list.back() ) ;
	G_ASSERT( m_buffer.empty() ) ;
}

void Gv::RtpPacketStream::clear()
{
	m_buffer.clear() ;
	m_seq_list.clear() ;
	m_timestamp = 0UL ;
}

bool Gv::RtpPacketStream::more() const
{
	return !m_list.empty() ;
}

std::vector<char> Gv::RtpPacketStream::get()
{
	G_ASSERT( !m_list.empty() ) ;
	std::vector<char> result = m_list.front() ;
	m_list.pop_front() ;
	return result ;
}

/// \file gvrtppacketstream.cpp

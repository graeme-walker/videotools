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
// gvrtpavcpacket.cpp
//

#include "gdef.h"
#include "gvrtpavcpacket.h"
#include "gravc.h"
#include "gassert.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

size_t Gv::RtpAvcPacket::smallest()
{
	return 2U ; // one NALU header byte and one of payload or FU header
}

Gv::RtpAvcPacket::RtpAvcPacket( const unsigned char * begin , const unsigned char * end ) :
	m_p(begin) ,
	m_n(end-begin)
{
	G_ASSERT( m_n >= smallest() ) ;
}

Gv::RtpAvcPacket::RtpAvcPacket( const std::string & data ) :
	m_p(reinterpret_cast<const unsigned char*>(data.data())) ,
	m_n(data.size())
{
	G_ASSERT( m_n >= smallest() ) ;
}

const char * Gv::RtpAvcPacket::reason() const
{
	if( (*m_p&0x80) != 0U ) return "top bit of nalu header byte is set" ;
	if( type() == 0 ) return "nalu header byte is zero" ;
	if( type_is_fu() && fu_start() && fu_end() ) return "conflicting fragmentation flags" ;
	if( m_n <= payloadOffset() ) return "packet too small" ;
	return nullptr ;
}

bool Gv::RtpAvcPacket::valid() const
{
	return reason() == nullptr ;
}

std::string Gv::RtpAvcPacket::str( bool more ) const
{
	std::ostringstream ss ;
	ss << "type=" << type() ;
	if( type_is_fu() )
		ss << " fu-start=" << fu_start() << " fu-end=" << fu_end() << " fu-type=" << fu_type() ;
	if( more )
		ss
			<< std::hex << std::setfill('0') << std::setw(2)
			<< " p0=0x" << static_cast<unsigned int>(m_p[0]) << " "
			<< " p1=0x" << static_cast<unsigned int>(m_p[1]) << " "
			<< " p2=0x" << static_cast<unsigned int>(m_p[2]) << " "
			<< " pf=0x" << static_cast<unsigned int>(static_cast<unsigned char>(payloadFirst()))
			<< std::dec ;
	return ss.str() ;
}

unsigned int Gv::RtpAvcPacket::type() const
{
	return *m_p & 0x1f ;
}

unsigned int Gv::RtpAvcPacket::nri() const
{
	unsigned int n = static_cast<unsigned char>(*m_p) ;
	return ( n >> 5 ) & 3U ;
}

unsigned int Gv::RtpAvcPacket::payloadOffset() const
{
	// for simple packets the first byte is the nalu-header so there 
	// is effectively no RTP-AVC header and the offset is zero -- 
	// for fu packets there is an fu-indicator byte and an fu-header 
	// byte, and the nalu-type is in the lower bits of the fu-header 
	// (see payloadFirst()) so the offset is one -- however, the RTP-AVC 
	// payload for non-leading fragments must not have a nalu-header 
	// byte because they are not at the head of the nalu, so the offset 
	// is two

	// TODO more packet types

	if( type() == FU_A )
		return fu_start() ? 1U : 2U ;
	else if( type() == FU_B )
		return fu_start() ? 3U : 4U ;
	else if( type_is_single() )
		return 0U ;
	else
		throw std::runtime_error( "unsupported RTP-AVC packet type" ) ;
}

size_t Gv::RtpAvcPacket::payloadSize() const
{
	G_ASSERT( m_n >= payloadOffset() ) ;
	return m_n - payloadOffset() ;
}

const char * Gv::RtpAvcPacket::payloadBegin() const
{
	const char * p = reinterpret_cast<const char*>( m_p ) ;
	return p + payloadOffset() ;
}

char Gv::RtpAvcPacket::payloadFirst() const
{
	if( type_is_fu() && fu_start() )
		return ( (m_p[0]&0xe0) | (m_p[1]&0x1f) ) ; // merge bits from fu-indicator and fu-header
	else
		return m_p[payloadOffset()] ;
}

const char * Gv::RtpAvcPacket::payloadEnd() const
{
	return payloadBegin() + payloadSize() ;
}

bool Gv::RtpAvcPacket::type_is_single() const
{
	return type() >= SINGLE_NALU_1 && type() <= SINGLE_NALU_23 ;
}

bool Gv::RtpAvcPacket::type_is_fu() const
{
	return type() == FU_A || type() == FU_B ;
}

bool Gv::RtpAvcPacket::fu_start() const
{
	G_ASSERT( m_n >= 2U ) ;
	return !!( m_p[1] & 0x80 ) ;
}

bool Gv::RtpAvcPacket::fu_end() const
{
	G_ASSERT( m_n >= 2U ) ;
	return !!( m_p[1] & 0x40 ) ;
}

unsigned int Gv::RtpAvcPacket::fu_type() const
{
	G_ASSERT( m_n >= 2U ) ;
	return m_p[1] & 0x1f ; // ie. inner nalu type
}

bool Gv::RtpAvcPacket::type_is_sequence_parameter_set() const
{
	return type() == 7 ; // SINGLE_NALU_7 = SPS
}

bool Gv::RtpAvcPacket::type_is_picture_parameter_set() const
{
	return type() == 8 ; // SINGLE_NALU_8 = PPS
}

/// \file gvrtpavcpacket.cpp

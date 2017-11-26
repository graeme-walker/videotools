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
// grtppacket.cpp
//

#include "gdef.h"
#include "gvrtppacket.h"
#include "gstr.h"
#include "gassert.h"
#include "ghexdump.h"
#include "glog.h"
#include <sstream>
#include <algorithm>

Gv::RtpPacket::RtpPacket( const char * p , size_t n ) :
	m_p(reinterpret_cast<const unsigned char*>(p)) ,
	m_n(n)
{
	G_ASSERT( n >= smallest() ) ;

	m_version = ( m_p[0] & 0xc0 ) >> 6 ;
	m_padding = !!( m_p[0] & 0x20 ) ;
	m_extension = !!( m_p[0] & 0x10 ) ;
	m_cc = m_p[0] & 0x0f ;
	m_marker = !!( m_p[1] & 0x80 ) ;
	m_pt = m_p[1] & 0x7f ;
	m_seq = make_word( &m_p[2] ) ;
	m_timestamp = make_dword( &m_p[4] ) ;
	m_src_id = make_dword( &m_p[8] ) ;

	m_header_size = 12U + 4U*m_cc ;
	m_ehl = (m_extension && n>=(m_header_size+4U)) ? make_word(&m_p[m_header_size+2U]) : 0U ;
	m_header_size += (m_ehl*4U) ;

	m_padsize = m_padding ? m_p[n-1U] : 0U ;
	m_payload_size = n - m_header_size - m_padsize ; // underflow checked later

	if( m_version != 2U ) m_reason = "invalid version" ;
	if( n < (m_header_size+m_padsize) ) m_reason = "invalid sizes" ;
}

size_t Gv::RtpPacket::smallest()
{
	return 12U ;
}

bool Gv::RtpPacket::valid() const
{
	return m_reason.empty() ;
}

std::string Gv::RtpPacket::reason() const
{
	return m_reason ;
}

std::string Gv::RtpPacket::str() const
{
	std::ostringstream ss ;
	ss
		<< "n=" << m_n
		<< " v=" << m_version << " p=" << m_padding << " e=" << m_extension 
		<< " cc=" << m_cc << " m=" << m_marker << " pt=" << m_pt 
		<< " ehl=" << m_ehl << " hs=" << m_header_size 
		<< " pds=" << m_padsize << " pls=" << m_payload_size
		<< " ts=" << m_timestamp
		<< " seq=" << m_seq ;
	return ss.str() ;
}

std::string Gv::RtpPacket::hexdump() const
{
	std::ostringstream ss ;
	ss << G::hexdump<8>(m_p,m_p+m_n) ;
	return ss.str() ;
}

bool Gv::RtpPacket::marker() const
{
	return m_marker ;
}

unsigned int Gv::RtpPacket::seq() const
{
	return m_seq ;
}

unsigned long Gv::RtpPacket::timestamp() const
{
	return m_timestamp ;
}

unsigned long Gv::RtpPacket::src() const
{
	return m_src_id ;
}

unsigned int Gv::RtpPacket::make_word( const unsigned char * p )
{
	return (static_cast<unsigned int>(p[0]) << 8) | p[1] ;
}

unsigned long Gv::RtpPacket::make_dword( const unsigned char * p )
{
	return 
		(static_cast<unsigned long>(p[0]) << 24) |
		(static_cast<unsigned long>(p[1]) << 16) |
		(static_cast<unsigned long>(p[2]) << 8) |
		(static_cast<unsigned long>(p[3]) << 0) ;
}

unsigned int Gv::RtpPacket::type() const
{
	return m_pt ;
}

bool Gv::RtpPacket::typeJpeg() const
{
	// See RFC 3551 table 5
	return m_pt == 26U ;
}

bool Gv::RtpPacket::typeDynamic() const
{
	// See RFC 3551 table 5
	return m_pt >= 96U && m_pt <= 127U ;
}

const unsigned char * Gv::RtpPacket::ubegin() const
{
	return m_p + m_header_size ;
}

const unsigned char * Gv::RtpPacket::uend() const
{
	return m_p + m_header_size + m_payload_size ;
}

const char * Gv::RtpPacket::begin() const
{
	return reinterpret_cast<const char*>( ubegin() ) ;
}

const char * Gv::RtpPacket::end() const
{
	return reinterpret_cast<const char*>( uend() ) ;
}

size_t Gv::RtpPacket::offset() const
{
	return m_header_size ;
}

size_t Gv::RtpPacket::size() const
{
	return m_payload_size ;
}

/// \file gvrtppacket.cpp

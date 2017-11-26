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
// glocalsocket.cpp
// 

#include "gdef.h"
#include "glocalsocket.h"
#include "gprocess.h"
#include "gstr.h"
#include "gassert.h"
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>

G::LocalSocketAddress::LocalSocketAddress( SignalSafe , const char * path )
{
	m_u.specific.sun_family = AF_LOCAL ; // ie. AF_UNIX
	::memset( m_u.specific.sun_path , 0 , sizeof(m_u.specific.sun_path) ) ;
	std::strncpy( m_u.specific.sun_path , path , sizeof(m_u.specific.sun_path)-1U ) ;
}

G::LocalSocketAddress::LocalSocketAddress( const std::string & path )
{
	if( path.length() >= sizeof(m_u.specific.sun_path) )
		throw std::runtime_error( "socket path too long" ) ;

	m_u.specific.sun_family = AF_LOCAL ; // ie. AF_UNIX
	::memset( m_u.specific.sun_path , 0 , sizeof(m_u.specific.sun_path) ) ;
	std::strncpy( m_u.specific.sun_path , path.c_str() , sizeof(m_u.specific.sun_path)-1U ) ;
}

struct sockaddr * G::LocalSocketAddress::p()
{
	return &m_u.general ;
}

const struct sockaddr * G::LocalSocketAddress::p() const
{
	return &m_u.general ;
}

size_t G::LocalSocketAddress::size() const
{
	return sizeof(m_u.specific) ;
}

// ==

G::LocalSocket::LocalSocket( bool datagram ) :
	m_connected(false)
{
	m_fd = ::socket( AF_LOCAL , datagram ? SOCK_DGRAM : SOCK_STREAM , 0 ) ;
	int e = G::Process::errno_() ;
	if( m_fd < 0 )
		throw Error( "failed to create local socket" , G::Process::strerror(e) ) ;
}

bool G::LocalSocket::connect( const std::string & path , bool no_throw )
{
	Address address( path.c_str() ) ;
	int rc = ::connect( m_fd , address.p() , address.size() ) ;
	int e = G::Process::errno_() ;
	if( rc != 0 && !no_throw )
		throw Error( "failed to connect local socket" , path , G::Process::strerror(e) ) ;
	m_connected = rc == 0 ;
	return rc == 0 ;
}

void G::LocalSocket::bind( const std::string & path )
{
	G_ASSERT( !path.empty() ) ;
	Address address( path.c_str() ) ;
	int rc = ::bind( m_fd , address.p() , address.size() ) ;
	if( rc != 0 )
	{
		int e = errno ;
		throw Error( "failed to bind path to local socket" , G::Process::strerror(e) , path ) ;
	}
}

G::LocalSocket::~LocalSocket()
{
	::close( m_fd ) ;
}

bool G::LocalSocket::connected() const
{
	return m_connected ;
}

int G::LocalSocket::fd() const
{
	return m_fd ;
}

void G::LocalSocket::nonblock()
{
	int flags = ::fcntl( m_fd , F_GETFL ) ;
	if( flags != -1 )
		::fcntl( m_fd , F_SETFL , flags | O_NONBLOCK ) ;
}

/// \file glocalsocket.cpp

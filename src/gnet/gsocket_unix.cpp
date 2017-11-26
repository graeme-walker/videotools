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
// gsocket_unix.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "gprocess.h"
#include "gdebug.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"
#include <cstring>
#include <sstream>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool GNet::Socket::setNonBlock()
{
	int mode = ::fcntl( m_socket.fd() , F_GETFL ) ;
	if( mode < 0 )
		return false ;

	int rc = ::fcntl( m_socket.fd() , F_SETFL , mode | O_NONBLOCK ) ;
	return rc >= 0 ;
}

int GNet::Socket::reason()
{
	int e = errno ;
	return e ;
}

void GNet::Socket::doClose()
{
	::close( m_socket.fd() ) ;
	m_socket = Descriptor::invalid() ;
}

bool GNet::Socket::error( int rc )
{
	return rc < 0 ;
}

bool GNet::Socket::sizeError( ssize_t size )
{
	return size < 0 ;
}

bool GNet::Socket::eWouldBlock()
{
	return m_reason == EWOULDBLOCK || m_reason == EAGAIN || m_reason == EINTR ;
}

bool GNet::Socket::eInProgress()
{
	return m_reason == EINPROGRESS ;
}

bool GNet::Socket::eMsgSize()
{
	return m_reason == EMSGSIZE ;
}

void GNet::Socket::setFault()
{
	m_reason = EFAULT ;
}

bool GNet::Socket::canBindHint( const Address & address )
{
	return bind( address , NoThrow() ) ;
}

void GNet::Socket::setOptionReuse()
{
	setOption( SOL_SOCKET , "so_reuseaddr" , SO_REUSEADDR , 1 ) ; // allow bind on TIME_WAIT address -- see also SO_REUSEPORT
}

void GNet::Socket::setOptionExclusive()
{
	// no-op
}

void GNet::Socket::setOptionPureV6( bool active )
{
	#if GCONFIG_HAVE_IPV6
		if( active )
			setOption( IPPROTO_IPV6 , "ipv6_v6only" , IPV6_V6ONLY , 1 ) ;
	#else
		if( active )
			throw SocketError( "cannot set socket option for pure ipv6" ) ;
	#endif
}

bool GNet::Socket::setOptionPureV6( bool active , NoThrow )
{
	#if GCONFIG_HAVE_IPV6
		return active ? setOption( IPPROTO_IPV6 , "ipv6_v6only" , IPV6_V6ONLY , 1 , NoThrow() ) : true ;
	#else
		return !active ;
	#endif
}

bool GNet::Socket::setOptionImp( int level , int op , const void * arg , socklen_t n )
{
	int rc = ::setsockopt( m_socket.fd() , level , op , arg , n ) ; 
	return ! error(rc) ;
}

std::string GNet::Socket::reasonStringImp( int e )
{
	return G::Process::strerror( e ) ;
}

/// \file gsocket_unix.cpp

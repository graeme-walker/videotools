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
// gsocket.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gtest.h"
#include "gsleep.h"
#include "gassert.h"
#include "gsocket.h"
#include "gcleanup.h"
#include "gstr.h"
#include "gdebug.h"

GNet::Socket::Socket( int domain , int type , int protocol ) :
	m_reason(0) ,
	m_domain(domain)
{
	m_socket = Descriptor( ::socket( domain , type , protocol ) ) ;
	if( !m_socket.valid() )
	{
		saveReason() ;
		throw SocketError( "cannot create socket" , m_reason_string ) ;
	}
	prepare() ;
}

GNet::Socket::Socket( Descriptor s ) :
	m_reason(0) ,
	m_socket(s)
{
	prepare() ;
}

void GNet::Socket::prepare()
{
	G::Cleanup::init() ; // ignore SIGPIPE
	if( ! setNonBlock() )
	{
		saveReason() ;
		doClose() ;
		G_WARNING( "GNet::Socket::open: cannot make socket non-blocking (" << m_reason_string << ")" ) ;
	}
}

GNet::Socket::~Socket()
{
	try 
	{ 
		drop() ; // first
		doClose() ;
	} 
	catch(...) // dtor
	{
	}
}

void GNet::Socket::drop()
{
	dropReadHandler() ;
	dropWriteHandler() ;
	dropExceptionHandler() ;
}

void GNet::Socket::saveReason()
{
	m_reason = reason() ;
	m_reason_string = reasonStringImp( m_reason ) ;
}

void GNet::Socket::saveReason() const
{
	const_cast<GNet::Socket*>(this)->saveReason() ;
}

void GNet::Socket::bind( const Address & local_address )
{
	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << m_socket ) ;

	if( local_address.domain() != m_domain )
		throw SocketBindError( "address family does not match the socket domain" ) ;

	setOptionReuse() ; // allow us to rebind another socket's (eg. time-wait zombie's) address
	//setOptionExclusive() ; // don't allow anyone else to bind our address
	setOptionPureV6( local_address.family() == Address::Family::ipv6() ) ;

	int rc = ::bind( m_socket.fd() , local_address.address() , local_address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;
		throw SocketBindError( local_address.displayString() , m_reason_string ) ;
	}
}

bool GNet::Socket::bind( const Address & local_address , NoThrow )
{
	G_DEBUG( "Socket::bind: binding " << local_address.displayString() << " on fd " << m_socket ) ;
	if( local_address.domain() != m_domain ) return false ;

	setOptionReuse() ; // allow us to rebind another socket's (eg. time-wait zombie's) address
	//setOptionExclusive() ; // don't allow anyone else to bind our address
	setOptionPureV6( local_address.family() == Address::Family::ipv6() ) ;

	int rc = ::bind( m_socket.fd() , local_address.address() , local_address.length() ) ;
	return ! error(rc) ;
}

bool GNet::Socket::connect( const Address & address , bool *done )
{
	G_DEBUG( "GNet::Socket::connect: connecting to " << address.displayString() ) ;
	if( address.domain() != m_domain )
	{
		G_WARNING( "GNet::Socket::connect: cannot connect: address family does not match "
			"the socket domain (" << address.domain() << "," << m_domain << ")" ) ;
		return false ;
	}

	setOptionPureV6( address.family() == Address::Family::ipv6() , NoThrow() ) ; // ignore errors - may fail if already bound

	int rc = ::connect( m_socket.fd() , address.address() , address.length() ) ;
	if( error(rc) )
	{
		saveReason() ;

		if( G::Test::enabled( "slow-connect" ) )
			sleep( 1 ) ;

		if( eInProgress() )
		{
			G_DEBUG( "GNet::Socket::connect: connection in progress" ) ;
			if( done != nullptr ) *done = false ;
			return true ;
		}

		G_DEBUG( "GNet::Socket::connect: synchronous connect failure: " << m_reason ) ;
		return false;
	}

	if( done != nullptr ) *done = true ;
	return true ;
}

GNet::Socket::ssize_type GNet::Socket::write( const char * buf , size_type len )
{
	if( static_cast<ssize_type>(len) < 0 )
		G_WARNING( "GNet::Socket::write: too big" ) ; // should get EMSGSIZE from ::send()

	ssize_type nsent = ::send( m_socket.fd() , buf , len , MSG_NOSIGNAL ) ; // no SIGPIPE
	if( sizeError(nsent) ) // if -1
	{
		saveReason() ;
		G_DEBUG( "GNet::Socket::write: write error " << m_reason ) ;
		return -1 ;
	}
	else if( nsent < 0 || static_cast<size_type>(nsent) < len )
	{
		saveReason() ;
	}
	return nsent;
}

void GNet::Socket::listen( int backlog )
{
	int rc = ::listen( m_socket.fd() , backlog ) ;
	if( error(rc) )
	{
		saveReason() ;
		throw SocketError( "cannot listen on socket" , m_reason_string ) ;
	}
}

std::pair<bool,GNet::Address> GNet::Socket::getAddress( bool local ) const
{
	std::pair<bool,Address> error_pair( false , Address::defaultAddress() ) ;
	AddressStorage address_storage ;
	int rc = 
		local ? 
			::getsockname( m_socket.fd() , address_storage.p1() , address_storage.p2() ) :
			::getpeername( m_socket.fd() , address_storage.p1() , address_storage.p2() ) ;

	if( error(rc) )
	{
		saveReason() ;
		return error_pair ;
	}

	return std::pair<bool,Address>( true , Address(address_storage) ) ;
}

std::pair<bool,GNet::Address> GNet::Socket::getLocalAddress() const
{
	return getAddress( true ) ;
}

std::pair<bool,GNet::Address> GNet::Socket::getPeerAddress() const
{
	return getAddress( false ) ;
}

bool GNet::Socket::hasPeer() const
{
	return getPeerAddress().first ;
}

void GNet::Socket::addReadHandler( EventHandler & handler )
{
	G_DEBUG( "GNet::Socket::addReadHandler: fd " << m_socket ) ;
	EventLoop::instance().addRead( m_socket , handler ) ;
}

void GNet::Socket::dropReadHandler()
{
	EventLoop::instance().dropRead( m_socket ) ;
}

void GNet::Socket::addWriteHandler( EventHandler & handler )
{
	G_DEBUG( "GNet::Socket::addWriteHandler: fd " << m_socket ) ;
	EventLoop::instance().addWrite( m_socket , handler ) ;
}

void GNet::Socket::addExceptionHandler( EventHandler & handler )
{
	G_DEBUG( "GNet::Socket::addExceptionHandler: fd " << m_socket ) ;
	EventLoop::instance().addException( m_socket , handler ) ;
}

void GNet::Socket::dropWriteHandler()
{
	EventLoop::instance().dropWrite( m_socket ) ;
}

void GNet::Socket::dropExceptionHandler()
{
	EventLoop::instance().dropException( m_socket ) ;
}

std::string GNet::Socket::asString() const
{
	std::ostringstream ss ;
	ss << m_socket ;
	return ss.str() ;
}

std::string GNet::Socket::reasonString() const
{
	std::ostringstream ss ;
	ss << m_reason ;
	return ss.str() ;
}

void GNet::Socket::shutdown( bool for_writing )
{
	::shutdown( m_socket.fd() , for_writing ? 1 : 0 ) ;
}

SOCKET GNet::Socket::fd() const
{
	return m_socket.fd() ;
}

//==

GNet::StreamSocket::StreamSocket( int address_domain ) :
	Socket( address_domain , SOCK_STREAM , 0 )
{
	setOptionNoLinger() ;
	setOptionKeepAlive() ;
}

GNet::StreamSocket::StreamSocket( Descriptor s ) : 
	Socket( s )
{
}

GNet::StreamSocket::~StreamSocket()
{
}

GNet::Socket::ssize_type GNet::StreamSocket::read( char * buf , size_type len )
{
	if( len == 0 ) 
		return 0 ;

	ssize_type nread = ::recv( m_socket.fd() , buf , len , 0 ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		G_DEBUG( "GNet::StreamSocket::read: cannot read from " << m_socket.fd() ) ;
		return -1 ;
	}
	return nread ;
}

GNet::Socket::ssize_type GNet::StreamSocket::write( const char * buf , size_type len )
{
	return Socket::write( buf , len ) ;
}

GNet::AcceptPair GNet::StreamSocket::accept()
{
	AddressStorage addr ;
	Descriptor new_socket( ::accept(m_socket.fd(),addr.p1(),addr.p2()) ) ;
	if( ! new_socket.valid() )
	{
		saveReason() ;
		throw SocketError( "cannot accept on socket" , m_reason_string ) ;
	}

	if( G::Test::enabled("accept-throws") )
		throw SocketError( "testing" ) ;

	AcceptPair pair ;
	pair.second = Address( addr.p() , addr.n() ) ;
	G_DEBUG( "GNet::StreamSocket::accept: accepted from socket " << m_socket.fd() 
		<< " onto socket " << new_socket << " with address " << pair.second.displayString() ) ;
	pair.first.reset( new StreamSocket(new_socket) ) ;
	pair.first.get()->setOptionNoLinger() ;
	return pair ;
}

//==

GNet::DatagramSocket::DatagramSocket( int address_domain ) : 
	Socket( address_domain , SOCK_DGRAM , 0 )
{
}

GNet::DatagramSocket::~DatagramSocket()
{
}

void GNet::DatagramSocket::disconnect()
{
	int rc = ::connect( m_socket.fd() , 0 , 0 ) ;
	if( error(rc) )
		saveReason() ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::read( char * buf , size_type len )
{
	if( len == 0 ) return 0 ;
	sockaddr sender ;
	socklen_t sender_len = sizeof(sender) ;
	ssize_type nread = ::recvfrom( m_socket.fd() , buf , len , 0 , &sender , &sender_len ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		return -1 ;
	}

	return nread ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::readfrom( char * buf , size_type len , Address & src_address )
{
	if( len == 0 ) return 0 ;
	sockaddr sender ;
	socklen_t sender_len = sizeof(sender) ;
	ssize_type nread = ::recvfrom( m_socket.fd() , buf , len , 0 , &sender , &sender_len ) ;
	if( sizeError(nread) )
	{
		saveReason() ;
		return -1 ;
	}
	src_address = Address( &sender , sender_len ) ;
	return nread ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::writeto( const char * buf , size_type len , const Address & dst )
{
	G_DEBUG( "GNet::DatagramSocket::write: sending " << len << " bytes to " << dst.displayString() ) ;

	ssize_type nsent = ::sendto( m_socket.fd() , buf , len , 0 , dst.address() , dst.length() ) ;
	if( nsent < 0 )
	{
		saveReason() ;
		G_DEBUG( "GNet::DatagramSocket::write: write error " << m_reason ) ;
		return -1 ;
	}
	return nsent ;
}

GNet::Socket::ssize_type GNet::DatagramSocket::write( const char * buf , size_type len ) 
{ 
	return Socket::write( buf , len ) ;
}

void GNet::Socket::setOptionKeepAlive()
{
	setOption( SOL_SOCKET , "so_keepalive" , SO_KEEPALIVE , 1 ) ;
}

void GNet::Socket::setOptionNoLinger()
{
	struct linger options ;
	options.l_onoff = 0 ;
	options.l_linger = 0 ;
	bool ok = setOptionImp( SOL_SOCKET , SO_LINGER , reinterpret_cast<char*>(&options) , sizeof(options) ) ;
	if( !ok )
	{
		saveReason() ;
		throw SocketError( "cannot set so_linger" , m_reason_string ) ;
	}
}

bool GNet::Socket::setOption( int level , const char * , int op , int arg , NoThrow )
{
	const void * const vp = reinterpret_cast<const void*>(&arg) ;
	bool ok = setOptionImp( level , op , vp , sizeof(int) ) ;
	if( !ok )
		saveReason() ;
	return ok ;
}

void GNet::Socket::setOption( int level , const char * opp , int op , int arg )
{
	if( !setOption( level , opp , op , arg , NoThrow() ) )
		throw SocketError( opp , m_reason_string ) ;
}

/// \file gsocket.cpp

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
// gsimpleclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gsocket.h"
#include "gdatetime.h"
#include "gexception.h"
#include "gresolver.h"
#include "groot.h"
#include "gmonitor.h"
#include "gsimpleclient.h"
#include "gassert.h"
#include "gtest.h"
#include "gdebug.h"
#include "glog.h"
#include <cstdlib>

namespace
{
	const char * c_cannot_connect_to = "cannot connect to " ;
}

// ==

GNet::SimpleClient::SimpleClient( const Location & remote ,
	bool bind_local_address , const Address & local_address , 
	bool sync_dns , unsigned int secure_connection_timeout ) :
		m_resolver(*this) ,
		m_remote_location(remote) ,
		m_bind_local_address(bind_local_address) ,
		m_local_address(local_address) ,
		m_state(Idle) ,
		m_sync_dns(sync_dns) ,
		m_secure_connection_timeout(secure_connection_timeout) ,
		m_on_connect_timer(*this,&GNet::SimpleClient::onConnectTimer,*static_cast<GNet::EventHandler*>(this))
{
	G_DEBUG( "SimpleClient::ctor" ) ;
	if( Monitor::instance() ) Monitor::instance()->addClient( *this ) ;
}

GNet::SimpleClient::~SimpleClient()
{
	if( Monitor::instance() ) Monitor::instance()->removeClient( *this ) ;
	close() ;
}

std::string GNet::SimpleClient::logId() const
{
	std::string s = m_remote_location.displayString() ;
	if( m_socket.get() != nullptr )
		s.append( std::string() + "@" + m_socket->asString() ) ; // cf. ServerPeer::logId()
	return s ;
}

GNet::Location GNet::SimpleClient::remoteLocation() const
{
	return m_remote_location ;
}

void GNet::SimpleClient::updateLocation( const Location & update )
{
	if( m_remote_location.host() == update.host() && m_remote_location.service() == update.service() && update.resolved() )
	{
		G_DEBUG( "GNet::SimpleClient::updateLocation: reusing dns lookup for " << update.displayString() ) ;
		m_remote_location = update ;
	}
}

GNet::StreamSocket & GNet::SimpleClient::socket()
{
	if( m_socket.get() == nullptr )
		throw NotConnected() ;
	return *m_socket.get() ;
}

const GNet::StreamSocket & GNet::SimpleClient::socket() const
{
	if( m_socket.get() == nullptr )
		throw NotConnected() ;
	return *m_socket.get() ;
}

void GNet::SimpleClient::connect()
{
	try
	{
		G_DEBUG( "GNet::SimpleClient::connect: [" << m_remote_location.displayString() << "]" ) ;
		if( m_state != Idle )
			throw ConnectError( "wrong state" ) ;

		m_remote_location.resolveTrivially() ; // if host:service is already address:port
		if( m_remote_location.resolved() )
		{
			setState( Connecting ) ;
			startConnecting() ;
		}
		else if( m_sync_dns || !Resolver::async() )
		{
			std::string error = Resolver::resolve( m_remote_location ) ;
			if( !error.empty() )
				throw DnsError( error ) ;
		
			setState( Connecting ) ;
			startConnecting() ;
		}
		else
		{
			setState( Resolving ) ;
			m_resolver.start( m_remote_location ) ;
		}
	}
	catch( ... )
	{
		setState( Idle ) ;
		throw ;
	}
}

void GNet::SimpleClient::onResolved( std::string error , Location location )
{
	try
	{
		if( !error.empty() )
			throw DnsError( error ) ;

		G_DEBUG( "GNet::SimpleClient::onResolved: " << location.displayString() ) ;
		m_remote_location.update( location.address() , location.name() ) ;
		setState( Connecting ) ;
		startConnecting() ;
	}
	catch(...)
	{
		close() ;
		setState( Idle ) ;
		throw ;
	}
}

void GNet::SimpleClient::startConnecting()
{
	try
	{
		G_DEBUG( "GNet::SimpleClient::startConnecting: local: " << m_local_address.displayString() ) ;
		G_DEBUG( "GNet::SimpleClient::startConnecting: remote: " << m_remote_location.displayString() ) ;
		if( G::Test::enabled("slow-client-connect") ) 
			setState( Testing ) ;

		// create and open a socket
		//
		m_socket.reset( new StreamSocket(m_remote_location.address().domain()) ) ;
		socket().addWriteHandler( *this ) ;

		// create a socket protocol object
		//
		m_sp.reset( new SocketProtocol(*this,*this,*m_socket.get(),m_secure_connection_timeout) ) ;

		// bind a local address to the socket (throws on failure)
		//
		if( m_bind_local_address )
			bindLocalAddress( m_local_address ) ;

		// start connecting
		//
		bool immediate = false ;
		if( !socket().connect( m_remote_location.address() , &immediate ) )
			throw ConnectError( c_cannot_connect_to + m_remote_location.address().displayString() ) ;

		// deal with immediate connection (typically if connecting locally)
		//
		if( immediate )
		{
			socket().dropWriteHandler() ;
			m_on_connect_timer.startTimer( 0U ) ; // -> onConnectTimer()
		}
	}
	catch( ... )
	{
		close() ;
		throw ;
	}
}

void GNet::SimpleClient::onConnectTimer()
{
	G_DEBUG( "GNet::SimpleClient::onConnectTimer: immediate connection" ) ;
	onWriteable() ;
}

void GNet::SimpleClient::writeEvent()
{
	G_DEBUG( "GNet::SimpleClient::writeEvent" ) ;
	onWriteable() ;
}

void GNet::SimpleClient::onWriteable()
{
	if( m_state == Connected )
	{
		if( m_sp->writeEvent() )
			onSendComplete() ;
	}
	else if( m_state == Testing )
	{
		socket().dropWriteHandler() ;
		setState( Connecting ) ;
		m_on_connect_timer.startTimer( 2U , 100000U ) ; // -> onConnectTimer()
	}
	else if( m_state == Connecting && socket().hasPeer() )
	{
		socket().dropWriteHandler() ;
		socket().addReadHandler( *this ) ;
		socket().addExceptionHandler( *this ) ;

		if( m_remote_location.socks() )
		{
			setState( Socksing ) ;
			sendSocksRequest() ;
		}
		else
		{
			setState( Connected ) ;
			onConnectImp() ;
			onConnect() ;
		}
	}
	else if( m_state == Connecting )
	{
		socket().dropWriteHandler() ;
		throw ConnectError( c_cannot_connect_to + m_remote_location.address().displayString() ) ;
	}
}

void GNet::SimpleClient::readEvent()
{
	G_ASSERT( m_sp.get() != nullptr ) ;
	if( m_state == Socksing )
	{
		bool complete = readSocksResponse() ;
		if( complete )
		{
			setState( Connected ) ;
			onConnectImp() ;
			onConnect() ;
		}
	}
	else
	{
		if( m_sp.get() != nullptr )
			m_sp->readEvent() ;
	}
}

bool GNet::SimpleClient::connectError( const std::string & error )
{
	return error.find( c_cannot_connect_to ) == 0U ;
}

void GNet::SimpleClient::close()
{
	m_sp.reset() ;
	m_socket.reset() ;
}

bool GNet::SimpleClient::connected() const
{
	return m_state == Connected ;
}

void GNet::SimpleClient::bindLocalAddress( const Address & local_address )
{
	{
		G::Root claim_root ;
		socket().bind( local_address ) ;
	}

	if( local_address.isLoopback() && !m_remote_location.address().isLoopback() )
		G_WARNING_ONCE( "GNet::SimpleClient::bindLocalAddress: binding the loopback address for "
			"outgoing connections may result in connection failures" ) ;
}

void GNet::SimpleClient::setState( State new_state )
{
	m_state = new_state ;
}

std::pair<bool,GNet::Address> GNet::SimpleClient::localAddress() const
{
	return 
		m_socket.get() != nullptr ?
			socket().getLocalAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;
}

std::pair<bool,GNet::Address> GNet::SimpleClient::peerAddress() const
{
	return 
		m_socket.get() != nullptr ?
			socket().getPeerAddress() :
			std::make_pair(false,GNet::Address::defaultAddress()) ;
}

std::string GNet::SimpleClient::peerCertificate() const
{
	return m_sp->peerCertificate() ;
}

void GNet::SimpleClient::sslConnect()
{
	if( m_sp.get() == nullptr )
		throw NotConnected( "for ssl-connect" ) ;
	m_sp->sslConnect() ;
}

void GNet::SimpleClient::onConnectImp()
{
}

bool GNet::SimpleClient::send( const std::string & data , std::string::size_type offset )
{
	bool rc = m_sp->send( data , offset ) ;
	onSendImp() ; // allow derived classes to implement a response timeout
	return rc ;
}

void GNet::SimpleClient::onSendImp()
{
}

void GNet::SimpleClient::sendSocksRequest()
{
	unsigned int far_port = m_remote_location.socksFarPort() ;
	if( !Address::validPort(far_port) ) throw SocksError("invalid port") ;
	g_port_t far_port_n = htons( static_cast<g_port_t>(far_port) ) ;
	g_port_t far_port_lo = far_port_n & 0xffU ;
	g_port_t far_port_hi = (far_port_n>>8U) & g_port_t(0xffU) ;

	std::string userid ; // TODO - socks userid
	std::string data ;
	data.append( 1U , 4 ) ; // version 4
	data.append( 1U , 1 ) ; // connect request
	data.append( 1U , static_cast<char>(far_port_lo) ) ;
	data.append( 1U , static_cast<char>(far_port_hi) ) ;
	data.append( 1U , 0 ) ;
	data.append( 1U , 0 ) ;
	data.append( 1U , 0 ) ;
	data.append( 1U , 1 ) ;
	data.append( userid ) ;
	data.append( 1U , 0 ) ; // NUL
	data.append( m_remote_location.socksFarHost() ) ;
	data.append( 1U , 0 ) ; // NUL
	GNet::Socket::ssize_type n = socket().write( data.data() , data.size() ) ;
	if( static_cast<std::string::size_type>(n) != data.size() ) // TODO - socks flow control
		throw SocksError( "request not sent" ) ;
}

bool GNet::SimpleClient::readSocksResponse()
{
	char buffer[8] ;
	GNet::Socket::ssize_type rc = socket().read( buffer , sizeof(buffer) ) ;
	if( rc == 0 || ( rc == -1 && !socket().eWouldBlock() ) ) throw SocksError( "read error" ) ;
	else if( rc == -1 ) return false ; // go again
	if( rc != 8 ) throw SocksError( "incomplete response" ) ; // TODO - socks response reassembly
	if( buffer[0] != 0 ) throw SocksError( "invalid response" ) ;
	if( buffer[1] != 'Z' ) throw SocksError( "request rejected" ) ;
	G_LOG( "GNet::SimpleClient::readSocksResponse: " << logId() << ": socks connection completed" ) ;
	return true ;
}

bool GNet::SimpleClient::synchronousDnsDefault()
{
	if( G::Test::enabled("dns-asynchronous") ) return false ;
	if( G::Test::enabled("dns-synchronous") ) return true ;
	return false ; // default to async, but G::thread might run synchronously as a build-time option
}

/// \file gsimpleclient.cpp

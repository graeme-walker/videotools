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
/// \file gclient.h
///

#ifndef G_CLIENT_H
#define G_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gheapclient.h"
#include "glinebuffer.h"
#include "gtimer.h"
#include "gslot.h"

namespace GNet
{
	class ClientTimer ;
	class Client ;
}

/// \class GNet::Client
/// A HeapClient class that adds slot/signal signalling, connection/response 
/// timeouts, and input line buffering.
/// 
/// The following pure virtual functions must be implemented by derived 
/// classes: onConnect(), onReceive(), onDelete(), and onSendComplete().
/// 
/// The intention is that derived classes use the virtual functions for 
/// event notification while the slot/signal event notifications are used 
/// by the containing object (in particular so that the container is 
/// informed when the client object deletes itself).
/// 
class GNet::Client : public HeapClient
{
public:
	explicit Client( const Location & remote_info , unsigned int connection_timeout = 0U ,
		unsigned int response_timeout = 0U , unsigned int secure_connection_timeout = 0U ,
		const std::string & eol = std::string("\n") ,
		bool bind_local_address = false ,
		const Address & local_address = Address::defaultAddress() ,
		bool sync_dns = synchronousDnsDefault() ) ;
			///< Constructor.

	G::Slot::Signal1<std::string> & doneSignal() ;
		///< Returns a signal that indicates that client processing
		///< is complete.
		///< 
		///< The first signal parameter is a failure reason, or the
		///< empty string on success. The second parameter gives some
		///< idea of whether the connection can be retried later.

	G::Slot::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal that indicates that something interesting
		///< has happened. 
		///< 
		///< The first signal parameter is one of "connecting",
		///< "connected", "sending", "failed" (on error) or "done"
		///< (for normal termination). Derived clases may emit
		///< other events over this channel.

	G::Slot::Signal0 & connectedSignal() ;
		///< Returns a signal that indicates that the client
		///< has successfully connected to the server.

	G::Slot::Signal0 & secureSignal() ;
		///< Returns a signal that indicates that the security
		///< layer has been successfully established.

protected:
	virtual ~Client() ;
		///< Destructor.

	virtual bool onReceive( const std::string & ) = 0 ;
		///< Called when a complete line is received from the peer.
		///< The implementation should return false if it needs
		///< to stop further onReceive() calls being generated
		///< from data already received and buffered, if for
		///< example the object has just deleted itself.

	void clearInput() ;
		///< Clears any pending input from the server.

    virtual void onDeleteImp( const std::string & reason ) override ;
		///< Override from GNet::HeapClient.

    virtual void onConnectImp() override ;
		///< Override from GNet::SimpleClient.

	virtual void onData( const char * , SimpleClient::size_type ) override ;
		///< Override from GNet::SocketProtocolSink.

	virtual void onConnecting() override ;
		///< Override from GNet::HeapClient.

	virtual void onSendImp() override ;
		///< Override from GNet::SimpleClient.

private:
	Client( const Client& ) ; // not implemented
	void operator=( const Client& ) ; // not implemented
	void onConnectionTimeout() ;
	void onResponseTimeout() ;

private:
	G::Slot::Signal1<std::string> m_done_signal ;
	G::Slot::Signal2<std::string,std::string> m_event_signal ;
	G::Slot::Signal0 m_connected_signal ;
	G::Slot::Signal0 m_secure_signal ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::Timer<Client> m_connection_timer ;
	GNet::Timer<Client> m_response_timer ;
	GNet::LineBuffer m_line_buffer ;
} ;

#endif

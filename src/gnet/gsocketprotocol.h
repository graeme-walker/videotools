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
/// \file gsocketprotocol.h
///

#ifndef G_SOCKET_PROTOCOL_H
#define G_SOCKET_PROTOCOL_H

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <utility>

namespace GNet
{
	class SocketProtocol ;
	class SocketProtocolImp ;
	class SocketProtocolSink ;
}

/// \class GNet::SocketProtocol
/// An interface for implementing a low-level TLS/SSL protocol layer on top 
/// of a connected non-blocking socket.
/// 
/// Provides send() to send data, and onData() in a callback interface to
/// receive data. The TLS/SSL socket protocol session is negotiated with the 
/// peer by calling sslConnect() or sslAccept(), and thereafter the interface 
/// is half-duplex. If no TLS/SSL session is in effect ('raw') then the protocol 
/// layer is transparent down to the socket. 
/// 
/// The interface has read-event and write-event handlers that should be
/// called when events are detected on the socket file descriptor. The read
/// handler delivers data via the onData() callback interface; the write
/// handler is used to flush the output pipeline.
/// 
class GNet::SocketProtocol
{
public:
	typedef SocketProtocolSink Sink ;
	G_EXCEPTION_CLASS( ReadError , "peer disconnected" ) ;
	G_EXCEPTION( SendError , "peer disconnected" ) ;
	G_EXCEPTION( SecureConnectionTimeout , "secure connection timeout" ) ;

	SocketProtocol( EventHandler & , Sink & , StreamSocket & , unsigned int secure_connection_timeout ) ;
		///< Constructor. The references are kept.

	~SocketProtocol() ;
		///< Destructor.

	void readEvent() ;
		///< Called on receipt of a read event. Delivers data via the sink 
		///< interface. Throws ReadError on error.

	bool writeEvent() ;
		///< Called on receipt of a write event. Sends more pending data 
		///< down the connection. Returns true if all pending data was
		///< sent. Throws SendError on error.

	bool send( const std::string & data , size_t offset = 0U ) ;
		///< Sends data. Returns false if flow control asserted before
		///< all the data is sent. Returns true if all the data was sent,
		///< or if the data passed in (taking the offset into account) 
		///< is empty. Throws SendError on error.
		///< 
		///< If flow control is asserted then the socket write-event
		///< handler is installed; when the subsequent write-event is
		///< triggered the user should call writeEvent(). There should
		///< be no new calls to send() until writeEvent() returns true.

	bool send( const std::vector<std::pair<const char *,size_t> > & data ) ;
		///< Overload to send data using scatter-gather segments.
		///< If false is returned then segment data pointers must 
		///< stay valid until writeEvent() returns true.

	static bool sslCapable() ;
		///< Returns true if the implementation supports TLS/SSL.

	void sslConnect() ;
		///< Initiates the TLS/SSL protocol.

	void sslAccept() ;
		///< Accepts the TLS/SSL protocol.

	bool sslEnabled() const ;
		///< Returns true if TLS/SSL is active.

	std::string peerCertificate() const ;
		///< Returns the peer's TLS/SSL certificate
		///< or the empty string.

private:
	SocketProtocol( const SocketProtocol & ) ;
	void operator=( const SocketProtocol & ) ;

private:
	SocketProtocolImp * m_imp ;
} ;

/// \class GNet::SocketProtocolSink
/// to deliver data from a socket.
/// 
class GNet::SocketProtocolSink
{
public:
	virtual ~SocketProtocolSink() ;
		///< Destructor.

protected:
	friend class SocketProtocolImp ;

	virtual void onData( const char * , size_t ) = 0 ;
		///< Called when data is read from the socket.

	virtual void onSecure( const std::string & peer_certificate ) = 0 ;
		///< Called once the secure socket protocol has
		///< been successfully negotiated.
} ;

#endif

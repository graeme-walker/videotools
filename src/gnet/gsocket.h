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
/// \file gsocket.h
///

#ifndef G_SOCKET_H
#define G_SOCKET_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gexception.h"
#include "gevent.h"
#include "gdescriptor.h"
#include "greadwrite.h"
#include <string>

namespace GNet
{
	class Socket ;
	class SocketProtocol ;
	class StreamSocket ;
	class DatagramSocket ;
	class AcceptPair ;
}

/// \class GNet::Socket
/// 
/// The Socket class encapsulates a non-blocking Unix socket file descriptor or a
/// Windows 'SOCKET' handle. 
/// 
/// (Non-blocking network i/o is particularly appropriate for single-threaded 
/// server processes which manage multiple client connections. The main 
/// disagvantage is that flow control has to be managed explicitly: see 
/// Socket::write() and Socket::eWouldBlock().)
/// 
/// Provides bind(), listen(), connect(), write(); derived classes provide 
/// accept() and read(). Also interfaces to the event loop with addReadHandler()
/// and addWriteHandler().
/// 
class GNet::Socket : public G::ReadWrite
{
public:
	G_EXCEPTION( SocketError , "socket error" ) ;
	G_EXCEPTION( SocketBindError , "socket bind error" ) ;
	typedef G::ReadWrite::size_type size_type ;
	typedef G::ReadWrite::ssize_type ssize_type ;
	struct NoThrow /// Overload discriminator class for GNet::Socket.
		{} ;

	virtual ~Socket() ;
		///< Destructor.

	std::pair<bool,Address> getLocalAddress() const ;
		///< Retrieves local address of the socket.
		///< Pair.first is false on error.

	std::pair<bool,Address> getPeerAddress() const ;
		///< Retrieves address of socket's peer.
		///< 'Pair.first' is false on error.

	bool hasPeer() const ;
		///< Returns true if the socket has a valid peer. This
		///< can be used to see if a connect succeeded.

	void bind( const Address & ) ;
		///< Binds the socket with the given address.

	bool bind( const Address & , NoThrow ) ;
		///< No-throw overload. Returns false on error.

	bool canBindHint( const Address & address ) ;
		///< Returns true if the socket can probably be bound 
		///< with the given address. Some implementations will 
		///< always return true. This method should be used on 
		///< a temporary socket of the correct dynamic type 
		///< since this socket may become unusable.

	bool connect( const Address & addr , bool *done = nullptr ) ;
		///< Initiates a connection to (or association with) 
		///< the given address. Returns false on error.
		///< 
		///< If successful, a 'done' flag is returned by 
		///< reference indicating whether the connect completed 
		///< immediately. Normally a stream socket connection 
		///< will take some time to complete so the 'done' flag 
		///< will be false: the completion will be indicated by 
		///< a write event some time later.
		///< 
		///< For datagram sockets this sets up an association 
		///< between two addresses. The socket should first be 
		///< bound with a local address.

	void listen( int backlog = 1 ) ;
		///< Starts the socket listening on the bound
		///< address for incoming connections or incoming
		///< datagrams.

	virtual ssize_type read( char * buffer , size_type buffer_length ) override = 0 ;
		///< Reads from the socket. This is a default implementation
		///< that can be called explicitly from derived classes. 

	virtual ssize_type write( const char * buf , size_type len ) override = 0 ;
		///< Writes to the socket. This is a default implementation
		///< that can be called explicitly from derived classes. 
		///< Override from G::ReadWrite.

	virtual SOCKET fd() const ;
		///< Returns the socket descriptor. Override from G::ReadWrite.

	virtual bool eWouldBlock() ;
		///< Returns true if the previous socket operation
		///< failed because the socket would have blocked.
		///< Override from G::ReadWrite.

	bool eInProgress() ;
		///< Returns true if the previous socket operation
		///< failed with the EINPROGRESS error status.
		///< When connecting this can be considered a
		///< non-error.

	bool eMsgSize() ;
		///< Returns true if the previous socket operation
		///< failed with the EMSGSIZE error status. When 
		///< writing to a datagram socket this indicates that 
		///< the message was too big to send atomically.

	void addReadHandler( EventHandler & handler ) ;
		///< Adds this socket to the event source list so that 
		///< the given handler receives read events.

	void dropReadHandler();
		///< Reverses addReadHandler().

	void addWriteHandler( EventHandler & handler ) ;
		///< Adds this socket to the event source list so that 
		///< the given handler receives write events when flow 
		///< control is released. (Not used for datagram 
		///< sockets.)

	void dropWriteHandler() ;
		///< Reverses addWriteHandler().

	void addExceptionHandler( EventHandler & handler );
		///< Adds this socket to the event source list so that 
		///< the given handler receives exception events. 
		///< A TCP exception event should be treated as a 
		///< disconnection event. (Not used for datagram 
		///< sockets.)

	void dropExceptionHandler() ;
		///< Reverses addExceptionHandler().

	std::string asString() const ;
		///< Returns the socket handle as a string.
		///< Only used in debugging.

	std::string reasonString() const ;
		///< Returns the failure reason as a string.
		///< Only used in debugging.

	void shutdown( bool for_writing = true ) ;
		///< Shuts the socket for writing (or reading).

protected:
	Socket( int domain , int type , int protocol ) ;
		///< Constructor used by derived classes. Opens the 
		///< socket using socket().

	explicit Socket( Descriptor s ) ;
		///< Constructor which creates a socket object from 
		///< an existing socket handle. Used only by 
		///< StreamSocket::accept().

protected:
	static int reason() ;
	static std::string reasonStringImp( int ) ;
	static bool error( int rc ) ;
	static bool sizeError( ssize_type size ) ;
	void saveReason() ;
	void saveReason() const ;
	void prepare() ;
	void setFault() ;
	void setOptionNoLinger() ;
	void setOptionReuse() ;
	void setOptionExclusive() ;
	void setOptionPureV6( bool ) ;
	bool setOptionPureV6( bool , NoThrow ) ;
	void setOptionKeepAlive() ;
	void setOption( int , const char * , int , int ) ;
	bool setOption( int , const char * , int , int , NoThrow ) ;
	bool setOptionImp( int , int , const void * , socklen_t ) ;
	std::pair<bool,Address> getAddress( bool ) const ;

private:
	void doClose() ;
	bool setNonBlock() ;

protected:
	int m_reason ;
	std::string m_reason_string ;
	int m_domain ;
	Descriptor m_socket ;

private:
	Socket( const Socket & ) ;
	void operator=( const Socket & ) ;
	void drop() ;
} ;

//

/// \class GNet::AcceptPair
/// A class which is used to return a new()ed socket to calling code, together 
/// with associated information.
/// 
class GNet::AcceptPair
{
public:
	shared_ptr<StreamSocket> first ;
	Address second ;
	AcceptPair() : second(Address::defaultAddress()) {}
} ;

//

/// \class GNet::StreamSocket
/// A derivation of GNet::Socket for a stream socket. 
/// 
class GNet::StreamSocket : public Socket
{
public:
	typedef Socket::size_type size_type ;
	typedef Socket::ssize_type ssize_type ;

	explicit StreamSocket( int address_domain ) ;
		///< Constructor with a hint of the bind()/connect()
		///< address to be used later.

	virtual ~StreamSocket() ;
		///< Destructor.

	virtual ssize_type read( char * buffer , size_type buffer_length ) ;
		///< Override from Socket::read().

	virtual ssize_type write( const char * buf , size_type len ) ;
		///< Override from Socket::write().

	AcceptPair accept() ;
		///< Accepts an incoming connection, returning
		///< a new()ed socket and the peer address.

private:
	StreamSocket( const StreamSocket & ) ; // not implemented
	void operator=( const StreamSocket & ) ; // not implemented
	StreamSocket( Descriptor s ) ; // A private constructor used in accept().
} ;

//

/// \class GNet::DatagramSocket
/// A derivation of GNet::Socket for a datagram socket. 
/// 
class GNet::DatagramSocket : public Socket
{
public:
	explicit DatagramSocket( int address_domain ) ;
		///< Constructor with a hint of a local address.

	virtual ~DatagramSocket() ;
		///< Destructor.

	virtual ssize_type read( char * buffer , size_type len ) ; 
		///< Override from Socket::read().

	virtual ssize_type write( const char * buffer , size_type len ) ;
		///< Override from Socket::write().

	ssize_type readfrom( char * buffer , size_type len , Address & src ) ; 
		///< Reads a datagram and returns the sender's address by reference. 
		///< If connect() has been used then only datagrams from the address 
		///< specified in the connect() call will be received.

	ssize_type writeto( const char * buffer , size_type len , const Address & dst ) ; 
		///< Sends a datagram to the given address. This should be used
		///< if there is no connect() assocation in effect.

	void disconnect() ;
		///< Releases the association between two datagram endpoints 
		///< reversing the effect of the previous Socket::connect().

private:
	DatagramSocket( const DatagramSocket & ) ; // not implemented
	void operator=( const DatagramSocket & ) ; // not implemented
} ;

#endif

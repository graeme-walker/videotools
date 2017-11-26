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
/// \file gvcommandsocket.h
///

#ifndef GV_COMMAND_SOCKET__H
#define GV_COMMAND_SOCKET__H

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "glocalsocket.h"
#include "gsignalsafe.h"
#include <vector>
#include <string>

namespace Gv
{
	class CommandSocket ;
	class CommandSocketMixin ;
	class CommandSocketMixinImp ;
}

/// \class Gv::CommandSocket
/// A non-blocking datagram socket that is used for sending and receiving 
/// process control commands. The socket is either a local unix-domain socket, 
/// or a UDP network socket, depending on the format of the socket name, 
/// although a prefix of "udp://" can be used to override the heuristics. 
/// Synchronous DNS lookups are used if necessary.
/// 
class Gv::CommandSocket
{
public:
	struct Type /// Describes a Gv::CommandSocket as a UDP address or unix-domain path.
	{
		bool net ;
		std::string path ;
		unsigned int port ;
	} ;
	struct NoThrow /// Overload descriminator for Gv::CommandSocket
	{
	} ;

	explicit CommandSocket( const std::string & bind_name ) ;
		///< Constructor for a receiving socket, taking a local filesystem path 
		///< or a transport address. Does synchronous dns resolution if necessary.
		///< Constructs a do-nothing object if the name is empty.
		///< On error fd() returns -1.

	CommandSocket() ;
		///< Constructor for a sending socket.

	void connect( const std::string & connect_name ) ;
		///< Creates an association with the remote socket, taking a local filesystem 
		///< path or a transport address. Does synchronous dns resolution if necessary.
		///< Throws on error.

	std::string connect( const std::string & connect_name , NoThrow ) ;
		///< Creates an association with the remote socket, taking a local filesystem 
		///< path or a transport address. Does synchronous dns resolution if necessary.
		///< Returns a failure reason on error.

	int fd() const ;
		///< Returns the file descriptor.

	std::string read() ;
		///< Reads the socket. Returns the empty string on error.

	static Type parse( const std::string & bind_name ) ;
		///< Parses a filesystem path or transport address.

	void close() ;
		///< Closes the sending socket, if open.

private:
	CommandSocket( const CommandSocket & ) ;
	void operator=( const CommandSocket & ) ;
	static void cleanup( G::SignalSafe , const char * ) ;

private:
	unique_ptr<G::LocalSocket> m_local ;
	unique_ptr<GNet::DatagramSocket> m_net ;
	std::vector<char> m_buffer ;
} ;

/// \class Gv::CommandSocketMixin
/// A mixin base class that contains a bound Gv::CommandSocket object
/// integrated with the GNet::EventLoop.
/// 
class Gv::CommandSocketMixin
{
public:
	explicit CommandSocketMixin( const std::string & bind_name ) ;
		///< Constructor.

	virtual ~CommandSocketMixin() ;
		///< Destructor.

	virtual void onCommandSocketData( std::string ) = 0 ;
		///< Callback that delivers a datagram.

private:
	CommandSocketMixin( const CommandSocketMixin & ) ;
	void operator=( const CommandSocketMixin & ) ;

private:
	CommandSocketMixinImp * m_imp ;
} ;

#endif

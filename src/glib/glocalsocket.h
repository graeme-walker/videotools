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
/// \file glocalsocket.h
///

#ifndef G_LOCAL_SOCKET__H
#define G_LOCAL_SOCKET__H

#include "gdef.h"
#include "gexception.h"
#include "gsignalsafe.h"
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>

namespace G
{
	class LocalSocketAddress ;
	class LocalSocket ;
}

/// \class G::LocalSocketAddress
/// An address class for G::LocalSocket that works with file system paths.
/// 
class G::LocalSocketAddress
{
public:
	explicit LocalSocketAddress( const std::string & path ) ;
		///< Constructor.

	LocalSocketAddress( SignalSafe , const char * path ) ;
		///< Constructor. Signal-safe overload.

	struct sockaddr * p() ;
		///< Returns the sockaddr pointer.

	const struct sockaddr * p() const ;
		///< Const overload.

	size_t size() const ;
		///< Returns the sockaddr size.

private:
	union Union
	{
		struct sockaddr_un specific ;
		struct sockaddr general ;
	} ;
	Union m_u ;
} ;

/// \class G::LocalSocket
/// and accept()ing should be performed directly on the file descriptor.
/// \see G::Msg
/// 
class G::LocalSocket
{
public:
	typedef LocalSocketAddress Address ;
	G_EXCEPTION( Error , "local socket error" ) ;

	explicit LocalSocket( bool datagram ) ;
		///< Constructor for a datagram socket or a stream socket.

	~LocalSocket() ;
		///< Destructor.

	bool connect( const std::string & address_path , bool no_throw = false ) ;
		///< Connects to the given address. For datagram sockets this just becomes 
		///< the default destination address. By default throws Error on error.

	void bind( const std::string & address_path ) ;
		///< Binds the given address to the socket. Throws Error on error.

	void nonblock() ;
		///< Makes the socket non-blocking.

	int fd() const ;
		///< Returns the socket's file descriptor.

	bool connected() const ;
		///< Returns true if the socket has been successfully connect()ed.

private:
	LocalSocket( const LocalSocket & ) ;
	void operator=( const LocalSocket & ) ;

private:
	int m_fd ;
	bool m_connected ;
} ;

#endif

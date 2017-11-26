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
// gfutureevent_unix.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gfutureevent.h"
#include "gmsg.h"
#include "geventloop.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/// \class GNet::FutureEventImp
/// A pimple-pattern implementation class used by GNet::FutureEvent.
///
class GNet::FutureEventImp : public EventHandler
{
public:
	typedef FutureEvent::Error Error ;
	typedef FutureEvent::handle_type handle_type ;

	explicit FutureEventImp( FutureEventHandler & handler ) ;
		// Constructor.

	virtual ~FutureEventImp() ;
		// Destructor.

	unsigned int receive() ;
		// Reads from the read socket.

	static bool send( handle_type , unsigned int ) g__noexcept ;
		// Writes to the write socket.

	handle_type handle() const ;
		// Returns the socket fd as a handle.

private:
	FutureEventImp( const FutureEventImp & ) ;
	void operator=( const FutureEventImp & ) ;
	static int init( int ) ;
	virtual void readEvent() ; // Override from GNet::EventHandler.
	virtual void onException( std::exception & ) ; // Override from GNet::EventExceptionHandler.

private:
	FutureEventHandler & m_handler ;
	int m_fd_read ;
	int m_fd_write ;
} ;

GNet::FutureEventImp::FutureEventImp( FutureEventHandler & handler ) :
	m_handler(handler) ,
	m_fd_read(-1) ,
	m_fd_write(-1)
{
	int fds[2] ;
	int rc = ::socketpair( AF_UNIX , SOCK_DGRAM , 0 , fds ) ;
	if( rc != 0 )
		throw Error("socketpair") ;
	m_fd_read = init( fds[0] ) ;
	m_fd_write = init( fds[1] ) ;
	EventLoop::instance().addRead( Descriptor(m_fd_read) , *this ) ;
}

int GNet::FutureEventImp::init( int fd )
{
	int rc = ::fcntl( fd , F_SETFL , ::fcntl(fd,F_GETFL) | O_NONBLOCK ) ; G_IGNORE_VARIABLE(rc) ;
	return fd ;
}

GNet::FutureEventImp::~FutureEventImp()
{
	if( m_fd_read >= 0 ) 
	{
		if( EventLoop::exists() ) // (in case we're in the process's last gasp)
			EventLoop::instance().dropRead( Descriptor(m_fd_read) ) ;
		::close( m_fd_read ) ;
	}
	if( m_fd_write >= 0 ) 
	{
		::close( m_fd_write ) ;
	}
}

GNet::FutureEventImp::handle_type GNet::FutureEventImp::handle() const
{
	return static_cast<handle_type>(m_fd_write) ;
}

unsigned int GNet::FutureEventImp::receive()
{
	char c = 0 ;
	ssize_t rc = ::recv( m_fd_read , &c , 1 , 0 ) ;
	if( rc != 1 )
		throw Error("recv") ;
	return static_cast<unsigned int>(c) ;
}

bool GNet::FutureEventImp::send( handle_type handle , unsigned int payload ) g__noexcept
{
	int fd = static_cast<int>(handle) ;
	char c = static_cast<char>(payload) ;
	ssize_t rc = G::Msg::send( fd , &c , 1 , 0 ) ;
	const bool ok = rc == 1 ;
	return ok ;
}

void GNet::FutureEventImp::readEvent()
{
	m_handler.onFutureEvent( receive() ) ;
}

void GNet::FutureEventImp::onException( std::exception & e )
{
	m_handler.onException( e ) ;
}

// ==

GNet::FutureEvent::FutureEvent( FutureEventHandler & handler ) :
	m_imp(new FutureEventImp(handler))
{
}

GNet::FutureEvent::~FutureEvent()
{
}

bool GNet::FutureEvent::send( handle_type handle , unsigned int payload ) g__noexcept
{
	return FutureEventImp::send( handle , payload ) ;
}

GNet::FutureEvent::handle_type GNet::FutureEvent::handle() const
{
	return m_imp->handle() ;
}

// ==

GNet::FutureEventHandler::~FutureEventHandler()
{
}


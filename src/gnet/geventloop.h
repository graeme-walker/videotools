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
/// \file geventloop.h
///

#ifndef G_EVENT_LOOP_H
#define G_EVENT_LOOP_H

#include "gdef.h"
#include "gnet.h"
#include "geventhandler.h"
#include "gexception.h"
#include "gdatetime.h"
#include "gdescriptor.h"
#include "gsignalsafe.h"
#include <list>
#include <string>

namespace GNet
{
	class EventLoop ;
}

/// \class GNet::EventLoop
/// An abstract base class for a singleton that keeps track of open sockets 
/// and their associated handlers. Derived classes are used to implement 
/// different event loops, such as select() or WinSock.
/// 
/// In practice sockets are added and removed from the class by calling
/// GNet::Socket::addReadHandler() etc rather than EventLoop::addRead(). 
/// This is to improve the encapsulation of the GNet::Descriptor data type 
/// within Socket.
/// 
/// The class has a static member for finding an instance, but instances 
/// are not created automatically.
/// 
class GNet::EventLoop
{
public:
	G_EXCEPTION( Error , "failed to initialise the event loop" ) ;
	G_EXCEPTION( NoInstance , "no event loop instance" ) ;

protected:
	EventLoop() ;
		///< Constructor.

	struct Running /// RAII class that sets a boolean, for implementations of run()/running().
	{
		Running( bool & bref ) : m_bref(bref) { m_bref = true ; }
		~Running() { m_bref = false ; }
		bool & m_bref ;
	} ;

public:
	static EventLoop * create() ;
		///< A factory method which creates an instance of a derived 
		///< class on the heap. Throws on error.

	static EventLoop & instance() ;
		///< Returns a reference to an instance of the class, 
		///< if any. Throws if none. Does not do any instantiation 
		///< itself.

	static bool exists() ;
		///< Returns true if an instance exists.

	static void stop( const G::SignalSafe & ) ;
		///< Calls quit() on instance().

	virtual ~EventLoop() ;
		///< Destructor.

	virtual std::string run() = 0 ;
		///< Runs the main event loop. Returns a quit() reason,
		///< if any.

	virtual bool running() const = 0 ;
		///< Returns true if called from within run().

	virtual void quit( std::string reason ) = 0 ;
		///< Causes run() to return (once the call stack has 
		///< unwound). If there are multiple quit()s before 
		///< run() returns then the latest reason is used.

	virtual void quit( const G::SignalSafe & ) = 0 ;
		///< A signal-safe overload to quit() the event loop.

	virtual void addRead( Descriptor fd , EventHandler & handler ) = 0 ;
		///< Adds the given event source descriptor and associated 
		///< handler to the read list. 
		///< See also Socket::addReadHandler().

	virtual void addWrite( Descriptor fd , EventHandler & handler ) = 0 ;
		///< Adds the given event source descriptor and associated 
		///< handler to the write list.
		///< See also Socket::addWriteHandler().

	virtual void addException( Descriptor fd , EventHandler & handler ) = 0 ;
		///< Adds the given event source descriptor and associated 
		///< handler to the exception list.
		///< See also Socket::addExceptionHandler().

	virtual void dropRead( Descriptor fd ) = 0 ;
		///< Removes the given event source descriptor from the 
		///< list of read sources.
		///< See also Socket::dropReadHandler().

	virtual void dropWrite( Descriptor fd ) = 0 ;
		///< Removes the given event source descriptor from the 
		///< list of write sources.
		///< See also Socket::dropWriteHandler().

	virtual void dropException( Descriptor fd ) = 0 ;
		///< Removes the given event source descriptor from the 
		///< list of exception sources.
		///< See also Socket::dropExceptionHandler().

	virtual void setTimeout( G::EpochTime t ) = 0 ;
		///< Used by GNet::TimerList. Sets the absolute time at which
		///< TimerList::doTimeouts() is to be called. A zero time 
		///< is used if there are no active timers.

	virtual std::string report() const = 0 ;
		///< Returns a line of text reporting the status of the event loop.
		///< Used in debugging and diagnostics.

private:
	static EventLoop * m_this ;
} ;

#endif

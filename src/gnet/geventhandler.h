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
/// \file geventhandler.h
///

#ifndef G_EVENT_HANDLER_H
#define G_EVENT_HANDLER_H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "gdescriptor.h"
#include <vector>
#include <string>

namespace GNet
{
	class EventHandler ;
	class EventExceptionHandler ;
	class EventHandlerList ;
	class TimerList ;
}

/// \class GNet::EventExceptionHandler
/// An abstract interface for handling exceptions thrown out of event-loop 
/// callbacks (socket events and timer events). If the handler just rethrows 
/// then the event loop will terminate.
/// 
class GNet::EventExceptionHandler
{
public:
	virtual void onException( std::exception & ) = 0 ;
		///< Called by the event loop when an exception is thrown out
		///< of an event loop callback.
		///< 
		///< The implementation may just do a "throw" to throw the current 
		///< exception out of the event loop, or a "delete this" for 
		///< objects that manage themselves on the heap.
		///< 
		///< EventHandler objects or timer objects that are sub-objects 
		///< of other EventHandler objects will normally have their 
		///< implementation of onException() delegate to the outer 
		///< object's onException().

protected:
	virtual ~EventExceptionHandler() ;
		///< Destructor.

private:
	void operator=( const EventExceptionHandler & ) ;
} ;

/// \class GNet::EventHandler
/// A base class for classes that handle asynchronous events from the event loop.
/// 
/// An event handler object has its virtual methods called when an event is 
/// detected on the associated file descriptor.
/// 
/// If an event handler throws an exception which is caught by the event loop 
/// then the event loop calls the handler's EventExceptionHandler::onException() 
/// method.
/// 
class GNet::EventHandler : public EventExceptionHandler
{
public:
	virtual ~EventHandler() ;
		///< Destructor.

	virtual void readEvent() ;
		///< Called for a read event. Overridable. The default 
		///< implementation does nothing.

	virtual void writeEvent() ;
		///< Called for a write event. Overrideable. The default 
		///< implementation does nothing.

	virtual void exceptionEvent() ;
		///< Called for a socket-exception event. Overridable. The default 
		///< implementation throws an exception resulting in a call to 
		///< GNet::EventExceptionHandler::onException().

private:
	void operator=( const EventHandler & ) ;
} ;

/// \class GNet::EventHandlerList
/// A class that maps from a file descriptor to an event handler, used in the 
/// implemention of classes derived from GNet::EventLoop.
/// 
class GNet::EventHandlerList
{
public:
	struct Iterator /// An iterator for file-descriptor/handler-function pairs in a GNet::EventHandlerList.
	{
		typedef std::pair<Descriptor,EventHandler*> Pair ;
		Iterator( const std::vector<Pair> & , bool ) ;
		Iterator & operator++() ;
		const Pair & operator*() const ;
		bool operator==( const Iterator & ) const ;
		bool operator!=( const Iterator & ) const ;
		Descriptor fd() const ;
		EventHandler * handler() ;
		std::vector<Pair>::const_iterator m_p ;
		std::vector<Pair>::const_iterator m_end ;
	} ;

public:
	explicit EventHandlerList( const std::string & type ) ;
		///< Constructor. The type parameter (eg. "read") is used only in 
		///< debugging messages.

	void add( Descriptor fd , EventHandler * handler ) ;
		///< Adds a file-descriptor/handler pair to the list.

	void remove( Descriptor fd ) ;
		///< Removes a file-descriptor from the list.

	bool contains( Descriptor fd ) const ;
		///< Returns true if the list contains the given file-descriptor.

	EventHandler * find( Descriptor fd ) ;
		///< Finds the handler associated with the given file descriptor.

	void lock() ; 
		///< To be called at the start of an begin()/end() iteration if the
		///< list might change during the iteration. Must be paired with 
		///< unlock().

	void unlock() ; 
		///< Called at the end of a begin()/end() iteration to match a call 
		///< to lock(). Does garbage collection.

	void collectGarbage() ;
		///< Collects garbage resulting from remove()s. Automatically called 
		///< from unlock().

	Iterator begin() const ;
		///< Returns a forward iterator.

	Iterator end() const ;
		///< Returns an end iterator.

private:
	typedef std::vector<Iterator::Pair> List ;
	EventHandlerList( const EventHandlerList & ) ;
	void operator=( const EventHandlerList & ) ;
	static bool contains( const List & , Descriptor fd ) ;
	static EventHandler * find( List & , Descriptor fd ) ;
	static void add( List & list , Descriptor fd , EventHandler * handler ) ;
	static bool disable( List & list , Descriptor fd ) ;
	static bool remove( List & list , Descriptor fd ) ;
	void commit() ;

private:
	std::string m_type ;
	List m_list ;
	List m_pending_list ;
	unsigned int m_lock ;
	bool m_has_garbage ;
} ;


inline
GNet::EventHandlerList::Iterator GNet::EventHandlerList::begin() const
{
	return Iterator( m_list , false ) ;
}

inline
GNet::EventHandlerList::Iterator GNet::EventHandlerList::end() const
{
	return Iterator( m_list , true ) ;
}


inline
GNet::EventHandlerList::Iterator::Iterator( const std::vector<Pair> & v , bool end ) :
	m_p(end?v.end():v.begin()) ,
	m_end(v.end())
{
	while( m_p != m_end && (*m_p).second == nullptr )
		++m_p ;
}

inline
GNet::EventHandlerList::Iterator & GNet::EventHandlerList::Iterator::operator++()
{
	++m_p ;
	while( m_p != m_end && (*m_p).second == nullptr )
		++m_p ;
	return *this ;
}

inline
const GNet::EventHandlerList::Iterator::Pair & GNet::EventHandlerList::Iterator::operator*() const
{
	return *m_p ;
}

inline
GNet::Descriptor GNet::EventHandlerList::Iterator::fd() const
{
	return (*m_p).first ;
}

inline
GNet::EventHandler * GNet::EventHandlerList::Iterator::handler()
{
	return (*m_p).second ;
}

inline
bool GNet::EventHandlerList::Iterator::operator==( const Iterator & other ) const
{
	return m_p == other.m_p ;
}

inline
bool GNet::EventHandlerList::Iterator::operator!=( const Iterator & other ) const
{
	return !(*this == other) ;
}

#endif

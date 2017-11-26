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
/// \file gclientptr.h
///

#ifndef G_SMTP_CLIENT_PTR_H
#define G_SMTP_CLIENT_PTR_H

#include "gdef.h"
#include "gnet.h"
#include "gclient.h"
#include "gexception.h"
#include "gassert.h"
#include "gslot.h"
#include <memory>

namespace GNet
{

/// \class ClientPtr
/// A smart pointer class for GNet::HeapClient that keeps track of when the 
/// contained instance deletes itself. When the smart pointer goes out of 
/// scope the HeapClient object is told to delete itself asynchronously 
/// using its doDelete() mechanism. The HeapClient's slots-and-signals
/// are managed automatically so that client code does not have to 
/// disconnect and reconnect them.
/// 
template <typename TClient>
class ClientPtr
{
public:
	G_EXCEPTION( InvalidState , "invalid state of smtp client" ) ;

	explicit ClientPtr( TClient * p = nullptr ) ;
		///< Constructor. Takes ownership of the new-ed client.

	~ClientPtr() ;
		///< Destructor.

	bool busy() const ;
		///< Returns true if the pointer is not nullptr.

	void reset( TClient * p = nullptr ) ;
		///< Resets the pointer.

	TClient * get() ;
		///< Returns the pointer, or nullptr if deleted.

	TClient * operator->() ;
		///< Returns the pointer. Throws if deleted.

	const TClient * operator->() const ;
		///< Returns the pointer. Throws if deleted.

	G::Slot::Signal1<std::string> & doneSignal() ;
		///< Returns a signal which indicates that client processing
		///< is complete and the client instance has deleted
		///< itself. The signal parameter is the failure
		///< reason.

	G::Slot::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal which indicates something interesting.

	G::Slot::Signal0 & connectedSignal() ;
		///< Returns a signal which indicates that the 
		///< connection has been established successfully.

	void releaseForExit() ;
		///< Can be called on program termination when there may
		///< be no TimerList or EventLoop instances.
		///< The client object leaks.

	void cleanupForExit() ;
		///< Can be called on program termination when there may
		///< be no TimerList or EventLoop instances. The client
		///< is destructed so all relevant destructors should 
		///< avoid doing anything with timers or the network 
		///< (possibly even just closing sockets).

private:
	ClientPtr( const ClientPtr & ) ; // not implemented
	void operator=( const ClientPtr & ) ; // not implemented
	void doneSlot( std::string ) ;
	void eventSlot( std::string , std::string ) ;
	void connectedSlot() ;
	void connectSignalsToSlots() ;
	void disconnectSignals() ;

private:
	TClient * m_p ;
	G::Slot::Signal1<std::string> m_done_signal ;
	G::Slot::Signal2<std::string,std::string> m_event_signal ;
	G::Slot::Signal0 m_connected_signal ;
} ;

template <typename TClient>
ClientPtr<TClient>::ClientPtr( TClient * p ) :
	m_p(p)
{
	try
	{
		connectSignalsToSlots() ;
	}
	catch(...)
	{
		// should p->doDelete() here
		throw ;
	}
}

template <typename TClient>
ClientPtr<TClient>::~ClientPtr()
{
	if( m_p != nullptr )
	{
		disconnectSignals() ;
		m_p->doDelete(std::string()) ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::releaseForExit()
{
	if( m_p != nullptr )
	{
		disconnectSignals() ;
		m_p = nullptr ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::cleanupForExit()
{
	if( m_p != nullptr )
	{
		disconnectSignals() ;
		TClient * p = m_p ;
		m_p = nullptr ;
		p->doDeleteForExit() ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::connectSignalsToSlots()
{
	if( m_p != nullptr )
	{
		m_p->doneSignal().connect( G::Slot::slot(*this,&ClientPtr::doneSlot) ) ;
		m_p->eventSignal().connect( G::Slot::slot(*this,&ClientPtr::eventSlot) ) ;
		m_p->connectedSignal().connect( G::Slot::slot(*this,&ClientPtr::connectedSlot) ) ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::reset( TClient * p )
{
	disconnectSignals() ;

	TClient * old = m_p ;
	m_p = p ;
	if( old != nullptr )
	{
		old->doDelete( std::string() ) ;
	}

	connectSignalsToSlots() ;
}

template <typename TClient>
G::Slot::Signal1<std::string> & ClientPtr<TClient>::doneSignal()
{
	return m_done_signal ;
}

template <typename TClient>
G::Slot::Signal2<std::string,std::string> & ClientPtr<TClient>::eventSignal()
{
	return m_event_signal ;
}

template <typename TClient>
G::Slot::Signal0 & ClientPtr<TClient>::connectedSignal()
{
	return m_connected_signal ;
}

template <typename TClient>
void ClientPtr<TClient>::doneSlot( std::string reason )
{
	G_ASSERT( m_p != nullptr ) ;
	disconnectSignals() ;
	m_p = nullptr ;
	m_done_signal.emit( reason ) ;
}

template <typename TClient>
void ClientPtr<TClient>::disconnectSignals()
{
	if( m_p != nullptr )
	{
		m_p->doneSignal().disconnect() ;
		m_p->eventSignal().disconnect() ;
		m_p->connectedSignal().disconnect() ;
	}
}

template <typename TClient>
void ClientPtr<TClient>::connectedSlot()
{
	G_ASSERT( m_p != nullptr ) ;
	m_connected_signal.emit() ;
}

template <typename TClient>
void ClientPtr<TClient>::eventSlot( std::string s1 , std::string s2 )
{
	m_event_signal.emit( s1 , s2 ) ;
}

template <typename TClient>
TClient * ClientPtr<TClient>::get()
{
	return m_p ;
}

template <typename TClient>
bool ClientPtr<TClient>::busy() const
{
	return m_p != nullptr ;
}

template <typename TClient>
TClient * ClientPtr<TClient>::operator->()
{
	if( m_p == nullptr )
		throw InvalidState() ;
	return m_p ;
}

template <typename TClient>
const TClient * ClientPtr<TClient>::operator->() const
{
	if( m_p == nullptr )
		throw InvalidState() ;
	return m_p ;
}

}

#endif

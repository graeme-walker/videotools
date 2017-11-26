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
/// \file gvclientholder.h
///

#ifndef GV_CLIENT_HOLDER__H
#define GV_CLIENT_HOLDER__H

#include "gdef.h"
#include "gnet.h"
#include "gtimer.h"
#include "geventloop.h"
#include "gclientptr.h"
#include "glog.h"

namespace Gv
{

/// \class Gv::ClientHolder
/// Manages a network client object so that the network client is 
/// automatically re-created from a factory when it finishes, or 
/// alternatively the event loop is quit()ed.
/// 
template <typename TFactory, typename TClient>
class ClientHolder : public GNet::TimerBase
{
public:
	explicit ClientHolder( TFactory & factory , bool once = false , 
		bool (*quit_test_fn)(std::string) = nullptr , unsigned int reconnect_timeout = 1U ) ;
			///< Constructor that creates a network client object from a factory. 
			///< 
			///< The quit-test function should return true if the onDone()
			///< reason is a fatal error rather than a retryable connection
			///< failure. The default quit-test function is one that always 
			///< returns false.
			///< 
			///< If the once flag is true then the onDone() event handler 
			///< quit()s the event loop if the client was previously 
			///< successfully connected. Otherwise the onDone() event 
			///< handler calls the quit-test function. If it returns true 
			///< then the event loop is quit()ed, otherwise the reconnect 
			///< timer is started.

	~ClientHolder() ;
		///< Destructor.

private:
	virtual void onTimeout() override ; // GNet::TimerBase
	virtual void onException( std::exception & ) override ; // GNet::EventExceptionHandler
	void onDone( std::string ) ;
	void onConnected() ;

private:
	TFactory & m_factory ;
	bool m_once ;
	bool m_once_connected ;
	bool (*m_quit_test_fn)(std::string) ;
	unsigned int m_reconnect_timeout ;
	GNet::ClientPtr<TClient> m_ptr ;
} ;

template <typename TFactory, typename TClient>
ClientHolder<TFactory,TClient>::ClientHolder( TFactory & factory , bool once , bool (*quit_test_fn)(std::string) , unsigned int reconnect_timeout ) : 
	m_factory(factory) ,
	m_once(once) ,
	m_once_connected(false) ,
	m_quit_test_fn(quit_test_fn) ,
	m_reconnect_timeout(reconnect_timeout)
{ 
	m_ptr.reset( m_factory.create() ) ; 
	m_ptr.doneSignal().connect( G::Slot::slot(*this,&ClientHolder::onDone) ) ;
	m_ptr.connectedSignal().connect( G::Slot::slot(*this,&ClientHolder::onConnected) ) ;
}

template <typename TFactory, typename TClient>
ClientHolder<TFactory,TClient>::~ClientHolder()
{
	m_ptr.doneSignal().disconnect() ;
	m_ptr.connectedSignal().disconnect() ;
}

template <typename TFactory, typename TClient>
void ClientHolder<TFactory,TClient>::onDone( std::string reason )
{
	G_WARNING( "ClientHolder::onDone: session ended: " << reason ) ;
	if( (m_once&&m_once_connected) || (m_quit_test_fn!=nullptr && m_quit_test_fn(reason)) )
		GNet::EventLoop::instance().quit( reason ) ;
	else
		startTimer( m_reconnect_timeout ) ;
}

template <typename TFactory, typename TClient>
void ClientHolder<TFactory,TClient>::onConnected()
{
	m_once_connected = true ;
}

template <typename TFactory, typename TClient>
void ClientHolder<TFactory,TClient>::onTimeout()
{
	G_LOG( "ClientHolder::onTimeout: reconnecting to " << m_factory.url().summary() ) ;
	m_ptr.reset( m_factory.create() ) ; 
}

template <typename TFactory, typename TClient>
void ClientHolder<TFactory,TClient>::onException( std::exception & )
{
	// exception thrown from onTimeout() when constructing the client -- this
	// should not happen for simple connection errors because GNet::Client 
	// does its connection asynchronously wrt. its constructor
	throw ;
}

}
#endif

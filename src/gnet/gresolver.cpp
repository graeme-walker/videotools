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
// gresolver.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "geventloop.h"
#include "gfutureevent.h"
#include "gtest.h"
#include "gstr.h"
#include "gsleep.h"
#include "gdebug.h"
#include "gassert.h"
#include "glog.h"
#include <utility>
#include <vector>
#include <cstring>
#include <cstdio>

#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

// ==

namespace GNet
{
	class ResolverFuture ;
}

namespace
{
	const char * ipvx( int family )
	{
		return family == AF_UNSPEC ? "ip" : ( family == AF_INET ? "ipv4" : "ipv6" ) ;
	}
}

/// \class GNet::ResolverFuture
/// A 'future' object for asynchronous name resolution, in practice holding 
/// just enough state for a single call to getaddrinfo().
///
/// The run() method can be called from a worker thread and the results 
/// collected by the main thread with get() once the run() has finished. 
/// 
/// Signalling completion from worker thread to main thread is out of scope
/// for this class; see GNet::FutureEvent.
///
/// Eg:
////
//// ResolverFuture f( "example.com" , "smtp" , AF_INET , false ) ;
//// std::thread t( &ResolverFuture::run , f ) ;
//// ...
//// t.join() ;
//// Address a = f.get().first ;
///
class GNet::ResolverFuture
{
public:
	typedef std::pair<Address,std::string> Pair ;
	typedef std::vector<Address> List ;

	ResolverFuture( const std::string & host , const std::string & service , 
		int family , bool dgram , bool for_async_hint = false ) ;
			// Constructor for resolving the given host and service names.

	~ResolverFuture() ;
		// Destructor.

	void run() ;
		// Does the name resolution.

	Pair get() ;
		// Returns the resolved address/name pair.

	void get( List & ) ;
		// Returns the list of resolved addresses.

	bool error() const ;
		// Returns true if name resolution failed or no suitable
		// address was returned. Use after get().

	std::string reason() const ;
		// Returns the reason for the error(). 
		// Precondition: error()

private:
	ResolverFuture( const ResolverFuture & ) ;
	void operator=( const ResolverFuture & ) ;
	std::string failure() const ;
	bool fetch( List & ) const ;
	bool fetch( Pair & ) const ;
	bool failed() const ;
	std::string none() const ;
	std::string ipvx_() const ;

private:
	bool m_numeric_service ;
	int m_socktype ;
	std::string m_host ;
	const char * m_host_p ;
	std::string m_service ;
	const char * m_service_p ;
	int m_family ;
	struct addrinfo m_ai_hint ;
	bool m_test_mode ;
	int m_rc ;
	struct addrinfo * m_ai ;
	std::string m_reason ;
} ;

GNet::ResolverFuture::ResolverFuture( const std::string & host , const std::string & service , int family , 
	bool dgram , bool for_async_hint ) :
		m_numeric_service(false) ,
		m_socktype(dgram?SOCK_DGRAM:SOCK_STREAM) ,
		m_host(host) ,
		m_host_p(m_host.c_str()) ,
		m_service(service) ,
		m_service_p(m_service.c_str()) ,
		m_family(family) ,
		m_test_mode(for_async_hint&&G::Test::enabled("getaddrinfo-slow")) ,
		m_rc(0) ,
		m_ai(nullptr)
{
	m_numeric_service = !service.empty() && G::Str::isNumeric(service) ;
	std::memset( &m_ai_hint , 0 , sizeof(m_ai_hint) ) ;
	m_ai_hint.ai_flags = AI_CANONNAME | 
		( family == AF_UNSPEC ? AI_ADDRCONFIG : 0 ) | 
		( m_numeric_service ? AI_NUMERICSERV : 0 ) ;
	m_ai_hint.ai_family = family ;
	m_ai_hint.ai_socktype = m_socktype ;
}

GNet::ResolverFuture::~ResolverFuture() 
{ 
	if( m_ai ) 
		::freeaddrinfo( m_ai ) ; // documented as "thread-safe"
}

void GNet::ResolverFuture::run()
{
	// worker thread - as simple as possible
	if( m_test_mode ) sleep( 10 ) ;
	m_rc = ::getaddrinfo( m_host_p , m_service_p , &m_ai_hint , &m_ai ) ;
}

std::string GNet::ResolverFuture::failure() const
{
	std::stringstream ss ;
	if( m_numeric_service )
		ss << "no such " << ipvx_() << "host: \"" << m_host << "\"" ;
	else
		ss << "no such " << ipvx_() << "host or service: \"" << m_host << ":" << m_service << "\"" ;
	//ss << " (" << G::Str::lower(gai_strerror(m_rc)) << ")" ; // not portable
	return ss.str() ;
}

std::string GNet::ResolverFuture::ipvx_() const
{
	return m_family == AF_UNSPEC ? std::string() : (ipvx(m_family)+std::string(1U,' ')) ;
}

bool GNet::ResolverFuture::failed() const
{
	return m_rc != 0 || m_ai == nullptr || m_ai->ai_addr == nullptr || m_ai->ai_addrlen == 0 ;
}

std::string GNet::ResolverFuture::none() const
{
	return std::string("no usable addresses returned for \"") + m_host + "\"" ;
}

bool GNet::ResolverFuture::fetch( Pair & pair ) const
{
	// fetch the first valid address/name pair
	for( const struct addrinfo * p = m_ai ; p ; p = p->ai_next )
	{
		if( Address::validData( p->ai_addr , p->ai_addrlen ) )
		{
			Address address( p->ai_addr , p->ai_addrlen ) ;
			std::string name( p->ai_canonname ? p->ai_canonname : "" ) ;
			pair = std::make_pair( address , name ) ;
			return true ;
		}
	}
	return false ;
}

bool GNet::ResolverFuture::fetch( List & list ) const
{
	// fetch all valid addresses
	for( const struct addrinfo * p = m_ai ; p ; p = p->ai_next )
	{
		if( Address::validData( p->ai_addr , p->ai_addrlen ) )
			list.push_back( Address( p->ai_addr , p->ai_addrlen ) ) ;
	}
	return !list.empty() ;
}

void GNet::ResolverFuture::get( List & list )
{
	if( failed() )
		m_reason = failure() ;
	else if( !fetch(list) )
		m_reason = none() ;
}

GNet::ResolverFuture::Pair GNet::ResolverFuture::get()
{
	Pair result( Address::defaultAddress() , std::string() ) ;
	if( failed() )
		m_reason = failure() ;
	else if( !fetch(result) )
		m_reason = none() ;
	return result ;
}

bool GNet::ResolverFuture::error() const
{
	return !m_reason.empty() ;
}

std::string GNet::ResolverFuture::reason() const
{
	return m_reason ;
}

// ==

/// \class GNet::ResolverImp
/// A private "pimple" implementation class used by GNet::Resolver to do
/// asynchronous name resolution. The implementation contains a worker thread using
/// the future/promise pattern.
///
class GNet::ResolverImp : private FutureEventHandler
{
public:
	ResolverImp( Resolver & , EventExceptionHandler & , const Location & ) ;
		// Constructor. 

	virtual ~ResolverImp() ;
		// Destructor. The destructor will block if the worker thread is still busy.

	void disarm( bool do_delete_this ) ;
		// Disables the Resolver::done() callback and optionally enables 
		// 'delete this' instead.

	static void start( ResolverImp * , FutureEvent::handle_type ) ;
		// Static worker-thread function to do name resolution. Calls 
		// ResolverFuture::run() to do the work and then FutureEvent::send() 
		// to signal the main thread. The event plumbing then results in a call 
		// to Resolver::done() on the main thread.

	static size_t count() ;
		// Returns the number of objects.

private:
	virtual void onFutureEvent( unsigned int ) override ; // FutureEventHandler
	virtual void onException( std::exception & ) override ; // EventExceptionHandler

private:
	typedef ResolverFuture::Pair Pair ;
	Resolver * m_resolver ;
	bool m_do_delete_this ;
	EventExceptionHandler & m_event_exception_handler ;
	unique_ptr<FutureEvent> m_future_event ;
	Location m_location ;
	ResolverFuture m_future ;
	G::threading::thread_type m_thread ;
	static size_t m_instance_count ;
} ;

size_t GNet::ResolverImp::m_instance_count = 0U ;

GNet::ResolverImp::ResolverImp( Resolver & resolver , EventExceptionHandler & event_exception_handler , const Location & location ) :
	m_resolver(&resolver) ,
	m_do_delete_this(false) ,
	m_event_exception_handler(event_exception_handler) ,
	m_future_event(new FutureEvent(*this)) ,
	m_location(location) ,
	m_future(location.host(),location.service(),location.family(),location.dgram(),true) ,
	m_thread(ResolverImp::start,this,m_future_event->handle())
{
	m_instance_count++ ;
}

GNet::ResolverImp::~ResolverImp()
{
	if( m_thread.joinable() )
	{
		G_WARNING( "ResolverImp::dtor: waiting for getaddrinfo thread to complete" ) ;
		m_thread.join() ; // blocks if getaddrinfo() still running
	}
	m_instance_count-- ;
}

size_t GNet::ResolverImp::count()
{
	return m_instance_count ;
}

void GNet::ResolverImp::start( ResolverImp * This , FutureEvent::handle_type handle )
{
	// thread function, spawned from ctor and join()ed from dtor
	try
	{
		This->m_future.run() ;
		FutureEvent::send( handle , 0/*payload*/ ) ;
	}
	catch(...) // worker thread outer function
	{
		FutureEvent::send( handle , 1 ) ; // nothrow
	}
}

void GNet::ResolverImp::onFutureEvent( unsigned int e )
{
	G_DEBUG( "GNet::ResolverImp::onFutureEvent: future event: e=" << e << " ptr=" << m_resolver << " dt=" << m_do_delete_this ) ;

	m_thread.join() ; // worker thread is finishing, so no delay here
	if( e != 0 ) 
		throw Resolver::Error( "exception in worker thread" ) ; // should never happen

	Pair result = m_future.get() ;
	if( !m_future.error() )
		m_location.update( result.first , result.second ) ;
	G_DEBUG( "GNet::ResolverImp::onFutureEvent: [" << m_future.reason() << "][" << m_location.displayString() << "]" ) ;

	if( m_resolver != nullptr )
		m_resolver->done( m_future.reason() , m_location ) ;

	if( m_do_delete_this )
		delete this ;
}

void GNet::ResolverImp::disarm( bool do_delete_this )
{
	m_resolver = nullptr ;
	m_do_delete_this = do_delete_this ;
}

void GNet::ResolverImp::onException( std::exception & e )
{
	m_event_exception_handler.onException( e ) ;
}

// ==

GNet::Resolver::Resolver( Resolver::Callback & callback ) :
	m_callback(callback) ,
	m_busy(false)
{
	// lazy imp construction
}

GNet::Resolver::~Resolver()
{
	const size_t sanity_limit = 50U ;
	if( m_busy && ResolverImp::count() < sanity_limit )
	{
		// release the imp to an independent lifetime until its getaddrinfo() completes
		G_ASSERT( m_imp != nullptr ) ;
		G_DEBUG( "GNet::Resolver::dtor: releasing still-busy thread: " << ResolverImp::count() ) ;
		m_imp->disarm( true ) ;
		m_imp.release() ; 
	}
}

std::string GNet::Resolver::resolve( Location & location )
{
	// synchronous resolve
	typedef ResolverFuture::Pair Pair ;
	G_DEBUG( "GNet::Resolver::resolve: resolve request [" << location.displayString() << "] (" << location.family() << ")" ) ;
	ResolverFuture future( location.host() , location.service() , location.family() , location.dgram() ) ;
	future.run() ; // blocks until complete
	Pair result = future.get() ;
	if( future.error() )
	{
		G_DEBUG( "GNet::Resolver::resolve: resolve error [" << future.reason() << "]" ) ;
		return future.reason() ;
	}
	else
	{
		G_DEBUG( "GNet::Resolver::resolve: resolve result [" << result.first.displayString() << "][" << result.second << "]" ) ;
		location.update( result.first , result.second ) ;
		return std::string() ;
	}
}

GNet::Resolver::AddressList GNet::Resolver::resolve( const std::string & host , const std::string & service , int family , bool dgram )
{
	// synchronous resolve
	G_DEBUG( "GNet::Resolver::resolve: resolve-request [" << host << "/" << service << "/" << ipvx(family) << "]" ) ;
	ResolverFuture future( host , service , family , dgram ) ;
	future.run() ;
	AddressList list ;
	future.get( list ) ;
	G_DEBUG( "GNet::Resolver::resolve: resolve result: list of " << list.size() ) ;
	return list ;
}

void GNet::Resolver::start( const Location & location )
{
	// asynchronous resolve
	if( !EventLoop::instance().running() ) throw Error("no event loop") ;
	if( busy() ) throw BusyError() ;
	G_DEBUG( "GNet::Resolver::start: resolve start [" << location.displayString() << "]" ) ;
	m_imp.reset( new ResolverImp(*this,m_callback,location) ) ;
	m_busy = true ;
}

void GNet::Resolver::done( const std::string & error , const Location & location )
{
	// callback from the event loop after worker thread is done
	G_DEBUG( "GNet::Resolver::done: resolve done [" << error << "] [" << location.displayString() << "]" ) ;
	m_busy = false ;
	m_callback.onResolved( error , location ) ;
}

bool GNet::Resolver::busy() const
{
	return m_busy ;
}

bool GNet::Resolver::async()
{
	static bool threading_works = G::threading::works() ;
	if( threading_works )
	{
		return EventLoop::instance().running() ;
	}
	else
	{
		//G_WARNING_ONCE( "GNet::Resolver::async: multi-threading not built-in: using synchronous domain name lookup");
		G_DEBUG( "GNet::Resolver::async: multi-threading not built-in: using synchronous domain name lookup");
		return false ;
	}
}

// ==

GNet::Resolver::Callback::~Callback()
{
}


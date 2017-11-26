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
/// \file gfutureevent.h
///

#ifndef G_FUTURE_EVENT__H
#define G_FUTURE_EVENT__H

#include "gdef.h"
#include "gnet.h"
#include "gexception.h"
#include "geventhandler.h"

namespace GNet
{
	class FutureEvent ;
	class FutureEventHandler ;
	class FutureEventImp ;
}

/// \class GNet::FutureEvent
/// An object that hooks into the event loop and calls back to the client 
/// code with a small unsigned integer payload. The trigger function send() 
/// is typically called from a "future/promise" worker thread just before 
/// it finishes.
/// 
/// Eg:
/// \code
/// struct Foo : private FutureEventHandler
/// {
///  Foo() ;
///  void onFutureEvent( unsigned int result ) ;
///  void run( FutureEvent::handle_type ) ;
///  FutureEvent m_future_event ;
///  std::thread m_thread ;
/// }
/// Foo::Foo() : m_thread(&Foo::run,this,m_future_event.handle())
/// {
/// }
/// void Foo::run( FutureEvent::handle_type h )
/// {
///   ... // do blocking work
///   FutureEvent::send( h , 123 ) ;
/// }
/// \endcode
/// 
class GNet::FutureEvent
{
public:
	G_EXCEPTION( Error , "FutureEvent error" ) ;
	typedef HWND handle_type ;

	explicit FutureEvent( FutureEventHandler & ) ;
		///< Constructor.

	~FutureEvent() ;
		///< Destructor.

	handle_type handle() const ;
		///< Returns a handle that can be passed between threads
		///< and used in send().

	static bool send( handle_type handle , unsigned int payload ) g__noexcept ;
		///< Pokes the event payload into the main event loop so that
		///< the callback is called once the stack is unwound. 
		///< Returns true on success.

private:
	FutureEvent( const FutureEvent & ) ;
	void operator=( const FutureEvent & ) ;

private:
	friend class FutureEventImp ;
	unique_ptr<FutureEventImp> m_imp ;
} ;

/// \class GNet::FutureEventHandler
/// A callback interface for GNet::FutureEvent.
/// 
class GNet::FutureEventHandler : public EventExceptionHandler
{
public:
	virtual void onFutureEvent( unsigned int ) = 0 ;
		///< Callback function that delivers the payload value
		///< from FutureEvent::send().

protected:
	virtual ~FutureEventHandler() ;
		///< Destructor.

private:
	void operator=( const FutureEventHandler & ) ;
} ;

#endif

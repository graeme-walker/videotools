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
/// \file gxeventloop.h
///

#ifndef GX_EVENT_LOOP_H
#define GX_EVENT_LOOP_H

#include "gdef.h"
#include "gxdef.h"
#include "gxwindowmap.h"
#include <time.h> // timeval
#include <sys/time.h> // gettimeofday()

namespace GX
{
	class EventLoop ;
	class Display ;
}

namespace GX
{
	struct Timeval /// A thin wrapper for 'struct timeval' providing relational operators etc.
	{
		Timeval() { ::gettimeofday(&m_timeval,nullptr) ; }
		explicit Timeval( int ) { reset() ; }
		explicit Timeval( const struct timeval & tv ) : m_timeval(tv) {}
		void reset() { m_timeval.tv_sec = 0 ; m_timeval.tv_usec = 0 ; }
		bool is_set() const { return m_timeval.tv_sec != 0 ; }
		bool operator<( const Timeval & other ) const
		{ 
			return 
				m_timeval.tv_sec < other.m_timeval.tv_sec || 
				( m_timeval.tv_sec == other.m_timeval.tv_sec && m_timeval.tv_usec < other.m_timeval.tv_usec ) ;
		}
		bool operator==( const Timeval & other ) const
		{
			return m_timeval.tv_sec == other.m_timeval.tv_sec && m_timeval.tv_usec == other.m_timeval.tv_usec ;
		}
		bool operator>( const Timeval & other ) const
		{
			return !((*this)==other) && other < (*this) ;
		}
		struct timeval m_timeval ;
	} ;
	inline Timeval operator-( const Timeval & big , const Timeval & small )
	{
		Timeval result ;
		const bool carry = small.m_timeval.tv_usec > big.m_timeval.tv_usec ;
		result.m_timeval.tv_usec = (carry?1000000L:0L) + big.m_timeval.tv_usec ; 
		result.m_timeval.tv_usec -= small.m_timeval.tv_usec ;
		result.m_timeval.tv_sec = big.m_timeval.tv_sec - small.m_timeval.tv_sec - (carry?1:0) ;
		return result ;
	}
	inline Timeval operator+( const Timeval & base , unsigned long usec )
	{
		Timeval result = base ;
		result.m_timeval.tv_usec += ( usec % 1000000UL ) ;
		result.m_timeval.tv_sec += ( usec / 1000000UL ) ;
		bool carry = result.m_timeval.tv_usec > 1000000L ;
		if( carry )
		{
			result.m_timeval.tv_usec -= 1000000L ;
			result.m_timeval.tv_sec++ ;
		}
		return result ;
	}
}

/// \class GX::EventLoop
/// An event-loop class that delivers Xlib 'Display' events to GX::Window 
/// objects via their GX::EventHandler interface. Note that GX::Window 
/// objects register themselves with the GX::WindowMap singleton.
/// 
class GX::EventLoop
{
public:
	explicit EventLoop( Display & ) ;
		///< Constructor. The display reference is kept.

	void run() ;
		///< Runs the event loop.

	void runUntil( int event_type ) ;
		///< Runs the event loop until the given event is received.

	void runOnce() ;
		///< Waits for one event and processes it.

	void runToEmpty() ;
		///< Processes all events in the queue and then returns.

	void runToTimeout() ;
		///< Processes all events until the timer goes off.

	void startTimer( unsigned int milliseconds ) ;
		///< Starts a timer for runToTimeout().

	void handlePendingEvents() ;
		///< Handles all pending events. This is to allow the GX::EventLoop
		///< to be subservient to another event loop; the main event loop 
		///< calls this method when it detects a read event on the display's 
		///< file descriptor.

private:
	EventLoop( const EventLoop & ) ;
	void operator=( const EventLoop & ) ;
	void handle( ::XEvent & ) ;
	void handleImp( ::XEvent & ) ;
	GX::Window & w( ::Window ) ;
	Display & display() ;

private:
	GX::Display & m_display ;
	GX::WindowMap m_window_map ;
	Timeval m_timeout ;
} ;

#endif

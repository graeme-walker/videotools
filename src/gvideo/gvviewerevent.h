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
/// \file gvviewerevent.h
///

#ifndef GV_VIEWEREVENT__H
#define GV_VIEWEREVENT__H

#include "gdef.h"
#include "gpublisher.h"
#include <string>
#include <iostream>

namespace Gv
{
	class ViewerEvent ;
	class ViewerEventMixin ;
	class ViewerEventMixinImp ;
}

/// \class Gv::ViewerEvent
/// A class for receiving and handling events published from a viewer process.
/// 
class Gv::ViewerEvent
{
public:
	enum Type { Up , Down , Drag , Move , Invalid } ;
	struct Info /// A structure representing an interaction event received from a viewer process.
	{
		Type type ;
		int x ;
		int y ;
		int dx ;
		int dy ;
		int x_down ;
		int y_down ;
		bool shift ;
		bool control ;
		Info() ;
		std::string str() const ;
	} ;

	explicit ViewerEvent( const std::string & channel_name = std::string() ) ;
		///< Constructor.

	Info apply( const std::string & ) ;
		///< Parses the event string and maintains an internal button state.

	Info apply( const std::vector<char> & ) ;
		///< Overload taking a vector.

	bool init() ;
		///< Tries to subscribe to the viewer's event channel.
		///< Returns true if newly subscribed.

	int fd() const ;
		///< Returns the subsription file descriptor, or minus one if
		///< not init()isalised.

	bool receive( std::vector<char> & , std::string * type_p = nullptr ) ;
		///< Receives the payload for an event on fd().
		/// \see G::PublisherSubscriber::receive().

	std::string channelName() const ;
		///< Returns the channel name, as passed to the ctor.

private:
	ViewerEvent( const ViewerEvent & ) ;
	void operator=( const ViewerEvent & ) ;
	static int parse( const std::string & event , const std::string & key , int default_ = -1 ) ;

private:
	std::string m_name ;
	G::PublisherSubscription m_channel ;
	bool m_button_down ;
} ;

/// \class Gv::ViewerEventMixin
/// A mixin base-class containing a ViewerEvent object that integrates
/// with the GNet::EventLoop.
/// 
class Gv::ViewerEventMixin
{
public:
	explicit ViewerEventMixin( const std::string & channel ) ;
		///< Constructor.

	virtual ~ViewerEventMixin() ;
		///< Destructor.

	virtual void onViewerEvent( ViewerEvent::Info ) = 0 ;
		///< Callback that delivers an event from the viewer.

private:
	ViewerEventMixin( const ViewerEventMixin & ) ;
	void operator=( const ViewerEventMixin & ) ;

private:
	ViewerEventMixinImp * m_imp ;
} ;

namespace Gv
{
	inline
	std::ostream & operator<<( std::ostream & stream , const ViewerEvent::Info & event )
	{
		return stream << event.str() ;
	}
}

#endif

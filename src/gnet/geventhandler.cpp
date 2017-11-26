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
// geventhandler.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventhandler.h"
#include "geventloop.h"
#include "gexception.h"
#include "gdebug.h"
#include "gassert.h"
#include "gdescriptor.h"
#include "glog.h"
#include <algorithm>

GNet::EventHandler::~EventHandler()
{
}

void GNet::EventHandler::readEvent()
{
	G_DEBUG( "GNet::EventHandler::readEvent: no override" ) ;
}

void GNet::EventHandler::writeEvent()
{
	G_DEBUG( "GNet::EventHandler::writeEvent: no override" ) ;
}

void GNet::EventHandler::exceptionEvent()
{
	throw G::Exception( "socket disconnect event" ) ; // "exception event" is confusing
}

// ===

namespace
{
	struct fdless
	{
		bool operator()( const std::pair<GNet::Descriptor,GNet::EventHandler*> & p1 , const std::pair<GNet::Descriptor,GNet::EventHandler*> & p2 )
		{
			return p1.first < p2.first ;
		}
	} ;
}

GNet::EventHandlerList::EventHandlerList( const std::string & type ) :
	m_type(type) ,
	m_lock(0U) ,
	m_has_garbage(false)
{
}

bool GNet::EventHandlerList::contains( Descriptor fd ) const
{
	return contains(m_list,fd) || (m_lock && contains(m_pending_list,fd)) ;
}

bool GNet::EventHandlerList::contains( const List & list , Descriptor fd )
{
	List::const_iterator p = std::lower_bound( list.begin() , list.end() , List::value_type(fd,nullptr) , fdless() ) ;
	return p != list.end() && (*p).first == fd && (*p).second != nullptr ;
}

GNet::EventHandler * GNet::EventHandlerList::find( Descriptor fd )
{
	EventHandler * p1 = find( m_list , fd ) ;
	EventHandler * p2 = m_lock ? find( m_pending_list , fd ) : nullptr ;
	return p2 ? p2 : p1 ;
}

GNet::EventHandler * GNet::EventHandlerList::find( List & list , Descriptor fd )
{
	List::const_iterator p = std::lower_bound( list.begin() , list.end() , List::value_type(fd,nullptr) , fdless() ) ;
	return ( p != list.end() && (*p).first == fd ) ? (*p).second : nullptr ;
}

void GNet::EventHandlerList::add( Descriptor fd , EventHandler * handler )
{
	G_ASSERT( fd.valid() && handler != nullptr ) ; if( !fd.valid() || handler == nullptr ) return ;
	G_DEBUG( "GNet::EventHandlerList::add: " << m_type << "-list: " << "adding " << fd ) ;

	add( m_lock?m_pending_list:m_list , fd , handler ) ;
}

void GNet::EventHandlerList::add( List & list , Descriptor fd , EventHandler * handler )
{
	typedef std::pair<List::iterator,List::iterator> Range ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd,nullptr) , fdless() ) ;
	if( range.first == range.second )
		list.insert( range.second , List::value_type(fd,handler) ) ;
	else
		(*range.first).second = handler ;
}

void GNet::EventHandlerList::remove( Descriptor fd )
{
	G_DEBUG( "GNet::EventHandlerList::remove: " << m_type << "-list: " << "removing " << fd ) ;
	if( m_lock )
	{
		if( disable(m_list,fd) ) m_has_garbage = true ;
		disable( m_pending_list , fd ) ;
	}
	else
	{
		remove( m_list , fd ) ;
	}
}

bool GNet::EventHandlerList::disable( List & list , Descriptor fd )
{
	typedef std::pair<List::iterator,List::iterator> Range ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd,nullptr) , fdless() ) ;
	const bool found = range.first != range.second ;
	if( found )
		(*range.first).second = nullptr ;
	return found ;
}

bool GNet::EventHandlerList::remove( List & list , Descriptor fd )
{
	typedef std::pair<List::iterator,List::iterator> Range ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd,nullptr) , fdless() ) ;
	const bool found = range.first != range.second ;
	if( found )
		list.erase( range.first ) ;
	return found ;
}

void GNet::EventHandlerList::lock()
{
	m_lock++ ;
}

void GNet::EventHandlerList::unlock()
{
	G_ASSERT( m_lock != 0U ) ;
	m_lock-- ;
	if( m_lock == 0U )
	{
		commit() ;
		if( m_has_garbage )
			collectGarbage() ;
	}
}

void GNet::EventHandlerList::commit()
{
	const List::iterator end = m_pending_list.end() ;
	for( List::iterator p = m_pending_list.begin() ; p != end ; ++p )
	{
		if( (*p).second != nullptr )
			add( m_list , (*p).first , (*p).second ) ;
	}
	m_pending_list.clear() ;
}

void GNet::EventHandlerList::collectGarbage()
{
	if( false ) // probably not worthwhile
	{
		const List::iterator end = m_list.end() ;
		for( List::iterator p = m_list.begin() ; p != end ; )
		{
			if( (*p).second == nullptr )
				p = m_list.erase( p ) ;
			else
				++p ;
		}
		m_has_garbage = false ;
	}
}

// ==

GNet::EventExceptionHandler::~EventExceptionHandler()
{
}

/// \file geventhandler.cpp

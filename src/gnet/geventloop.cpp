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
// geventloop.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventloop.h"
#include "gdebug.h"
#include "gassert.h"

GNet::EventLoop * GNet::EventLoop::m_this = nullptr ;

GNet::EventLoop::EventLoop()
{
	if( m_this == nullptr )
		m_this = this ;
	else
		G_WARNING( "GNet::EventLoop::ctor: multiple instances" ) ;
}

GNet::EventLoop::~EventLoop()
{
	if( m_this == this )
		m_this = nullptr ;
}

GNet::EventLoop & GNet::EventLoop::instance()
{
	if( m_this == nullptr ) 
		throw NoInstance() ;
	G_ASSERT( m_this != nullptr ) ;
	return *m_this ;
}

bool GNet::EventLoop::exists()
{
	return m_this != nullptr ;
}

void GNet::EventLoop::stop( const G::SignalSafe & signal_safe )
{
	if( m_this != nullptr )
		m_this->quit( signal_safe ) ;
}

/// \file geventloop.cpp

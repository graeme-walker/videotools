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
/// \file gmonitor.h
///

#ifndef G_GNET_MONITOR_H
#define G_GNET_MONITOR_H

#include "gdef.h"
#include "gslot.h"
#include "gnet.h"
#include "gconnection.h"
#include <iostream>
#include <utility>

namespace GNet
{
	class Monitor ;
	class MonitorImp ;
}

/// \class GNet::Monitor
/// A singleton for monitoring GNet::SimpleClient and GNet::ServerPeer 
/// connections and for storing their TLS certificates.
/// \see GNet::SimpleClient, GNet::ServerPeer
/// 
class GNet::Monitor
{
public:
	Monitor() ;
		///< Default constructor.

	virtual ~Monitor() ;
		///< Destructor.

	static Monitor * instance() ;
		///< Returns the singleton pointer. Returns null if none.

	void addClient( const Connection & simple_client ) ;
		///< Adds a client connection.

	void removeClient( const Connection & simple_client ) ;
		///< Removes a client connection.

	void addServerPeer( const Connection & server_peer ) ;
		///< Adds a server connection.

	void removeServerPeer( const Connection & server_peer ) ;
		///< Removes a server connection.

	void report( std::ostream & stream ,
		const std::string & line_prefix = std::string() ,
		const std::string & eol = std::string("\n") ) const ;
			///< Reports itself onto a stream.

	void report( G::StringArray & out ) const ;
			///< Reports itself into a three-column table.

	std::pair<std::string,bool> findCertificate( const std::string & certificate ) ;
		///< Returns a short id for the given certificate and a boolean
		///< flag to indicate if it is a new certificate id that has 
		///< not been returned before.

	G::Slot::Signal2<std::string,std::string> & signal() ;
		///< Provides a callback signal which can be connect()ed
		///< to a slot.
		///< 
		///< the first is "in" or "out", and the second is 
		///< "start" or "stop".

private:
	Monitor( const Monitor & ) ;
	void operator=( const Monitor & ) ;

private:
	static Monitor * & pthis() ;
	MonitorImp * m_imp ;
	G::Slot::Signal2<std::string,std::string> m_signal ;
} ;

#endif

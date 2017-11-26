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
/// \file gvmulticast.h
///

#ifndef GV_MULTICAST__H
#define GV_MULTICAST__H

#include "gdef.h"
#include "gnet.h"
#include <utility>
#include <string>

namespace Gv
{
	class Multicast ;
}

/// \class Gv::Multicast
/// A class to do setsocketopt() on a socket file descriptor, with 
/// IP_ADD_MEMBERSHIP.
/// 
class Gv::Multicast
{
public:
	static void join( SOCKET , const std::string & ) ;
		///< Joins the socket to the multicast group. IPv4 only. Throws on error.

	static void join( SOCKET , const in_addr & multiaddr ) ;
		///< Joins the socket to the multicast group. IPv4 only. Throws on error.

private:
	Multicast() ;
	static std::pair<int,int> join_imp( SOCKET , const in_addr & ) ;
} ;

#endif

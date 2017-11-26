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
// gvmulticast.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gstr.h"
#include "gvmulticast.h"
#include "gprocess.h"
#include <stdexcept>
#include <sstream>

#ifdef G_UNIX

void Gv::Multicast::join( SOCKET fd , const std::string & group )
{
	in_addr a ; 
	a.s_addr = inet_addr( group.c_str() ) ;

	std::pair<int,int> pair = join_imp( fd , a ) ;
	if( pair.first < 0 )
	{
		std::ostringstream ss ;
		ss << "multicast join error: " << group << ": " << G::Process::strerror(pair.second) ;
		throw std::runtime_error( ss.str() ) ;
	}
}

void Gv::Multicast::join( SOCKET fd , const in_addr & multiaddr )
{
	std::pair<int,int> pair = join_imp( fd , multiaddr ) ;
	if( pair.first < 0 )
		throw std::runtime_error( "multicast join error: " + G::Process::strerror(pair.second) ) ;
}

std::pair<int,int> Gv::Multicast::join_imp( SOCKET fd , const in_addr & multiaddr )
{
	in_addr any ; 
	any.s_addr = INADDR_ANY ;

	#if GCONFIG_HAVE_IP_MREQN
		struct ip_mreqn m ; // man ip(7)
		m.imr_multiaddr = multiaddr ;
		m.imr_address = any ;
		m.imr_ifindex = 0 ;
	#else
		struct ip_mreq
		{
			struct in_addr imr_multiaddr ;
			struct in_addr imr_interface ;
		} ;
		struct ip_mreq m ; // man ip(4)
		m.imr_multiaddr = multiaddr ;
		m.imr_interface = any ;
		// also set 'multicast=YES' in OpenBSD /etc/rc.conf
	#endif

	int rc = setsockopt( fd , IPPROTO_IP , IP_ADD_MEMBERSHIP , reinterpret_cast<void*>(&m) , sizeof(m) ) ;
	int e = errno ;
	return std::make_pair(rc,e) ;
}




#else
void Gv::Multicast::join( SOCKET fd , const std::string & group )
{
	throw std::runtime_error( "multicast not implemented" ) ;
}
void Gv::Multicast::join( SOCKET , const in_addr & )
{
	throw std::runtime_error( "multicast not implemented" ) ;
}
#endif
/// \file gvmulticast.cpp

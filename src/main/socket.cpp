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
// socket.cpp
//
// Sends a command string to a local-domain or UDP socket. 
//
// This can be used with the `vt-fileplayer` and `vt-recorder` programs when using 
// their `--command-socket` option.
//
// The socket is a local-domain socket by default, but a UDP network socket 
// if the socket name looks like a transport address or if it starts 
// with `udp://`.
// 
// The standard "netcat" utility (`nc`) can also be used to send messages to
// UDP and local-domain sockets.
//
// usage: socket <socket> <command-word> [<command-word> ...]
//

#include "gdef.h"
#include "gnet.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gvcommandsocket.h"
#include "garg.h"
#include "ggetopt.h"
#include <string>
#include <iostream>
#include <stdexcept>

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::GetOpt opt( arg , 
			"V!version!show the program version and exit!!0!!3" "|"
			"h!help!show this help!!0!!3" "|"
		) ;
		std::string args_help = "<socket> <command-word> [<command-word> ...]" ;
		Gv::Startup startup( opt , args_help , opt.args().c() > 2U ) ;
		try
		{
			std::string socket_path( opt.args().v(1) ) ;

			std::string command ;
			const char * sep = "" ;
			for( unsigned int i = 2 ; i < opt.args().c() ; i++ , sep = " " )
			{
				command.append( sep ) ;
				command.append( opt.args().v(i) ) ;
			}

			Gv::CommandSocket socket ;
			socket.connect( socket_path ) ;
			ssize_t n = ::send( socket.fd() , command.data() , command.size() , 0 ) ;
			if( n != static_cast<ssize_t>(command.size()) )
				throw std::runtime_error( "send failed" ) ;
		}
		catch( std::exception & e )
		{
			startup.report( arg.prefix() , e ) ;
			throw ;
		}
		return EXIT_SUCCESS ;
	}
	catch( Gv::Exit & e )
	{
		return e.value() ;
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
	}
	return EXIT_FAILURE ;
}

/// \file socket.cpp

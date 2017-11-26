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
// channel.cpp
//
// A tool for working with publication channels.
//
// The following sub-commands are available: "list" lists all local
// publication channels; "info" displays diagnostic information for 
// the channel; "read" reads one message from the channel; "peek"
// reads the last-available message from the channel without
// waiting; "purge" clears subscriber slots if subscribers have 
// failed without cleaning up; and "delete" deletes the channel,
// in case a publisher fails without cleaning up.
//
// usage: channel <command> <channel> ...
//

#include "gdef.h"
#include "gnet.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gpublisher.h"
#include "gcleanup.h"
#include "gdirectory.h"
#include "groot.h"
#include "garg.h"
#include "grimagetype.h"
#include "ggetopt.h"
#include "gassert.h"
#include "gstr.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <unistd.h> // isatty()

namespace
{
	struct ForeignName
	{
		bool operator()( const std::string & name )
		{
			std::string prefix = "vt-" ;
			if( prefix.find("__") == 0U ) prefix = "vt-" ;
			return name.find(prefix) != 0U ;
		}
	} ;
}

std::ostream & open( std::ofstream & file , std::string filename , std::ostream & default_ )
{
	if( filename.empty() )
	{
		return default_ ;
	}
	else
	{
		file.open( filename.c_str() ) ;
		if( !file.good() )
			throw std::runtime_error( "cannot open file [" + filename + "] for output" ) ;
		return file ;
	}
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;

		// allow options to follow the sub-command
		if( arg.c() > 3U && arg.v(1U) != "list" &&
			!arg.v(1U).empty() && arg.v(1U).at(0U) != '-' && 
			!arg.v(2U).empty() && arg.v(2U).at(0U) == '-' )
		{
			// move the sub-command to just before the channel name
			G::StringArray a = arg.array() ;
			std::rotate( a.begin()+1 , a.begin()+2 , a.begin()+(a.size()-1U) ) ;
			arg = G::Arg( a ) ;
		}

		G::GetOpt opt( arg , 
			"V!version!show the program version and exit!!0!!3" "|"
			"h!help!show this help!!0!!3" "|"
			"d!debug!with debug logging! if compiled in!0!!2" "|"
			"a!all!include info for unused slots!!0!!1" "|"
			"o!out!output filename!!1!filename!1" "|"
			"q!quiet!fewer warnings!!0!!1" "|"
		) ;
		std::string args_help = "{list|info|purge|delete|peek|read} <channel>" ;

		Gv::Startup startup( opt , args_help , opt.args().c() > 2U || (
			opt.args().c() == 2U && opt.args().v(1U) == "list" ) ) ;
		try
		{
			std::string command( opt.args().v(1) ) ;
			std::string channel_name = opt.args().c() > 2U ? opt.args().v(2) : std::string() ;
			std::string out_filename = opt.value( "out" ) ;
			bool all = opt.contains( "all" ) ;
			bool quiet = opt.contains( "quiet" ) ;

			std::ofstream out_file ;
			std::ostream & out = open( out_file , out_filename , std::cout ) ;

			if( command == "info" )
			{
				G::Item info = G::Publisher::info( channel_name , all ) ;
				info.out( out , 1 ) ;
				out << "\n" ;
			}
			else if( command == "read" || command == "peek" )
			{
				G::PublisherSubscription channel( channel_name ) ;
				std::vector<char> buffer ;
				std::string type ;
				G::EpochTime time( 0 ) ;
				bool peek = command == "peek" ;
				bool ok = peek ? channel.peek(buffer,&type,&time) : channel.receive(buffer,&type,&time) ;
				if( ok )
				{
					if( !out_filename.empty() ) 
					{
						std::cout << type << "\n" ;
						if( time.s ) 
							std::cout << "time=" << time << "\n" ;
					}

					if( Gr::ImageType(type).isRaw() )
					{
						Gr::ImageType image_type( type ) ;
						out
							<< (image_type.channels()==1?"P5":"P6") << "\n" 
							<< image_type.dx() << " " << image_type.dy() << "\n" 
							<< "255\n" ;
					}

					out.write( &buffer[0] , buffer.size() ) ;
					if( isatty(1) && buffer.size() && buffer[buffer.size()-1] != '\n' ) out << "\n" ;
					out.flush() ;
					if( !out.good() )
						throw std::runtime_error( "failed to write to file" ) ;
				}
				else
					throw std::runtime_error( std::string("channel read failed") + (peek?": no data":"") ) ;
			}
			else if( command == "purge" )
			{
				G::Publisher::purge( channel_name ) ;
			}
			else if( command == "delete" )
			{
				std::string e = G::Publisher::delete_( channel_name ) ;
				if( !e.empty() )
					throw std::runtime_error( "shared memory deletion failed for channel [" + channel_name + "]: " + e ) ;
			}
			else if( command == "list" && channel_name.empty() ) // was && G::is_linux()
			{
				G::StringArray others ;
				G::StringArray list = G::Publisher::list( &others ) ;

				// more heuristics to warn about only our shared memory channels
				others.erase( std::remove_if(others.begin(),others.end(),ForeignName()) , others.end() ) ;

				if( !quiet && !others.empty() )
					std::cerr << "warning: unreadable: " << G::Str::join(",",others) << std::endl ;

				for( G::StringArray::iterator p = list.begin() ; p != list.end() ; ++p )
				{
					out << *p << std::endl ;
				}
			}
			else if( command == "list" )
			{
				throw std::runtime_error( "do not use a channel name with \"list\"" ) ;
			}
			else
			{
				throw std::runtime_error( "invalid command [" + command + "]" ) ;
			}
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

/// \file channel.cpp

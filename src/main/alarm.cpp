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
// alarm.cpp
//
// Monitors a watcher's output channel and runs an external command on 
// each motion-detection event.
//
// For example, an "alarm" process can be used to generate an alarm sound 
// when motion detection is triggered, by running an external command 
// like "aplay".
//
// If the `--run-as` option is given then the external command runs with
// that account's user-id and group-id if possible.
//
// If you need pipes, backticks, file redirection, etc. in your external 
// command then add the "--shell" option. Awkward shell escapes can 
// avoided by using "--url-decode".
//
// usage: alarm [<options>] <event-channel> [<command> [<arg> ...]]
//

#include "gdef.h"
#include "gnewprocess.h"
#include "gidentity.h"
#include "gpublisher.h"
#include "gurl.h"
#include "garg.h"
#include "ggetopt.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gitem.h"
#include "gstr.h"
#include <vector>
#include <exception>
#include <sstream>

struct EventInfo
{
	explicit EventInfo( const std::string & s ) ;
	bool valid() const { return m_valid ; }
	unsigned int count() const { return m_count ; }
	bool m_valid ;
	unsigned int m_count ;
} ;

EventInfo::EventInfo( const std::string & s ) :
	m_valid(false) ,
	m_count(0U)
{
	try
	{
		const G::Item item = G::Item::parse( s ) ;
		std::string app = item["app"]() ;
		std::string event = item["event"]() ;
		std::string count = item["count"]() ;
		unsigned int repeat = G::Str::toUInt( item["repeat"]() , "0" ) ;
		if( app == "watcher" && event == "changes" && repeat == 0 && !count.empty() && G::Str::isUInt(count) )
		{
			m_count = G::Str::toUInt( count ) ;
			m_valid = true ;
		}
	}
	catch( std::exception & e )
	{
		G_LOG( "EventInfo::ctor: event parsing failure for [" << G::Str::printable(s) << "]: " << e.what() ) ;
	}
}

static std::pair<int,std::string> runImp( G::Identity run_as , G::StringArray command )
{
	G_LOG( "run: running [" << G::Str::join("][",command) << "]" ) ;
	if( command.empty() ) throw std::runtime_error( "invalid empty command" ) ;
	G::Path exe = command.at(0U) ;
	command.erase( command.begin() ) ;
	G::NewProcess child( exe , command , 2 , false , false , run_as , false , 127 , "exec failed: __""strerror""__" ) ;
	int rc = child.wait().run().get() ;
	std::string output = child.wait().output() ;
	output = G::Str::printable( G::Str::head( G::Str::trimmed(output,G::Str::ws()) , "\n" , false ) ) ;
	return std::make_pair( rc , output ) ;
}

static std::pair<int,std::string> runImp( bool url_decode , bool shell , G::Identity run_as , G::StringArray command )
{
	if( shell )
	{
		G::StringArray shell_command ;
		shell_command.push_back( "/bin/sh" ) ;
		shell_command.push_back( "-c" ) ;
		shell_command.push_back( url_decode ? G::Url::decode(G::Str::join("+",command)) : G::Str::join(" ",command) ) ;
		return runImp( run_as , shell_command ) ;
	}
	else if( url_decode )
	{
		return runImp( run_as , G::Str::splitIntoTokens(G::Url::decode(G::Str::join("+",command))," ") ) ;
	}
	else
	{
		return runImp( run_as , command ) ;
	}
}

static void run( bool url_decode , bool shell , G::Identity run_as , const G::StringArray & command )
{
	std::pair<int,std::string> result = runImp( url_decode , shell , run_as , command ) ;

	std::ostringstream ss ;
	ss << "command exit " << result.first ;
	if( !result.second.empty() )
		ss << ": output=[" << result.second << "]" ;
	std::string s = ss.str() ;
	if( result.first != 0 || !result.second.empty() )
		G_WARNING( "alarm: " << s ) ;
	else
		G_LOG( "alarm: " << s ) ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::GetOpt opt( arg , 
			"V!version!show the program version and exit!!0!!3" "|"
			"h!help!show this help!!0!!3" "|"
			"d!debug!with debug logging! if compiled in!0!!2" "|"
			"y!syslog!log to syslog!!0!!2" "|"
			"L!log-time!add a timestamp to log output!!0!!2" "|"
			"b!daemon!run in the background!!0!!2" "|"
			"u!user!user to switch to when idle if run as root!!1!username!2" "|"
			"U!run-as!user to switch to when running the command!!1!username!2" "|"
			"P!pid-file!write process id to file!!1!path!2" "|"
			"v!verbose!verbose logging!!0!!1" "|"
			"A!threshold!alarm threshold! in pixels per image!1!pixels!1" "|"
			"S!shell!use '/bin/sh -c'!!0!!1" "|"
			"E!url-decode!treat the command as url-encoded!!0!!1" "|"
			"!test!!!0!!0" "|"
		) ;
		std::string args_help = "<event-channel> [<command> [<arg> ...]]" ;
		Gv::Startup startup( opt , args_help , opt.args().c() >= 2U ) ;
		try
		{
			unsigned int threshold = G::Str::toUInt( opt.value("threshold","0") ) ;
			std::string channel_name = opt.args().v(1U) ;
			G::Identity run_as = opt.contains("run-as") ? G::Identity(opt.value("run-as")) : G::Identity::invalid() ;
			bool shell = opt.contains("shell") ;
			bool url_decode = opt.contains("url-decode") ;

			G::StringArray command = opt.args().array() ; 
			command.erase( command.begin() , command.begin()+2U ) ; // remove program name and channel name

			if( !shell && !url_decode )
			{
				G::Path exe = command.at( 0U ) ;
				if( exe == G::Path() )
					throw std::runtime_error( "invalid empty command" ) ;
				if( exe.isRelative() && opt.contains("daemon") )
					G_WARNING( "alarm: relative path [" << exe << "] used with \"--daemon\": beware of cwd changes" ) ;
			}
 
			G::PublisherSubscription channel( channel_name ) ;
	
			startup.start() ;

			if( opt.contains("test") )
			{
				std::pair<int,std::string> result = runImp( url_decode , shell , run_as , command ) ;
				std::cout << "exit=" << result.first << " stderr=[" << result.second << "]\n" ;
				throw Gv::Exit( 0 ) ;
			}

			std::vector<char> buffer ;
			std::string type ;
			while( channel.receive( buffer , &type ) )
			{
				if( type == "application/json" )
				{
					std::string event( &buffer[0] , buffer.size() ) ;
					G_LOG( "alarm: " << G::Str::printable(event) ) ;

					EventInfo event_info( event ) ;
					if( event_info.valid() && event_info.count() >= threshold )
					{
						run( url_decode , shell , run_as , command ) ;
					}
				}
			}
			G_LOG( "alarm: publisher on channel [" << channel_name << "] has gone away" ) ;
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
/// \file alarm.cpp

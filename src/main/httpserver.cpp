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
// httpserver.cpp
//
// Reads video from one or more publication channels and makes it available 
// over the network using HTTP.
//
// The HTTP client can either request individual images ("client-pull") or
// receive a continuous JPEG video stream delivered as a 
// `multipart/x-mixed-replace` content type ("server-push").
//
// For server-push add a `streaming=1` parameter to the URL, eg. 
// `http://localhost:80/?streaming=1`. This should always be used when the 
// client is the `vt-httpclient` program, but be careful when using a browser 
// because not all of them support `multipart/x-mixed-replace`, and some of 
// them go beserk with "save-as" dialog boxes.
// 
// The client-pull model works best with a browser that uses JavaScript code
// to generate a continuous stream of GET requests, typically using the 
// "Fetch" API.
//
// For browsers that do not have JavaScript or support for `multipart/x-mixed-replace`
// a `refresh=1` URL parameter can be used so that the server sends
// back static images with a HTTP "Refresh" header, causing the browser to 
// repeat the GET requests at intervals. This can also be made the default
// behaviour using the `--refresh` command-line option on the server.
//
// A `scale` URL parameter can be used to reduce the size of the images, and
// a `type` parameter can be used to request an image format (`jpeg`, `pnm`, or
// `raw`, with `jpeg` as the default). The type can also be `any`, which means 
// that no image conversion or validation is performed; whatever data is 
// currently in the publication channel is served up.
//
// Static files can be served up by using the `--dir` option. Normally any file
// in the directory will be served up, but a restricted set of files can be 
// specified by using the `--file` option. The `--file-type` option can be used
// to give the files' content-type, by using the filename and the content-type 
// separated by a colon, eg. `--file-type=data.json:application/json`. The default 
// file that is served up for an empty path in the url can be modified by the 
// `--default` option, eg. `--default=main.html`.
//
// One or more channel names should normally be specified on the command-line
// using the `--channel` option. Clients can select a channel by name by
// using URL paths with an underscore prefix (eg. `_foo`, `_bar`); or by index 
// by using URL paths like `_0`, `_1`, etc. 
//
// If `--channel=*` is specified then any valid channel name can be used in 
// the URL path. This also enables the special `/__` url that returns channel 
// information as a JSON structure.
//
// The `--gateway=<host>` option can be used to integrate static web pages
// with UDP command sockets; HTTP requests with a `send` parameter of the
// form `<port> <message>` are sent as UDP messages to the `--gateway` IP 
// address and the specified port.
//
// Usage: httpserver [<options>] [--port <port>] <channel> [<channel> ...]
//

#include "gdef.h"
#include "gnet.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gvhttpserverpeer.h"
#include "glocation.h"
#include "gresolver.h"
#include "groot.h"
#include "gpath.h"
#include "gfile.h"
#include "garg.h"
#include "gstr.h"
#include "gstrings.h"
#include "geventloop.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "ggetopt.h"
#include "gassert.h"
#include "gdebug.h"
#include <iostream>
#include <stdexcept>
#include <exception>
#include <algorithm>
#include <sstream>
#include <limits>
#include <sys/stat.h>

class HttpServer : public GNet::Server
{
public:
	HttpServer( Gv::HttpServerSources & , const Gv::HttpServerResources & , Gv::HttpServerConfig config , const GNet::Address & ) ;
		// Constructor.

private:
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) override ;
	virtual void onException( std::exception & ) override ;

private:
	Gv::HttpServerSources & m_sources ;
	const Gv::HttpServerResources & m_resources ;
	Gv::HttpServerConfig m_config ;
} ;

// ==

HttpServer::HttpServer( Gv::HttpServerSources & sources , const Gv::HttpServerResources & resources , 
	Gv::HttpServerConfig config , const GNet::Address & listening_address ) :
		GNet::Server(listening_address) ,
		m_sources(sources) ,
		m_resources(resources) ,
		m_config(config)
{
	G_LOG( "HttpServer::ctor: httpserver listening on " << listening_address.displayString() ) ;
}

GNet::ServerPeer * HttpServer::newPeer( GNet::Server::PeerInfo peer_info )
{
	G_LOG( "HttpServer::newPeer: new connection from " << peer_info.m_address.displayString() ) ;
	return new Gv::HttpServerPeer( peer_info , m_sources , m_resources , m_config ) ;
}

void HttpServer::onException( std::exception & )
{
	throw ;
}

// ==

int main( int argc , char ** argv )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::GetOpt opt( arg , 
			"V!version!show the program version and exit!!0!!3" "|"
			"h!help!show this help!!0!!3" "|"
			"d!debug!with debug logging! if compiled in!0!!2" "|"
			"L!log-time!add a timestamp to log output!!0!!2" "|"
			"v!verbose!verbose logging!!0!!2" "|"
			"b!daemon!run in the background!!0!!2" "|"
			"u!user!user to switch to when idle if run as root!!1!username!2" "|"
			"y!syslog!log to syslog!!0!!2" "|"
			"P!pid-file!write process id to file!!1!path!2" "|"
			"l!port!listening port!!1!port!1" "|"
			"T!timeout!network idle timeout! (default 60s)!1!seconds!1" "|"
			"I!data-timeout!initial image timeout! (default 3s)!1!seconds!1" "|"
			"J!repeat-timeout!image repeat timeout! (default 1000ms)!1!ms!1" "|"
			"a!address!listening address!!1!ip!1" "|"  
			"D!dir!serve up static files!!1!dir!1" "|"
			"f!file!allow serving the named file !!2!filename!1" "|"  
			"t!file-type!define the content-type of the the named file !!2!fname:type!1" "|"  
			"i!default!define the default resource!!1!fname-or-channel!1" "|"  
			"c!channel!serve up named video channel!!2!name!1" "|"  
			"U!gateway!act as a http-post-to-udp-command-line gateway!!1!udp-host!1" "|"  
			"R!refresh!add a refresh header to http responses! by default!1!seconds!1" "|"
		) ;
		std::string args_help = "" ;
		Gv::Startup startup( opt , args_help , opt.args().c() >= 1U ) ;
		try
		{
			unsigned int idle_timeout = G::Str::toUInt(opt.value("timeout"),"60") ;
			unsigned int data_timeout = G::Str::toUInt(opt.value("data-timeout"),"3") ;
			unsigned int repeat_timeout_ms = G::Str::toUInt(opt.value("repeat-timeout"),"1000") ;
			unsigned int port = G::Str::toUInt(opt.value("port"),"80") ;
			std::string ip_address = opt.contains("address") ? opt.value("address") : std::string() ;
			std::string gateway_str = opt.value("gateway","") ;
			G::Path dir = opt.contains("dir") ? G::Path(opt.value("dir")) : G::Path() ;
			G::StringArray file_list = G::Str::splitIntoTokens( opt.value("file","") , "," ) ;
			G::StringArray file_types = G::Str::splitIntoTokens( opt.value("file-type","") , "," ) ;
			std::string default_resource = opt.value("default","") ;
			unsigned int channel_reopen_timeout = 4U ;
			G::StringArray channel_list = G::Str::splitIntoTokens( opt.value("channel","") , "," ) ;
			unsigned int refresh = opt.contains("refresh") ? G::Str::toUInt(opt.value("refresh")) : 0U ;
			unsigned int more_verbose = opt.count("verbose") > 1U ;
			GNet::Address bind_address = 
				ip_address.empty() ? 
					GNet::Address(GNet::Address::Family::ipv4(),port) : 
					GNet::Address(ip_address,port) ;

			if( channel_list.empty() && dir == G::Path() )
				throw std::runtime_error( "please use \"--dir\" or \"--channel\"" ) ;

			if( dir == G::Path() && (!file_list.empty() || !file_types.empty()) )
				throw std::runtime_error( "please use \"--dir\" with \"--file\" or \"--file-type\"" ) ;

			if( dir != G::Path() && !G::File::isDirectory(dir) )
				throw std::runtime_error( "invalid \"--dir\" directory" ) ;

			if( dir != G::Path() && G::Path(dir).isRelative() && opt.contains("daemon") )
				throw std::runtime_error( "please use an absolute \"--dir\" path with \"--daemon\"" ) ;

			// build up the configuration resource map
			//
			Gv::HttpServerResources resources ;
			if( dir != G::Path() )
			{
				resources.setDirectory( dir ) ;
				if( file_list.empty() )
				{
					resources.addFileAny() ;
				}
				for( G::StringArray::iterator file_p = file_list.begin() ; file_p != file_list.end() ; ++file_p )
				{
					std::string reason = resources.addFile( *file_p ) ;
					if( !reason.empty() )
						throw std::runtime_error( "invalid \"--file\" path: " + reason ) ;
				}
				for( G::StringArray::iterator file_type_p = file_types.begin() ; file_type_p != file_types.end() ; ++file_type_p )
				{
					std::string fname = G::Str::head( *file_type_p , ":" , false ) ;
					std::string type = G::Str::tail( *file_type_p , ":" ) ;
					if( fname.empty() || type.empty() ) throw std::runtime_error( "invalid \"--file-type\" value" ) ;
					resources.addFileType( fname , type ) ;
				}
			}

			// add channels to the configuration resource map
			//
			for( G::StringArray::iterator channel_p = channel_list.begin() ; channel_p != channel_list.end() ; ++channel_p )
			{
				if( *channel_p == "*" )
					resources.addChannelAny() ;
				else
					resources.addChannel( *channel_p ) ;
			}

			// define the default resource
			{
				std::string reason ;
				if( !default_resource.empty() )
					reason = resources.setDefault( default_resource ) ;
				else if( !channel_list.empty() )
					resources.setDefault( "_0" ) ;
				else if( !file_list.empty() )
					resources.setDefault( file_list.at(0) ) ;
				else
					resources.setDefault( "index.html" ) ; // may 404

				if( !reason.empty() )
					G_WARNING( "httpserver: invalid default resource: " << reason ) ;
			}

			// prepare the gateway address
			//
			GNet::Address gateway = GNet::Address::defaultAddress() ;
			if( !gateway_str.empty() )
			{
				GNet::Location location( gateway_str , "0" ) ;
				std::string e = GNet::Resolver::resolve( location ) ;
				if( !e.empty() )
					throw std::runtime_error( e ) ;
				gateway = location.address() ;
			}

			// finalise the configuration
			//
			Gv::HttpServerConfig config ;
			resources.log() ;
			config.init( idle_timeout , data_timeout , G::EpochTime(0U,repeat_timeout_ms) , refresh , gateway , more_verbose ) ;

			// event loop singletons
			//
			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			// run the server event loop
			//
			Gr::ImageConverter converter ;
			Gv::HttpServerSources sources( converter , resources , channel_reopen_timeout ) ;
			HttpServer server( sources , resources , config , bind_address ) ;
			startup.start() ;
			event_loop->run() ;
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

/// \file httpserver.cpp

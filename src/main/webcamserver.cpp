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
// webcamserver.cpp
//
// Serves up video images from a local webcam or video capture device over HTTP.
//
// The HTTP client can either request individual images ("client-pull") or
// receive a continuous JPEG video stream delivered as a 
// `multipart/x-mixed-replace` content type ("server-push"). Refer to the
// `vt-httpserver` program for more information.
//
// This program is useful for running on satellite machines in the local network 
// that have webcams attached; a central monitoring machine can run `vt-httpclient`
// to pull the video stream across the network and onto a local publication 
// channel. 
//
// Use `--device=test` or `--device=demo` for testing without an attached 
// camera, or on linux use the "vivi" kernel module (`sudo modprobe vivi`).
// Device-specific configuration options can be added after the device
// name, using a semi-colon separator.
//
// The server will normally poll for the webcam device to come on-line, although
// `--retry=0` can be used to disable this behaviour. When the device goes 
// off-line again the program will go back to polling, or it will terminate if the
// `--once` option is used.
//
// The `--tz` option can be used with `--caption` to adjust the timezone for the
// caption timestamp. It is typically a negative number for western longitudes 
// in order to show local time.
//
// Usage: webcamserver [<options>] [--port <port>] [--device <video-device>]
//

#include "gdef.h"
#include "gnet.h"
#include "gvcamera.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gvhttpserverpeer.h"
#include "grjpeg.h"
#include "garg.h"
#include "gstr.h"
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
#include <limits>

class WebcamServer : public GNet::Server
{
public:
	WebcamServer( Gv::HttpServerSources & , Gv::HttpServerConfig config , const GNet::Address & ) ;

private:
	virtual GNet::ServerPeer * newPeer( GNet::Server::PeerInfo ) override ;
	virtual void onException( std::exception & ) override ;

private:
	Gv::HttpServerSources & m_sources ;
	Gv::HttpServerResources m_resources ; // empty
	Gv::HttpServerConfig m_config ;
} ;

WebcamServer::WebcamServer( Gv::HttpServerSources & sources , Gv::HttpServerConfig config , 
	const GNet::Address & listening_address ) :
		GNet::Server(listening_address) ,
		m_sources(sources) ,
		m_config(config)
{
	G_LOG( "WebcamServer::ctor: webcamserver listening on " << listening_address.displayString() ) ;
}

GNet::ServerPeer * WebcamServer::newPeer( GNet::Server::PeerInfo peer_info )
{
	G_LOG( "Server::newPeer: new connection from " << peer_info.m_address.displayString() ) ;
	return new Gv::HttpServerPeer( peer_info , m_sources , m_resources , m_config ) ;
}

void WebcamServer::onException( std::exception & )
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
			"y!syslog!log to syslog!!0!!2" "|"
			"L!log-time!add a timestamp to log output!!0!!2" "|"
			"b!daemon!run in the background!!0!!2" "|"
			"u!user!user to switch to when idle if run as root!!1!username!2" "|"
			"P!pid-file!write process id to file!!1!path!2" "|"
			"v!verbose!verbose logging!!0!!1" "|"
			"l!port!listening port!!1!port!1" "|"
			"p!caption!add a timestamp caption!!0!!1" "|"
			"C!caption-text!defines the caption text! in ascii, with %t for a timestamp!1!text!1" "|"
			"T!timeout!network idle timeout! (default 60s)!1!seconds!1" "|"
			"I!data-timeout!initial image timeout! (default 3s)!1!seconds!1" "|"
			"J!repeat-timeout!image repeat timeout! (default 1000ms)!1!ms!1" "|"
			"a!address!listening address!!1!ip!1" "|"  
			"z!tz!timezone offset! for the caption!1!hours!1" "|"
			"D!device!webcam video device! (eg. /dev/video0)!1!device!1" "|"
			"k!refresh!add a refresh header to http responses! by default!1!seconds!1" "|"
			"R!retry!poll for the webcam device to appear! (default 1s)!1!timeout!1" "|"
			"O!once!exit if the webcam device disappears!!0!!1" "|"
		) ;
		std::string args_help = "" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 1U ) ;
		try
		{
			std::string dev_name = opt.value("device","/dev/video0") ;
			unsigned int port = G::Str::toUInt(opt.value("port"),"80") ;
			unsigned int idle_timeout = G::Str::toUInt(opt.value("timeout"),"60") ;
			unsigned int data_timeout = G::Str::toUInt(opt.value("data-timeout"),"3") ;
			unsigned int repeat_timeout_ms = G::Str::toUInt(opt.value("repeat-timeout"),"1000") ;
			std::string ip_address = opt.value("address","") ;
			std::string caption = opt.contains("caption") ? opt.value("caption-text","%t") : std::string() ;
			std::string caption_tz = opt.value("tz","0") ;
			unsigned int refresh = G::Str::toUInt(opt.value("refresh"),"0") ;
			unsigned int retry = G::Str::toUInt(opt.value("retry","1")) ;
			bool once = opt.contains("once") ;

			GNet::Address bind_address = 
				ip_address.empty() ? 
					GNet::Address(GNet::Address::Family::ipv4(),port) : 
					GNet::Address(ip_address,port) ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			std::string dev_config = G::Str::tail( dev_name , dev_name.find(';') , std::string() ) ;
			dev_name = G::Str::head( dev_name , dev_name.find(';') , dev_name ) ;
			Gr::ImageConverter converter ;
			Gv::Camera camera( converter , dev_name , dev_config , 
				once , retry!=0U/*lazy-open*/ , std::max(1U,retry) , caption , Gv::Timezone(caption_tz) ) ;
			Gv::HttpServerSources sources( converter , camera ) ;

			// by default serve up individual images with a one second refresh header
			Gv::HttpServerConfig server_config ;
			server_config.init( idle_timeout , data_timeout , G::EpochTime(0U,repeat_timeout_ms) , refresh ,
				Gr::Jpeg::available()?"jpeg":"pnm" ) ;

			WebcamServer server( sources , server_config , bind_address ) ;
	
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

/// \file webcamserver.cpp

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
// httpclient.cpp
// 
// Pulls video from a remote HTTP server or network camera and sends it to a 
// publication channel or a viewer window.
//
// Uses repeated GET requests on a persistent connection, with each GET request
// separated by a configurable time delay (`--request-timeout`). A watchdog 
// timer tears down the connection if no data is received for some time and 
// then the connection is retried. Watchdog timeouts and peer disconnections 
// result in connection retries; connection failures will terminate the 
// program only if using `--retry=0`.
//
// The server should normally return a mime type of `image/jpeg`, or 
// preferably `multipart/x-mixed-replace` with `image/jpeg` sub-parts.
// In the latter case the client only needs to make one GET request and
// the server determines the frame rate.
// 
// When getting video from a network camera the exact form of the URL to use 
// will depend on the camera, so please refer to its documentation or
// search the internet for help.
//
// To get video from a remote webcam use the `vt-webcamserver` on the remote
// maching and use `vt-httpclient` to pull it across the network.
//
// As a convenience, a URL of `vt://<address>` can be used instead of `http://<address>`
// and this is equivalent to `http://<address>/_?streaming=1&type=any`.
//
// usage: httpclient [--viewer] [--channel=<channel>] <url>
//

#include "gdef.h"
#include "gnet.h"
#include "gvimageoutput.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "grimagetype.h"
#include "ghttpclientprotocol.h"
#include "gurl.h"
#include "gtimerlist.h"
#include "gclient.h"
#include "gvclientholder.h"
#include "geventloop.h"
#include "geventhandler.h"
#include "glocation.h"
#include "gresolver.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "gpath.h"
#include "glogoutput.h"
#include "gassert.h"
#include <cstdlib>
#include <exception>
#include <iostream>

class HttpClient ;

/// \class Output
/// Holds an output channel.
///
class Output : private GNet::EventExceptionHandler
{
public:
	Output( const std::string & title , unsigned int viewer_scale , const std::string & channel , const G::Url & ) ;
	void process( std::string , const char * , size_t ) ;

private:
	virtual void onException( std::exception & ) ;

private:
	Gv::ImageOutput m_image_output ;
} ;

/// \class HttpClientFactory 
/// A factory class for HttpClient objects.
///
class HttpClientFactory 
{
public:
	HttpClientFactory( Output & , const GNet::Location & , const G::Url & , 
		unsigned int connection_timeout , unsigned int watchdog_timeout , unsigned int request_timeout_ms ) ;
	HttpClient * create() ;
	const G::Url & url() const ;

private:
	Output & m_output ;
	GNet::Location m_location ;
	G::Url m_url ;
	unsigned int m_connection_timeout ;
	unsigned int m_watchdog_timeout ;
	unsigned int m_request_timeout_ms ;
} ;

/// \class HttpClient
/// A network client using http.
///
class HttpClient : public GNet::Client , private GNet::HttpClientProtocol::Callback
{
public:
	HttpClient( Output & , const GNet::Location & , const G::Url & url , unsigned int watchdog_timeout ,
		unsigned int request_timeout_ms ) ;
	virtual ~HttpClient() ;

private:
	virtual void onData( const char* , std::string::size_type ) override ; // dont use GNet::Client line buffering
	virtual void onSecure( const std::string & ) override ; // GNet::SocketProtocolSink
	virtual void onConnect() override ; // GNet::SimpleClient
	virtual void onSendComplete() override ; // GNet::SimpleClient
	virtual void onDelete( const std::string & ) override ; // GNet::HeapClient
	virtual bool onReceive( const std::string & ) override ; // GNet::Client

private:
	virtual void sendHttpRequest( const std::string & ) override ;
	virtual void onHttpBody( const std::string & , const std::string & , const char * , size_t ) override ;
	void onWatchdogTimeout() ;
	void onRequestTimeout() ;

private:
	Output & m_output ;
	GNet::HttpClientProtocol m_http ;
	unsigned int m_watchdog_timeout ;
	GNet::Timer<HttpClient> m_watchdog_timer ;
	unsigned int m_request_timeout_ms ;
	GNet::Timer<HttpClient> m_request_timer ;
} ;

// ==

HttpClient::HttpClient( Output & output , const GNet::Location & location , const G::Url & url , 
	unsigned int watchdog_timeout , unsigned int request_timeout_ms ) :
		GNet::Client(location) ,
		m_output(output) ,
		m_http(*this,url) ,
		m_watchdog_timeout(watchdog_timeout) ,
		m_watchdog_timer(*this,&HttpClient::onWatchdogTimeout,*static_cast<GNet::EventHandler*>(this)) ,
		m_request_timeout_ms(request_timeout_ms) ,
		m_request_timer(*this,&HttpClient::onRequestTimeout,*static_cast<GNet::EventHandler*>(this))
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::onConnect()
{
	G_LOG( "HttpClient::onConnect: connected to " << peerAddress().second.displayString() ) ;
	m_http.start() ;
}

void HttpClient::onData( const char * p , std::string::size_type n )
{
	// this overrides the GNet::Client input line buffering

	if( m_watchdog_timeout != 0U )
		m_watchdog_timer.startTimer( m_watchdog_timeout ) ;

	try
	{
		m_http.apply( p , n ) ;
	}
	catch( GNet::HttpClientProtocol::Retry & )
	{
		// absorb the exception and rely on the watchdog to retry
		G_WARNING( "HttpClient::onData: service unavailable" ) ;
	}
}

void HttpClient::onHttpBody( const std::string & outer_type , const std::string & type , const char * p , size_t n )
{
	m_output.process( type , p , n ) ;
	if( outer_type.empty() ) // not multipart
		m_request_timer.startTimer( 0 , m_request_timeout_ms*1000U ) ; // assume connection is http keep-alive
}

void HttpClient::onRequestTimeout()
{
	m_http.start() ; 
}

void HttpClient::sendHttpRequest( const std::string & get )
{
	send( get ) ; // GNet::SimpleClient
}

void HttpClient::onWatchdogTimeout()
{
	// throw to terminate this client -- the client-holder will re-connect as required
	G_WARNING( "HttpClient::onWatchdogTimeout: no data received" ) ;
	throw std::runtime_error( "watchdog timeout" ) ;
}

void HttpClient::onSecure( const std::string & )
{
	// no-op -- never gets here
}

void HttpClient::onDelete( const std::string & )
{
	// no-op
}

bool HttpClient::onReceive( const std::string & )
{
	// no-op -- never gets here -- input line buffering has been by-passed
	return true ;
}

void HttpClient::onSendComplete()
{
	// no-op
}

// ==

HttpClientFactory::HttpClientFactory( Output & output , const GNet::Location & location , const G::Url & url , 
	unsigned int connection_timeout , unsigned int watchdog_timeout , unsigned int request_timeout_ms ) :
		m_output(output),
		m_location(location) ,
		m_url(url) ,
		m_connection_timeout(connection_timeout) ,
		m_watchdog_timeout(watchdog_timeout) ,
		m_request_timeout_ms(request_timeout_ms)
{
}

HttpClient * HttpClientFactory::create()
{
	return new HttpClient( m_output , m_location , m_url , m_watchdog_timeout , m_request_timeout_ms ) ;
}

const G::Url & HttpClientFactory::url() const 
{
	return m_url ;
}

// ==

Output::Output( const std::string & viewer_title , unsigned int viewer_scale , 
	const std::string & channel , const G::Url & url ) :
		m_image_output(*this)
{
	if( viewer_scale != 0 )
		m_image_output.startViewer( viewer_title , viewer_scale ) ;

	if( !channel.empty() )
	{
		G::Item info = G::Item::map() ;
		info.add( "url" , url.summary() ) ; // no passwords
		m_image_output.startPublisher( channel , info ) ;
	}
}

void Output::process( std::string type_str , const char * p , size_t n )
{
	G_LOG( "Output::process: received [" << type_str << "]: " << n << " bytes" ) ;

	Gr::ImageType type( type_str ) ;
	if( !type.isRaw() )
	{
		// get the full image type by examining the payload
		type = Gr::ImageType( p , n ) ;
	}

	if( type.valid() )
		m_image_output.send( p , n , type ) ;
	else
		G_DEBUG( "Output::process: invalid image" ) ;
}

void Output::onException( std::exception & )
{
	throw ; // viewer-ping failure
}

// ==

static bool quitAlways( std::string )
{
	return true ;
}

static bool quitTest( std::string reason )
{
	if( reason.find("watchdog timeout") == 0U ) return false ;
	if( reason.find("peer disconnected") == 0U ) return false ; // GNet::SocketProtocol::ReadError, GNet::SocketProtocol::SendError
	if( reason.find("connect failure") == 0U ) return false ; // GNet::SimpleClient::ConnectError
	return true ;
}

int main( int argc, char ** argv )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		Gv::Startup::sanitise( argc , argv ) ;
		G::GetOpt opt( arg , 
			"V!version!show the program version and exit!!0!!3" "|"
			"h!help!show this help!!0!!3" "|"
			"d!debug!with debug logging! if compiled in!0!!2" "|"
			"L!log-time!add a timestamp to log output!!0!!2" "|"
			"y!syslog!log to syslog!!0!!2" "|"
			"b!daemon!run in the background!!0!!2" "|"
			"u!user!user to switch to when idle if run as root!!1!username!2" "|"
			"P!pid-file!write process id to file!!1!path!2" "|"
			"v!verbose!verbose logging!!0!!1" "|"
			"w!viewer!run a viewer!!0!!1" "|"
			"s!scale!reduce the viewer image size!!1!divisor!1" "|"
			"t!viewer-title!viewer window title!!1!title!1" "|"
			"W!watchdog-timeout!watchdog timeout! (default 20s)!1!seconds!1" "|"
			"T!request-timeout!time between http get requests! (default 1ms)!1!ms!1" "|"
			"c!channel!publish to the named channel!!1!channel!1" "|"
			"x!connection-timeout!connection timeout!!1!seconds!1" "|"
			"R!retry!retry failed connections! or zero to exit (default 1s)!1!seconds!1" "|"
			"O!once!exit when the first connection closes!!0!!1" "|"
		) ;
		std::string args_help = "<http-url>" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 2U ) ;
		try
		{
			std::string url_in = opt.args().v(1U) ;
			unsigned int viewer_scale = opt.contains("scale") ? G::Str::toUInt(opt.value("scale")) : 1U ;
			bool viewer = opt.contains("viewer") ;
			std::string viewer_title = opt.contains("viewer-title") ? opt.value("viewer-title") : arg.prefix() ;
			std::string channel = opt.value("channel",std::string()) ;
			unsigned int watchdog_timeout = G::Str::toUInt( opt.value("watchdog-timeout","20") ) ;
			unsigned int request_timeout_ms = G::Str::toUInt( opt.value("request-timeout","1") ) ;
			unsigned int retry = G::Str::toUInt( opt.value("retry","1") ) ;
			unsigned int connection_timeout = G::Str::toUInt( opt.value("connection-timeout","30") ) ;
			bool once = opt.contains("once") ;

			// default to "--viewer"
			if( channel.empty() )
				viewer = true ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			std::string appid = "vt" ; if( appid.find("__") == 0U ) appid = "vt" ;
			if( url_in.find(appid+"://") == 0U )
			{
				G::Url url( url_in ) ;
				url_in = G::Url::join( "http" , url.authorisation() , url.address() , 
					"/_0" , G::Str::join("&",url.parameters(),"streaming=1&type=any") ) ;
			}

			std::cout << url_in << "\n" ;
			G::Url url( url_in ) ;
			if( url.protocol() != "http" ) 
				throw std::runtime_error( "invalid protocol in the url: expecting \"http://\" or \""+appid+"://\"" ) ;

			GNet::Location location( url.host() , url.port("80") ) ;

			Output output( viewer_title , viewer?viewer_scale:0U , channel , url ) ;
			HttpClientFactory factory( output , location , url , connection_timeout , watchdog_timeout , request_timeout_ms ) ;
			Gv::ClientHolder<HttpClientFactory,HttpClient> client_holder( factory , once , retry==0U?quitAlways:quitTest , retry ) ;

			startup.start() ;
			std::string quit = event_loop->run() ;
			if( !quit.empty() )
				std::cout << arg.prefix() << ": " << quit << std::endl ;
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


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
// rtspclient.cpp
//
// Issues RTSP commands to control network cameras that send out their video 
// over RTP with H.264 encoding (RTP/AVP).
//
// The RTSP protocol acts as a "remote control" for video devices, so an RTSP
// client is used to ask a camera to send its RTP video stream back to a 
// particular pair of UDP ports where the RTP server is listening.
//
// The RTSP client also reports on what video format the camera will send, 
// encoded as a "fmtp" string. This information can help the RTP server 
// decode the video stream. Use the `--format-file` option to make it
// available to the RTP server.
//
// The RTSP protocol is similar to HTTP, and it uses the same URL syntax, with
// `rtsp://` instead of `http://` , eg. `rtsp://example.com/channel/1`. The 
// format of right hand side of the URL, following the network address, will 
// depend on the camera, so refer to its documentation.
//
// The camera should send its video stream for as long as the `vt-rtspclient`
// program keeps its session open; once it terminates then the video stream from
// the camera will stop.
//
// usage: rtspclient [--port=<port-port>] [<rtsp-url>]
//
// Eg.
//       $ ./rtspclient --port=8000-8001 rtsp://example.com/channel/1
//

#include "gdef.h"
#include "gnet.h"
#include "ghttpclientparser.h"
#include "ghttpclientprotocol.h"
#include "groot.h"
#include "gfile.h"
#include "gurl.h"
#include "garg.h"
#include "ggetopt.h"
#include "gclient.h"
#include "gvclientholder.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "glogoutput.h"
#include "gexception.h"
#include "gstr.h"
#include "gassert.h"
#include "gstrings.h"
#include "gvsdp.h"
#include "gdebug.h"
#include <vector>

class RtspClient ;

/// \class RtspClientFactory
/// A factory for RtspClient objects.
///
class RtspClientFactory
{
public:
	RtspClientFactory( const GNet::Location & , const G::Url & , unsigned int connection_timeout , 
		std::string client_port_range , bool use_log_s , const std::string & format_file , 
		GNet::Address multicast_address ) ;
			// Constructor.

	RtspClient * create() ;
		// Returns a new RtspClient instance on the heap.

	const G::Url & url() const ;
		// Returns a reference to the url.

private:
	GNet::Location m_location ;
	G::Url m_url ;
	unsigned int m_connection_timeout ;
	std::string m_client_port_range ;
	bool m_use_log_s ;
	std::string m_format_file ;
	GNet::Address m_multicast_address ;
} ;

/// \class RtspClient
/// An interface for RTSP clients.
/// \see RFC-2326
///
class RtspClient : public GNet::Client
{
public:
	G_EXCEPTION( InvalidResponse , "invalid response" ) ;
	G_EXCEPTION( InvalidContent , "invalid response content" ) ;

	RtspClient( const GNet::Location & , const G::Url & url , unsigned int connection_timeout , 
		std::string client_port_range , bool use_log_s , const std::string & format_file ,
		GNet::Address multicast_address = GNet::Address::defaultAddress() , 
		const std::string & teardown_session_id = std::string() ) ;
			// Constructor. Objects should be created on the heap so that they can manage 
			// their own lifetime.
			//
			// After construction the RtspClient initiates a dialog with the remote RTSP
			// server to set up a session in which the server sends a media resource
			// stream back to the client box, targeting the specified ports.
			//
			// Session teardown is not normally a concern as the server will detect when 
			// the RTSP connection goes away.

private:
	virtual void onConnect() override ;
	virtual void onSecure( const std::string & peer_certificate ) override ;
	virtual void onSendComplete() override ;
	virtual void onDelete( const std::string & ) override ;
	virtual bool onReceive( const std::string & ) override ;

private:
	void onResponse() ;
	void sendRequest( const std::string & command , const std::string & resource = std::string() , 
		const std::string & header = std::string() ) ;
	bool have( const std::string & item ) const ;
	std::string value( const std::string & item ) const ;
	unsigned int value( const std::string & item , int ) const ;
	unsigned int contentLength() const ;
	static void formatSave( const std::string & format , const std::string & format_file ) ;
	static void formatLog( const std::string & format , bool ) ;

private:
	enum State { Connecting , SentOptions , SentDescribe , SentSetup , SentPlay , SentTeardown } ;
	G::Url m_url ;
	std::string m_transport ;
	std::string m_teardown_session_id ;
	bool m_describe_only ;
	State m_state ;
	bool m_authentication_supplied ;
	int m_seq ;
	unique_ptr<Gv::Sdp> m_session ;
	GNet::HttpClientParser m_parser ;
	bool m_use_log_s ;
	std::string m_format_file ;
} ;

// ==

RtspClientFactory::RtspClientFactory( const GNet::Location & location , const G::Url & url , 
	unsigned int connection_timeout , std::string client_port_range , bool use_log_s , 
	const std::string & format_file , GNet::Address multicast_address ) :
		m_location(location) ,
		m_url(url) ,
		m_connection_timeout(connection_timeout) ,
		m_client_port_range(client_port_range) ,
		m_use_log_s(use_log_s) ,
		m_format_file(format_file) ,
		m_multicast_address(multicast_address)
{
}

RtspClient * RtspClientFactory::create()
{
	return new RtspClient( m_location , m_url , m_connection_timeout , m_client_port_range , m_use_log_s , m_format_file , m_multicast_address ) ;
}

const G::Url & RtspClientFactory::url() const
{
	return m_url ;
}

// ==

RtspClient::RtspClient( const GNet::Location & location , const G::Url & url , unsigned int connection_timeout , 
	std::string client_port_range , bool use_log_s , const std::string & format_file , 
	GNet::Address multicast_address , const std::string & teardown_session_id ) :
		GNet::Client(location,connection_timeout,0,0,"\r\n") ,
		m_url(url) ,
		m_teardown_session_id(teardown_session_id) ,
		m_describe_only(false) ,
		m_state(Connecting) ,
		m_authentication_supplied(false) ,
		m_seq(1) ,
		m_parser("RTSP") ,
		m_use_log_s(use_log_s) ,
		m_format_file(format_file)
{
	// allow a single port number rather than a range
	if( client_port_range.find("-") == std::string::npos && !client_port_range.empty() && G::Str::isUInt(client_port_range) )
	{
		unsigned int port = G::Str::toUInt( client_port_range ) ;
		client_port_range.append( "-" ) ;
		client_port_range.append( G::Str::fromUInt(port+1U) ) ;
	}

	if( multicast_address == GNet::Address::defaultAddress() )
		m_transport = "Transport: RTP/AVP;unicast;client_port=" + client_port_range ;
	else
		m_transport = std::string() + "Transport: RTP/AVP;multicast;" +
			"destination=" + multicast_address.hostPartString() + ";" +
			"client_port=" + client_port_range + ";ttl=1" ;

	G_LOG( "RtspClient::RtspClient: transport=[" << m_transport << "]" ) ;
}

void RtspClient::onConnect()
{
	G_DEBUG( "RtspClient::onConnect: " << peerAddress().first << " " << peerAddress().second.displayString() ) ;
	G_ASSERT( connected() ) ;
	G_ASSERT( m_state == Connecting ) ;

	if( !m_teardown_session_id.empty() )
	{
		sendRequest( "TEARDOWN" , std::string() , "Session: " + m_teardown_session_id ) ;
		m_state = SentTeardown ;
	}
	else
	{
		sendRequest( "OPTIONS" ) ;
		m_state = SentOptions ;
	}
}

void RtspClient::sendRequest( const std::string & command , const std::string & url_in , const std::string & header )
{
	std::string url = url_in.empty() ? 
		std::string("rtsp://"+peerAddress().second.displayString()+G::Url(m_url).path()) : 
		url_in ;

	const char * eol = "\r\n" ;
	std::ostringstream ss ;
	ss 
		<< command << " " << url << " RTSP/1.0" << eol
		<< "CSeq: " << m_seq++ << eol ;

	if( !header.empty() )
		ss << header << eol ;

	ss << eol ;

	G_DEBUG( "RtspClient::sendRequest: sending [" << G::Str::printable(ss.str()) << "]" ) ;
	send( ss.str() ) ;
}

void RtspClient::onSecure( const std::string & )
{
	G_DEBUG( "RtspClient::onSecure" ) ;
}

void RtspClient::onSendComplete()
{
	G_DEBUG( "RtspClient::onSendComplete" ) ;
}

void RtspClient::onDelete( const std::string & s )
{
	G_DEBUG( "RtspClient::onDelete" ) ;
}

bool RtspClient::have( const std::string & item ) const
{
	return !m_parser.header(item).empty() ;
}

std::string RtspClient::value( const std::string & item ) const
{
	return m_parser.header( item ) ;
}

unsigned int RtspClient::value( const std::string & item , int ) const
{
	return G::Str::toUInt( value(item) ) ;
}

unsigned int RtspClient::contentLength() const
{
	return m_parser.bodySize() ;
}

bool RtspClient::onReceive( const std::string & line )
{
	G_DEBUG( "RtspClient::onReceive: [" << G::Str::printable(line) << "]" ) ;
	m_parser.apply( line.data() , line.size() ) ;
	m_parser.apply( "\r\n" , 2U ) ;
	if( m_parser.gotBody() )
	{
		onResponse() ;
		m_parser.clear() ;
	}
	return true ;
}

void RtspClient::formatLog( const std::string & format , bool log_s )
{
	if( !format.empty() )
	{
		if( log_s )
			G_LOG_S( "RtspClient::onResponse: format=[" << format << "]" ) ;
		else
			G_LOG( "RtspClient::onResponse: format=[" << format << "]" ) ;
	}
}

void RtspClient::formatSave( const std::string & format , const std::string & format_file )
{
	if( format.empty() || format_file.empty() )
		return ;

	if( format_file == "-" )
	{
		std::cout << format << std::endl ;
	}
	else
	{
		bool ok = false ;
		{
			std::ofstream file_tmp ;
			{
				G::Root claim_root ;
				file_tmp.open( (format_file+".tmp").c_str() ) ;
			}
			if( file_tmp.good() )
			{
				file_tmp << format << std::endl ;
				file_tmp.close() ;
				if( file_tmp.good() )
				{
					G::Root claim_root ;
					ok = G::File::rename( format_file+".tmp" , format_file , G::File::NoThrow() ) ;
				}
			}
		}
		if( !ok )
		{
			std::ofstream file ;
			{
				G::Root claim_root ;
				file.open( format_file.c_str() ) ;
			}
			file << format << std::endl ;
			file.close() ;
			if( !file.good() )
				throw std::runtime_error( "failed to write format file: " + format_file ) ;
		}
	}
}

void RtspClient::onResponse()
{
	G_DEBUG( "RtspClient::onResponse: got " << m_parser.headerCount() << " header lines" ) ;

	if( m_parser.headerCount() <= 2U ) 
		throw InvalidResponse( "too few lines" ) ;

	bool authentication_required = m_state == SentDescribe && m_parser.responseUnauthorised() ;
	if( authentication_required && m_authentication_supplied )
		throw InvalidResponse( "authentication required" ) ;

	if( !authentication_required && !m_parser.responseOk() ) 
		throw InvalidResponse( "not ok" ) ;

	if( m_parser.headerValue("CSeq") != (m_seq-1) ) 
		throw InvalidResponse( "out of sequence" ) ;

	G_DEBUG( "RtspClient::onResponse: response parsed ok" ) ;

	if( m_state == SentOptions )
	{
		sendRequest( "DESCRIBE" ) ;
		m_state = SentDescribe ;
	}
	else if( m_state == SentDescribe && authentication_required )
	{
		sendRequest( "DESCRIBE" , std::string() , GNet::HttpClientProtocol::authorisation(m_parser,m_url,"DESCRIBE",true) ) ;
		m_authentication_supplied = true ;
	}
	else if( m_state == SentDescribe )
	{
		G_ASSERT( m_parser.header("Content-Type") == "application/sdp" ) ;
		std::vector<std::string> sdp_lines ;
		G::Str::splitIntoTokens(m_parser.body(),sdp_lines,"\r\n") ;

		m_session.reset( new Gv::Sdp(sdp_lines) ) ;

		for( std::vector<std::string>::iterator line_p = sdp_lines.begin() ; line_p != sdp_lines.end() ; ++line_p )
		{
			G_LOG( "RtspClient::onResponse: sdp=[" << *line_p << "]" ) ;
		}

		std::string format = m_session->fmtp() ; // picks the first if more than one
		formatLog( format , m_use_log_s ) ;
		formatSave( format , m_format_file ) ;

		if( m_describe_only )
		{
			GNet::EventLoop::instance().quit("") ;
			return ;
		}

		sendRequest( "SETUP" , m_session->mediaAttribute(0,"control") , m_transport ) ;
		m_state = SentSetup ;
	}
	else if( m_state == SentSetup )
	{
		std::string session_id = value( "Session" ) ;
		G_LOG( "RtspClient::onResponse: session-id=[" << session_id << "]" ) ;

		session_id = G::Str::head( session_id , session_id.find(";timeout=") , session_id ) ;
		std::string play_path = m_session->mediaAttribute(0,"control") ;
		G_LOG( "RtspClient::onResponse: control=[" << play_path << "]" ) ;

		sendRequest( "PLAY" , play_path , "Session: " + session_id ) ;
		m_state = SentPlay ;
	}
}

// ==

static bool quitAlways( std::string )
{
	return true ;
}

static bool quitTest( std::string reason )
{
	if( reason.find("peer disconnected") == 0U ) return false ; // GNet::SocketProtocol::ReadError, GNet::SocketProtocol::SendError
	if( reason.find("connect failure") == 0U ) return false ; // GNet::SimpleClient::ConnectError
	return true ;
}

int main( int argc , char * argv [] )
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
			"l!port!listening port range! eg. 8000-8001!1!port-port!1" "|"
			"m!multicast!multicast address! eg. 224.1.1.1!1!address!1" "|"
			"F!format-file!a file to use for storing the video format string!!1!path!1" "|"
			"x!connection-timeout!connection timeout!!1!seconds!1" "|"
			"R!retry!retry failed connections! or zero to exit (default 1s)!1!seconds!1" "|"
			"O!once!exit when the first session closes!!0!!1" "|"
		) ;
		std::string args_help = "<rtsp-url>" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 2U ) ;
		try
		{
			std::string url_string = opt.args().v(1U) ;
			std::string port_range = opt.contains("port") ? opt.value("port") : std::string("8000-8001") ;
			GNet::Address mcast = opt.contains("multicast") ? GNet::Address(opt.value("multicast")+":0") : GNet::Address::defaultAddress() ;
			unsigned int retry = G::Str::toUInt( opt.value("retry","1") ) ;
			unsigned int connection_timeout = G::Str::toUInt( opt.value("connection-timeout","30") ) ;
			bool once = opt.contains("once") ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			G::Url url( url_string ) ;
			GNet::Location location( url.host() , url.port(url.protocol()) ) ;

			RtspClientFactory factory( location , url , connection_timeout , port_range , !opt.contains("syslog") , opt.value("format-file") , mcast ) ;
			Gv::ClientHolder<RtspClientFactory,RtspClient> client_holder( factory , once , retry==0U?quitAlways:quitTest , retry ) ;

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


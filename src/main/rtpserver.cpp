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
// rtpserver.cpp
//
// Listens for an incoming RTP video stream and sends it to a local publication 
// channel. 
//
// Some network cameras use RTP rather than HTTP because it is more suited to 
// high resolution video, especially when using H.264 encoding, and it also 
// allows for IP multicasting.
//
// Only the "H264/90000" and "JPEG/90000" RTP payload types are supported by this
// server. 
//
// The H.264 video format requires plenty of processing power; if the H.264 
// decoding is too slow then packets will be missed and the packet stream
// will have too many holes to decode anything. The processing load can 
// be reduced by handling only RTP packets that match certain criteria by 
// using the `--filter` option, or by using `--scale` and `--monochrome`
// to reduce the image resolution after decoding.
//
// Cameras that use RTP will normally be controlled by an RTSP client (such as
// `vt-rtspclient`); the RTSP client asks the camera to start sending
// its video stream to the address that the RTP server is listening on.
//
// The camera responds to the RTSP request with a "fmtp" string, which describes
// the video format for the H.264 video packets. If this information is not
// available from within the RTP packet stream itself then it can be passed 
// to the rtpserver using the `--format-file` option.
//
// The RTCP protocol is not currently used so the rtpserver only opens one
// UDP port for listening, not two. Also be aware that if two rtpservers
// are listening on the same UDP port then they will steal packets from
// one another and it is likely that both will stop working.
//
// usage: rtpserver [<options>] [--port=<port>] [--scale={auto|<scale>}] 
//   [--address <bind-address>] [--multicast <group-address>]
//   [--save <base-dir>] [--viewer] [--channel <channel>]
//
// eg.
/// $ ./rtpserver --port=8000 --address=239.0.0.1 --multicast=239.0.0.1
/// $ ./rtpserver --port=8000 --multicast=224.3.3.3
//

#include "gdef.h"
#include "gnet.h"
#include "gvrtpserver.h"
#include "gvimageoutput.h"
#include "gravc.h"
#include "grjpeg.h"
#include "gvavcreader.h"
#include "gvtimezone.h"
#include "gstr.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gaddress.h"
#include "gtimerlist.h"
#include "geventloop.h"
#include "gfile.h"
#include "garg.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "gassert.h"
#include <cstdlib>
#include <exception>
#include <iostream>

/// \class RtpServer
/// An application class for rtpserver.
///
class RtpServer : private Gv::RtpServer, public Gv::RtpServerHandler
{
public:
	RtpServer( GNet::Address bind_address , std::string group_address , unsigned int packet_type , 
		std::string format_file , int jpeg_fudge_factor , const std::string & filter_spec ,
		int scale , bool viewer , unique_ptr<G::Publisher> & , bool monochrome ,
		unsigned int source_stale_timeout ) ;

private:
	virtual void onImage( const std::vector<char> & , const Gr::ImageType & , bool ) override ;

private:
	Gv::ImageOutput m_image_output ;
	unsigned int m_frame_count ;
	bool m_had_key_frame ;
} ;

// ==

RtpServer::RtpServer( GNet::Address bind_address , std::string group_address , unsigned int packet_type , 
	std::string format_file , int jpeg_fudge_factor , const std::string & filter_spec ,
	int scale , bool viewer , unique_ptr<G::Publisher> & channel , bool monochrome ,
	unsigned int source_stale_timeout ) :
		Gv::RtpServer(*this,scale,monochrome,bind_address,group_address,packet_type,format_file,jpeg_fudge_factor,filter_spec,source_stale_timeout) ,
		m_image_output(*this) ,
		m_frame_count(0U) ,
		m_had_key_frame(false)
{
	G_LOG( "RtpServer::RtpServer: rtpserver listening on " << bind_address.displayString() ) ;

	if( viewer )
	{
		std::string viewer_title = "rtpserver" ;
		m_image_output.startViewer( viewer_title ) ;
	}

	if( channel.get() != nullptr )
	{
		m_image_output.startPublisher( channel.release() ) ;
	}
}

void RtpServer::onImage( const std::vector<char> & buffer_in , const Gr::ImageType & image_type , bool key_frame )
{
	m_frame_count++ ;
	if( key_frame )
		m_had_key_frame = true ;

	if( !m_had_key_frame && m_frame_count < 100 ) // suppress avc images prior to the first key frame (up to some sanity limit)
		G_LOG( "RtpServer::hadKeyFrame: frame " << m_frame_count << " ignored: waiting for first key frame" ) ;
	else
		m_image_output.send( &buffer_in[0] , buffer_in.size() , image_type ) ;
}

// ==

int main( int argc, char ** argv )
{
	try
	{
		G::Arg arg( argc , argv ) ;
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
			"s!scale!reduce the image size! (or 'auto')!1!divisor!1" "|"
			"o!monochrome!convert to monochrome!!0!!1" "|"
			"l!port!listening port!!1!port!1" "|"
			"c!channel!publish to the named channel!!1!channel!1" "|"
			"a!address!listening address!!1!ip!1" "|"  
			"m!multicast!multicast group address!!1!address!1" "|"
			"F!format-file!file to read for the H264 format parameters!!1!path!1" "|"
			"Y!type!process packets of one rtp payload type! (eg. 26 or 96)!1!type!1" "|"
			"j!jpeg-table!tweak for jpeg quantisation tables! (0,1 or 2)!1!ff!1" "|"
			"f!filter!process only matching rtp packets! (eg. '5,7,8')!1!type!3" "|"
		) ;
		std::string args_help = "" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 1U ) ;
		try
		{
			unsigned int source_stale_timeout = 10U ;
			std::string format_file = opt.value("format-file",std::string()) ;
			std::string group_address = opt.value("multicast",std::string()) ;
			unsigned int packet_type = opt.contains("type") ? G::Str::toUInt(opt.value("type")) : 0U ;
			int jpeg_fudge_factor = G::Str::toUInt(opt.value("jpeg-table","0")) ;
			std::string filter = opt.value("filter") ;
			unsigned int port = opt.contains("port") ? G::Str::toUInt(opt.value("port")) : 8000U ;
			std::string ip_address = opt.contains("address") ? opt.value("address") : std::string() ;
			GNet::Address bind_address = ip_address.empty() ? 
				GNet::Address(GNet::Address::Family::ipv4(),port) : 
				GNet::Address(ip_address,port) ;

			if( !opt.contains("viewer") && opt.value("channel").empty() )
				throw std::runtime_error( "nothing to do: use \"--viewer\" or \"--channel\"" ) ;

			if( opt.contains("daemon") && opt.contains("format-file") && !G::Path(format_file).isAbsolute() )
				throw std::runtime_error( "format file must be an absolute path when using \"--daemon\"" ) ;

			if( opt.contains("format-file") && !G::File::exists(format_file) )
				G_WARNING( "rtpserver: format file missing at startup: " + format_file ) ;

			// expect packet type 26 for jpeg, or something in the dynamic range
			if( packet_type != 0U && packet_type != 26U && !( packet_type >= 96U && packet_type <= 127U ) )
				G_WARNING( "rtpserver: unexpected packet type value" ) ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			unique_ptr<G::Publisher> publisher ;
			if( opt.contains("channel") )
			{
				G::Item info = G::Item::map() ;
				info.add( "type" , G::Str::tail(G::Path(G::Arg::v0()).basename(),"-",false) ) ;
				info.add( "address" , bind_address.displayString() ) ;
				publisher.reset( new G::Publisher(opt.value("channel"),info,true) ) ;
			}

			startup.start() ;

			int scale = 1 ;
			if( opt.value("scale") == "auto" )
				scale = -1 ;
			else if( opt.contains("scale") )
				scale = std::max( 1 , G::Str::toInt(opt.value("scale")) ) ;

			::RtpServer server( bind_address , group_address , packet_type , format_file , 
				jpeg_fudge_factor , filter , scale , opt.contains("viewer") , publisher ,
				opt.contains("monochrome") , source_stale_timeout ) ;

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


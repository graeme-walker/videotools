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
// webcamplayer.cpp
//
// Displays or publishes video from a local webcam or video capture device.
//
// If no publication channel is specified then the video is just displayed in 
// a window. The published video can be recorded by the `vt-recorder` program
// and later played back by `vt-fileplayer`, and it can be analysed by a 
// `vt-watcher`.
//
// Use `--device=test` or `--device=demo` for testing without an attached 
// camera, or on linux use the "vivi" kernel module (`sudo modprobe vivi`).
// Device-specific configuration options can be added after the device
// name, using a semi-colon separator.
//
// The program will normally poll for the webcam device to come on-line, although
// `--retry=0` can be used to disable this behaviour. When the device goes 
// off-line again the program will go back to polling, or it will terminate if the
// `--once` option is used.
//
// The `--tz` option can be used with `--caption` to adjust the timezone for 
// the caption timestamp. It is typically a negative number for western 
// longitudes in order to show local time.
//
// usage: webcamplayer [--viewer] [--channel=<channel>] [--caption] [--device=<video-device>] 
//

#include "gdef.h"
#include "gnet.h"
#include "geventloop.h"
#include "gtimer.h"
#include "geventhandler.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "gvcamera.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "ggetopt.h"
#include "gvimageoutput.h"
#include "gvtimezone.h"
#include "glog.h"
#include "glogoutput.h"
#include <vector>
#include <errno.h>

class WebcamPlayer : private Gv::ImageInputHandler , private GNet::EventExceptionHandler
{
public:
	WebcamPlayer( Gr::ImageConverter & , const std::string & channel , bool with_viewer , 
		const std::string & dev_name , const std::string & dev_config , 
		bool once , unsigned int reopen_timeout , 
		int scale , bool monochrome , const std::string & caption , 
		const Gv::Timezone & caption_tz ) ;
			// Constructor.

	virtual ~WebcamPlayer() ;
		// Destructor.

private:
	WebcamPlayer( const WebcamPlayer & ) ;
	void operator=( const WebcamPlayer & ) ;
	virtual void onImageInput( Gv::ImageInputSource & , Gr::Image ) override ;
	virtual Gv::ImageInputConversion imageInputConversion( Gv::ImageInputSource & ) override ;
	virtual void onException( std::exception & ) override ;

private:
	bool m_with_viewer ;
	Gv::Camera m_camera ;
	Gv::ImageOutput m_image_output ;
	int m_scale ;
	bool m_monochrome ;
} ;

// ==

WebcamPlayer::WebcamPlayer( Gr::ImageConverter & converter , const std::string & channel , bool with_viewer , 
	const std::string & dev_name , const std::string & dev_config , 
	bool once , unsigned int reopen_timeout , 
	int scale , bool monochrome , const std::string & caption , 
	const Gv::Timezone & caption_tz ) :
		m_with_viewer(channel.empty()||with_viewer) ,
		m_camera(converter,dev_name,dev_config,once,reopen_timeout!=0U/*lazy-open*/,std::max(1U,reopen_timeout),caption,caption_tz) ,
		m_image_output(*this) ,
		m_scale(scale) ,
		m_monochrome(monochrome)
{
	// start the viewer
	if( m_with_viewer )
	{
		std::string viewer_title = dev_name ;
		m_image_output.startViewer( viewer_title ) ;
	}

	// start the channel
	if( !channel.empty() )
	{
		G::Item info = G::Item::map() ;
		info.add( "tz" , caption_tz.str() ) ;
		info.add( "device" , dev_name ) ;
		m_image_output.startPublisher( channel , info ) ;
	}

	// issue a warning if the webcam device is not yet available
	if( !m_camera.isOpen() )
	{
		G_ASSERT( reopen_timeout != 0U ) ; // otherwise camera ctor throws
		G_WARNING( "WebcamPlayer::ctor: failed to open device [" << dev_name << "]: retry period " << reopen_timeout << "s" ) ;
	}

	// link up the camera to our onImageInput() callback
	m_camera.addImageInputHandler( *this ) ;
}

WebcamPlayer::~WebcamPlayer()
{
	m_camera.removeImageInputHandler( *this ) ;
}

void WebcamPlayer::onException( std::exception & )
{
	throw ;
}

void WebcamPlayer::onImageInput( Gv::ImageInputSource & , Gr::Image image )
{
	G_ASSERT( !image.empty() ) ;
	G_DEBUG( "WebcamPlayer::onImageInput: image input: " << image.size() << " " << image.type() ) ;
	m_image_output.send( image.data() , image.type() ) ;
}

Gv::ImageInputConversion WebcamPlayer::imageInputConversion( Gv::ImageInputSource & )
{
	Gv::ImageInputConversion convert ;
	if( m_scale > 1 || m_monochrome )
	{
		convert.type = Gv::ImageInputConversion::to_raw ;
		convert.scale = m_scale ;
		convert.monochrome = m_monochrome ;
	}
	return convert ;
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
			"D!device!video device! (eg. /dev/video0)!1!device!1" "|"
			"p!caption!add a timestamp caption!!0!!1" "|"
			"C!caption-text!defines the caption text! in ascii, with %t for a timestamp!1!text!1" "|"
			"w!viewer!run a viewer!!0!!1" "|"
			"c!channel!publish to the named channel!!1!channel!1" "|"
			"s!scale!reduce the image size!!1!divisor!1" "|"
			"z!tz!timezone offset! for the caption!1!hours!1" "|"
			"o!monochrome!convert to monochrome!!0!!1" "|"
			"R!retry!poll for the device to appear!!1!timeout!1" "|"
			"O!once!exit if the webcam device disappears!!0!!1" "|"
		) ;
		std::string args_help = "" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 1U ) ;
		try
		{
			std::string dev_name = opt.value("device","/dev/video0") ;
			std::string channel = opt.value("channel",std::string()) ;
			bool with_viewer = opt.contains("viewer") ;
			std::string caption = opt.contains("caption") ? opt.value("caption-text","%t") : std::string() ;
			std::string dev_config = G::Str::tail( dev_name , dev_name.find(';') , std::string() ) ;
			dev_name = G::Str::head( dev_name , dev_name.find(';') , dev_name ) ;
			int scale = G::Str::toInt(opt.value("scale","1")) ;
			std::string caption_tz = opt.value("tz","0") ;
			bool monochrome = opt.contains("monochrome") ;
			unsigned int retry = G::Str::toUInt(opt.value("retry","1")) ;
			bool once = opt.contains("once") ;

			if( !opt.contains("device") )
				G_LOG( "webcamplayer: video capture device [" << dev_name << "]" ) ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			Gr::ImageConverter converter ;
			WebcamPlayer webcam_player( converter , channel , with_viewer , dev_name , dev_config , 
				once , retry , scale , monochrome , caption , Gv::Timezone(caption_tz) ) ;

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

/// \file webcamplayer.cpp

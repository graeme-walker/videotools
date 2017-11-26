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
// recorder.cpp
//
// Reads a video stream from a publication channel and records it to disk.
//
// Individual video frames are stored in a directory tree, organised by date
// and time.
//
// Images are recorded as fast as they are received, or once a second, or not 
// at all. These are the fast, slow and stopped states.
//
// The state can be changed by sending a `fast`, `slow`, or `stopped` command 
// over the recorder's command socket (`--command-socket`).
//
// The fast state is temporary unless the `--timeout` option is set to zero; 
// after recording in the fast state for a while the state drops back to the 
// previous slow or stopped state. 
//
// The recorder enters the fast state on startup (`--fast`) or when it gets a 
// `fast` command over its command socket, typically sent by the `vt-watcher` 
// program. On entering the fast state any in-memory images from the last few
// seconds are also committed to disk.
//
// A sub-directory called `.cache` is used to hold the recent images that have 
// not been commited to the main image store. These files are held open, and 
// they should not be accessed externally. When the recorder is triggered
// by a `fast` command on the command socket the cached files are moved
// into the main image store so that they provide a lead-in to the event
// that triggered the recording.
//
// On machines with limited memory the cache size should be tuned so that
// there is no disk activity when the recorder is in the "stopped" state.
// The cache can be completely disabled by setting the cache size to zero, 
// but then there will be no lead-in to recordings triggered by an event.
//
// Use `--fast --timeout=0 --cache-size=0` for continuous recording of a 
// channel.
//
// The `--tz` option is the number of hours added to UTC when constructing
// file system paths under the base directory, so to get local times at western
// longitudes the value should be negative (eg. `--tz=-5`). However, if this 
// option is used inconsistently then there is a risk of overwriting or 
// corrupting old recordings, so it is safer to use UTC recordings and 
// add `--tz` adjustments elsewhere.
//
// Loopback filesystems are one way to put a hard limit on disk usage. On 
// Linux do something like this as root 
// `dd if=/dev/zero of=/usr/share/recordings.img count=20000`,
// `mkfs -t ext2 -F /usr/share/recordings.img`, 
// `mkdir /usr/share/recordings`,
// `mount -o loop /usr/share/recordings.img /usr/share/recordings`,
// add add the mount to `/etc/fstab`.
//
// Individual video streams should normally be recorded into their own directory
// tree, but the `--name` option allows multiple video streams to share the same
// root directory by adding a fixed prefix to every image filename. However,
// this approach does not scale well on playback, so only use it for a very
// small number of video streams.
//
// usage: recorder [<options>] <image-channel-in> <base-dir>
//

#include "gdef.h"
#include "gvimageoutput.h"
#include "gvcache.h"
#include "gvimageinput.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gvcommandsocket.h"
#include "gvtimezone.h"
#include "gdatetime.h"
#include "gpublisher.h"
#include "gpath.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "gassert.h"
#include <errno.h>
#include <exception>
#include <iostream>

class Recorder : private Gv::ImageInputSource, private Gv::ImageInputHandler, private GNet::EventHandler, private Gv::CommandSocketMixin
{
public:
	enum State { s_init , s_stopped , s_fast , s_slow } ;
	Recorder( Gr::ImageConverter & , const std::string & image_channel , const std::string & name , const std::string & command_socket_path ,
		G::Path base_dir , int scale , const std::string & file_type , State base_state , bool set_state_fast ,
		unsigned int fast_timeout , size_t , const Gv::Timezone & tz , unsigned int reopen_timeout , bool once ) ;
	~Recorder() ;
	void run() ;

private:
	void setState( State ) ;
	void checkState() ;
	void cacheStore( const Gr::ImageBuffer & , Gr::ImageType type , G::EpochTime , const G::Path & same_as ) ;
	virtual void onImageInput( Gv::ImageInputSource & , Gr::Image ) override ;
	virtual Gv::ImageInputConversion imageInputConversion( Gv::ImageInputSource & ) override ;
	virtual void resend( Gv::ImageInputHandler & ) override ;
	virtual void onException( std::exception & ) override ; // GNet::EventExceptionHandler
	virtual void readEvent() override ; // GNet::EventHandler
	virtual void onCommandSocketData( std::string ) override ; // Gv::CommandSocketMixin
	void onTimeout() ;

private:
	G::PublisherSubscription m_image_channel ;
	G::Path m_base_dir ;
	std::string m_name ;
	Gv::ImageOutput m_image_output ;
	Gv::Cache m_cache ;
	Gv::ImageInputConversion m_conversion ;
	State m_state ;
	State m_old_state ;
	G::EpochTime m_fast_time ;
	time_t m_fast_timeout ;
	Gv::Timezone m_tz ;
	unsigned int m_reopen_timeout ;
	bool m_once ;
	Gr::Image m_image ;
	std::string m_image_type_str ;
	GNet::Timer<Recorder> m_timer ;
} ;

Recorder::Recorder( Gr::ImageConverter & converter , 
	const std::string & image_channel_name , const std::string & name ,
	const std::string & command_socket_path , G::Path base_dir , int scale , 
	const std::string & file_type , State base_state , bool set_state_fast ,
	unsigned int fast_timeout , size_t cache_size , const Gv::Timezone & tz , 
	unsigned int reopen_timeout , bool once ) :
		Gv::ImageInputSource(converter) ,
		Gv::CommandSocketMixin(command_socket_path) ,
		m_image_channel(image_channel_name,reopen_timeout!=0U/*lazy-open*/) ,
		m_base_dir(base_dir) ,
		m_name(name) ,
		m_cache(base_dir,name,cache_size) ,
		m_state(s_init) ,
		m_old_state(s_init) ,
		m_fast_time(0) ,
		m_fast_timeout(static_cast<time_t>(fast_timeout)) ,
		m_tz(tz) ,
		m_reopen_timeout(reopen_timeout) ,
		m_once(once) ,
		m_timer(*this,&Recorder::onTimeout,*this)
{
	G_ASSERT( base_state == s_slow || base_state == s_stopped ) ;
	setState( base_state ) ;
	if( set_state_fast )
		setState( s_fast ) ;

	m_conversion.scale = scale ;
	if( file_type == "jpeg" || file_type == "jpg" ) 
	{
		m_conversion.type = Gv::ImageInputConversion::to_jpeg ;
	}
	else if( file_type == "ppm" )
	{
		m_conversion.type = Gv::ImageInputConversion::to_raw ;
	}
	else if( file_type == "pgm" )
	{
		m_conversion.type = Gv::ImageInputConversion::to_raw ;
		m_conversion.monochrome = true ;
	}
	else if( !file_type.empty() )
	{
		throw std::runtime_error( "invalid file type [" + file_type + "]" ) ;
	}

	if( m_reopen_timeout != 0U && !m_image_channel.open() )
	{
		G_ASSERT( m_image_channel.fd() == -1 ) ;
		G_WARNING( "Recorder::ctor: channel not available at startup: " << m_image_channel.name() ) ;
		m_timer.startTimer( m_reopen_timeout ) ;
	}

	if( m_image_channel.fd() >= 0 )
		GNet::EventLoop::instance().addRead( GNet::Descriptor(m_image_channel.fd()) , *this ) ;

	addImageInputHandler( *this ) ;
}

Recorder::~Recorder()
{
	removeImageInputHandler( *this ) ;
	if( m_image_channel.fd() != -1 )
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_image_channel.fd()) ) ;
}

void Recorder::onException( std::exception & )
{
	throw ;
}

void Recorder::checkState()
{
	// if we have been fast for a while, flip back to slow or stopped
	if( m_state == s_fast && m_fast_timeout != 0 && (G::DateTime::now()-m_fast_time).s > m_fast_timeout )
	{
		G_ASSERT( m_old_state == s_slow || m_old_state == s_stopped ) ;
		setState( m_old_state ) ;
	}
}

void Recorder::setState( State state )
{
	G_ASSERT( state != s_init ) ;
	if( state == s_fast )
		m_fast_time = G::DateTime::now() ;

	if( state != m_state )
	{
		G_LOG( "Recorder::run: recording speed: " << (state==s_fast?"fast":(state==s_slow?"slow":"stopped")) ) ;
		if( state == s_fast )
			m_old_state = m_state ;
		m_state = state ;

		if( m_state == s_fast )
			m_image_output.saveTo( m_base_dir.str() , m_name , true , m_tz , false ) ;
		else if( m_state == s_slow )
			m_image_output.saveTo( m_base_dir.str() , m_name , false , m_tz , false ) ;
		else
			m_image_output.saveTo( std::string() , m_name , false , m_tz , false ) ;
	}
}

void Recorder::onImageInput( Gv::ImageInputSource & , Gr::Image image )
{
	if( image.valid() ) // only record images
	{
		G::EpochTime time = G::DateTime::now() ;

		// save to disk
		G::Path path = m_image_output.send( image.data() , image.type() , time ) ;

		// save to cache
		cacheStore( image.data() , image.type() , time , path ) ;
	}
}

Gv::ImageInputConversion Recorder::imageInputConversion( Gv::ImageInputSource & )
{
	return m_conversion ;
}

void Recorder::readEvent()
{
	G_DEBUG( "Recorder::readEvent: channel subscriber event" ) ;

	// come out of the fast state if appropriate
	checkState() ;

	// read the image channel
	if( ! Gr::Image::receive( m_image_channel , m_image , m_image_type_str ) )
	{
		G_ASSERT( m_image_channel.fd() >= 0 ) ;
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_image_channel.fd()) ) ;
		m_image_channel.close() ;
		G_ASSERT( m_image_channel.fd() == -1 ) ;

		if( m_once )
		{
			throw std::runtime_error( "channel publisher has gone away: " + m_image_channel.name() ) ;
		}
		else
		{
			G_WARNING( "Recorder::readEvent: channel publisher has gone away: " << m_image_channel.name() ) ;
			m_timer.startTimer( m_reopen_timeout ) ;
		}
	}

	// send the image back to ourselves via ImageInputSource
	sendImageInput( m_image ) ;
}

void Recorder::onTimeout()
{
	if( m_image_channel.open() )
	{
		G_LOG( "Recorder::onTimeout: channel opened: " << m_image_channel.name() ) ;
		GNet::EventLoop::instance().addRead( GNet::Descriptor(m_image_channel.fd()) , *this ) ;
	}
	else
	{
		m_timer.startTimer( m_reopen_timeout ) ;
	}
}

void Recorder::onCommandSocketData( std::string s )
{
	G::Str::trim( s , G::Str::ws() ) ;
	G_LOG( "Recorder::onCommandSocketData: command=[" << G::Str::printable(s) << "]" ) ;
	if( s == "fast" )
	{
		setState( s_fast ) ;
		m_cache.commit() ;
	}
	else if( s == "slow" )
	{
		setState( s_slow ) ;
		m_cache.commit( true ) ;
	}
	else if( s == "stop" )
	{
		setState( s_stopped ) ;
	}
	else
	{
		G_WARNING( "Recorder::run: unknown command" ) ;
	}
}

void Recorder::cacheStore( const Gr::ImageBuffer & buffer , Gr::ImageType type , G::EpochTime time , const G::Path & same_as_path )
{
	if( m_state == s_stopped || m_state == s_slow )
	{
		std::string commit_path ; 
		Gv::ImageOutput::path( commit_path , m_cache.base() , m_name , time , type , true , m_tz , false ) ;

		std::string commit_path_other ; 
		Gv::ImageOutput::path( commit_path_other , m_cache.base() , m_name , time , type , false , m_tz , false ) ;

		m_cache.store( buffer , commit_path , commit_path_other , same_as_path.str() ) ;
	}
}

void Recorder::resend( Gv::ImageInputHandler & )
{
}

// ==

Recorder::State state_from( const std::string & state_name )
{
	if( state_name != "slow" && state_name != "stopped" && state_name != "fast" )
		throw std::runtime_error( "invalid state name [" + state_name + "]" ) ;
	return
		state_name == "slow" ? Recorder::s_slow : (
			state_name == "fast" ? Recorder::s_fast : Recorder::s_stopped ) ;
}

int main( int argc, char ** argv )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		G::GetOpt opt( arg , 
			"V!version!show the program version and exit!!0!!3" "|"
			"h!help!show this help!!0!!2" "|"
			"d!debug!with debug logging! if compiled in!0!!2" "|"
			"L!log-time!add a timestamp to log output!!0!!2" "|"
			"y!syslog!log to syslog!!0!!2" "|"
			"b!daemon!run in the background!!0!!2" "|"
			"u!user!user to switch to when idle if run as root!!1!username!2" "|"
			"P!pid-file!write process id to file!!1!path!2" "|"
			"v!verbose!verbose logging!!0!!1" "|"
			"s!scale!reduce the image size!!1!divisor!1" "|"
			"C!command-socket!socket for commands!!1!path!1" "|"
			"f!file-type!file format: jpg, ppm, or pgm!!1!type!1" "|"
			"T!timeout!fast-state timeout!!1!s!1" "|"
			"F!fast!start in fast state!!0!!1" "|"
			"Z!state!state when not fast! (slow or stopped, slow by default)!1!state!1" "|"
			"z!tz!timezone offset!!1!hours!1" "|"
			"S!cache-size!cache size!!1!files!1" "|"
			"n!name!prefix for all image files! (defaults to the channel name)!1!prefix!1" "|"
			"R!retry!poll for the input channel to appear!!1!timeout!1" "|"
			"O!once!exit if the input channel disappears!!0!!1" "|"
		) ;
		std::string args_help = "<input-channel> <base-dir>" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 3U ) ;
		try
		{
			std::string image_channel_name = opt.args().v(1U) ;
			std::string name = opt.value("name",image_channel_name) ;
			std::string base_dir = opt.args().v(2U) ;
			int scale = static_cast<int>( G::Str::toUInt(opt.value("scale","1")) ) ;
			size_t cache_size = static_cast<size_t>( G::Str::toUInt(opt.value("cache-size","100")) ) ;
			unsigned int fast_state_timeout = G::Str::toUInt(opt.value("timeout","15")) ;
			std::string state_name = opt.value("state","slow") ;
			Recorder::State base_state = state_from( state_name ) ;
			std::string tz = opt.value("tz","0") ;
			std::string file_type = opt.value("file-type","") ;
			unsigned int retry = G::Str::toUInt(opt.value("retry","0")) ;
			bool once = opt.contains("once") ;

			if( base_state == Recorder::s_fast )
				throw std::runtime_error( "invalid \"--state\" option" ) ;

			if( name.find_first_of(G::Str::meta()+G::Str::ws()+"/\\") != std::string::npos )
				throw std::runtime_error( "invalid characters in \"--name\"" ) ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			Gr::ImageConverter converter ;
			Recorder recorder( converter , image_channel_name , name , opt.value("command-socket") , base_dir , scale , 
				file_type , base_state , opt.contains("fast") , 
				fast_state_timeout , cache_size , Gv::Timezone(tz) , 
				retry , once ) ;
	
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

/// \file recorder.cpp

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
// fileplayer.cpp
// 
// Plays back recorded video to a publication channel or into a viewer window.
//
// The directory specified on the command-line is a base directory where the 
// video images can be found. 
//
// The base directory can also be specified using the `--root` option, in which 
// case the directory on the end of the command-line is just the starting point
// for the playback. If the `--rerootable` and `--command-socket` options are
// used then the root directory can be changed at run-time by sending an
// appropriate `move` command.
//
// In interactive mode (`--interactive`) the ribbon displayed at the bottom of 
// the viewer window can be used to move forwards and backwards through the 
// day's recording. Click on the ribbon to move within the current day, or click
// on the left of the main image to go back to the previous day, or on the right
// to go forwards to the next.
//
// The `--skip` option controls what proportion of images are skipped during
// playback (speeding it up), and the `--sleep` option can be used to add a 
// delay between each displayed image (slowing it down). A negative skip value
// puts the player into reverse.
//
// A socket interface (`--command-socket`) can be used to allow another program
// to control the video playback. Sockets can be local-domain unix sockets or 
// UDP sockets; for UDP the socket should be like `udp://<address>:<port>`, eg. 
// `udp://0:20001`. 
//
// Command messages are text datagrams in the form of a command-line, with 
// commands including `play`, `move {first|last|next|previous|.|<full-path>}`, 
// `ribbon <xpos>` and `stop`. The play command can have options of `--sleep=<ms>`, 
// `--skip=<count>`, `--forwards` and `--backwards`. The `move` command can 
// have options of `--match-name=<name>` and `--root=<root>`, although
// the `--root` option requires `--rerootable` on the program command-line.
// Multiple commands in one datagram should be separated by a semi-colon.
//
// The timezone option (`--tz`) affects the ribbon at the bottom of the display 
// window so that that the marks on the ribbon can span a local calendar day 
// even with UTC recordings. It also changes the start time for each new day 
// when navigating one day at a time. A positive timezone is typically used for
// UTC recordings viewed at western longitudes.
//
// If multiple recorders are using the same file store with different filename
// prefixes then the `--match-name` option can be used to disentangle the
// different recorder streams. Note that this is not the recommended because 
// it does not scale well and it can lead to crazy-slow startup while looking 
// for a matching file. The match name can be changed at run-time by using the
// `--match-name` option on a `move` command sent to the command socket.
//
// usage: fileplayer [--viewer] [--channel=<channel>] [--sleep=<ms>] 
//          [--skip=<count>] [--loop] [--root=<root>] <dir>
//

#include "gdef.h"
#include "gvimageoutput.h"
#include "gtimerlist.h"
#include "geventloop.h"
#include "gvviewerevent.h"
#include "groot.h"
#include "gtimer.h"
#include "gvribbon.h"
#include "grimagetype.h"
#include "grimagedecoder.h"
#include "gexception.h"
#include "goptions.h"
#include "goptionparser.h"
#include "gvcommandsocket.h"
#include "gvtimezone.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gfiletree.h"
#include "garg.h"
#include "ggetopt.h"
#include "genvironment.h"
#include "ghexdump.h"
#include "glogoutput.h"
#include "gassert.h"
#include <exception>
#include <iostream>
#include <map>

class Skippage
{
public:
	explicit Skippage( int n ) ;
	void update( bool f , bool b , bool n_set , unsigned int n , G::FileTree & ) ;
	void reset() ;
	bool skip() ;
	void noskip() ;
	bool inReverse() const ;

private:
	bool m_in_reverse ;
	unsigned int m_skip ;
	unsigned int m_skipped ;
} ;

class Sleepage
{
public:
	Sleepage( unsigned int ms , unsigned int minimum_ms ) ;
	bool update( bool , unsigned int ms ) ;
	G::EpochTime idleTime() const ;
	G::EpochTime time( bool idle = false ) const ;

private:
	bool update( unsigned int ms ) ;
	static G::EpochTime from_ms( unsigned long ms ) ;

private:
	G::EpochTime m_sleep_minimum ;
	G::EpochTime m_sleep ;
} ;

class FileTreeIgnore : public G::DirectoryTreeCallback
{
public:
	explicit FileTreeIgnore( const std::string & match_name ) ;
	virtual bool directoryTreeIgnore( const G::DirectoryList::Item & , size_t ) override ;
	void set( const std::string & match_name ) ;
	void disarm() ;

private:
	std::string m_match_name ;
	G::EpochTime m_time_limit ;
	size_t m_count ;
	bool m_armed ;
} ;

class ImageFader : private GNet::EventExceptionHandler
{
public:
	ImageFader( Gv::ImageOutput & output , unsigned int timeout_ms , bool fine ) ;
	void send( const Gr::ImageBuffer & , const Gr::ImageData & , Gr::ImageType ) ;
	void resend( const Gr::ImageBuffer & , const Gr::ImageData & , Gr::ImageType ) ;

private:
	void save( const Gr::ImageBuffer & , const Gr::ImageData & , Gr::ImageType ) ;
	void merge() ;
	void merge( unsigned int ) ;
	void send() ;
	void onTimeout() ;
	virtual void onException( std::exception & ) override ;

private:
	Gv::ImageOutput & m_output ;
	bool m_fine ;
	unsigned int m_timeout_ms ;
	G::EpochTime m_timeout ;
	Gr::ImageBuffer m_image_buffer_new ;
	Gr::ImageBuffer m_image_buffer_old ;
	Gr::ImageBuffer m_image_buffer_out ;
	Gr::ImageData m_image_data_new ;
	Gr::ImageData m_image_data_old ;
	Gr::ImageData m_image_data_out ;
	GNet::Timer<ImageFader> m_timer ;
	unsigned int m_i ;
	unsigned int m_n ;
} ;

class Image
{
public:
	Image( ImageFader & , std::pair<int,int> dx_range , std::pair<int,int> dy_range ) ;
	bool read( const G::Path & path , int scale , bool monochrome , bool blank_on_error ) ;
	std::string reason() const ;
	bool valid() const ;
	void send() const ;
	void resend() const ;
	int dx() const ;
	const Gr::ImageType & imageType() const ;
	size_t count() const ;
	void addRibbon( Gv::Ribbon & ) ;
	size_t size() const ;

private:
	Image( const Image & ) ;
	void operator=( const Image & ) ;
	Gr::ImageType readType( const G::Path & ) ;
	bool decode( const G::Path & , Gr::ImageType , int scale , bool monochrome ) ;
	void blank( const G::Path & ) ;

private:
	ImageFader & m_fader ;
	Gr::ImageType m_image_type ; // after decoding
	unsigned int m_count ;
	std::pair<int,int> m_fit_dx_range ;
	std::pair<int,int> m_fit_dy_range ;
	int m_fit_dx ;
	int m_fit_dy ;
	G::Path m_path ;
	bool m_ribbon_added ;
	Gr::ImageBuffer m_image_buffer ;
	Gr::ImageData m_image_data ;
	Gr::ImageDecoder m_decoder ;
	std::string m_reason ;
} ;

class RibbonConfig
{
public:
	RibbonConfig( bool no_ribbon , const std::string & match_name , Gv::Timezone tz ) ;
	bool enabled() const ;
	G::EpochTime scanTimeFirst() const ;
	G::EpochTime scanTime() const ;
	Gv::Timezone tz() const ;
	const std::string & matchName() const ;

private:
	bool m_no_ribbon ;
	std::string m_match_name ;
	G::EpochTime m_ribbon_scan_interval_first ;
	G::EpochTime m_ribbon_scan_interval ;
	Gv::Timezone m_ribbon_tz ;
} ;

class RibbonScanner
{
public:
	explicit RibbonScanner( const RibbonConfig & config ) ;
	bool timestamped( const G::Path & ) ;
	void start( const G::Path & , Gv::Ribbon * ribbon_p = nullptr ) ;
	void work() ;
	bool busy() const ;
	static bool ignore( const G::Path , size_t depth ) ;

private:
	RibbonScanner( const RibbonScanner & ) ;
	void operator=( const RibbonScanner & ) ;

private:
	const RibbonConfig & m_config  ;
	size_t m_tpos ;
	Gv::Ribbon * m_ribbon ;
	G::FileTree m_tree ;
	bool m_busy ;
} ;

class Command
{
public:
	G_EXCEPTION( Error , "invalid command" ) ;
	explicit Command( const std::string & line ) ;
	std::string operator()() const ;
	std::string arg() const ;
	std::string rootOption() const ;
	std::string matchNameOption() const ;
	bool forwardsOption() const ;
	bool backwardsOption() const ;
	bool hasSkipValue() const ;
	unsigned int skipValue() const ;
	bool hasSleepValue() const ;
	unsigned int sleepValue() const ;

private:
	std::string m_command ;
	std::string m_command_arg ;
	bool m_skip_value_set ;
	unsigned int m_skip_value ;
	bool m_sleep_value_set ;
	unsigned int m_sleep_value ;
	bool m_forwards ;
	bool m_backwards ;
	bool m_start ;
	bool m_stop ;
	std::string m_root ;
	std::string m_match_name ;
} ;

class FilePlayer : private Gv::ViewerEventMixin , private Gv::CommandSocketMixin , private GNet::EventExceptionHandler
{
public:
	FilePlayer( const G::Path & root , const G::Path & path , bool rerootable , const std::string & name , const std::string & channel , 
		const std::string & command_socket , bool no_ribbon , bool with_viewer , const std::string & viewer_event_channel , 
		std::pair<int,int> dx_range , std::pair<int,int> dy_range , int scale , bool monochrome , 
		const Gv::Timezone & ribbon_tz , int skip , unsigned int sleep_ms , bool stopped , bool loop ,
		unsigned int fade_timeout_ms , bool fade_fine ) ;

private:
	bool process( const G::Path & path , bool ) ;
	void resend() ;
	void doCommand( const std::string & ) ;
	void doCommandNext() ;
	void doCommandPrevious() ;
	void doCommandMove( const std::string & , const std::string & , const std::string & ) ;
	void doCommandRibbonMove( int ) ;
	void reposition( const G::Path & path ) ;
	void onRibbonTimeout() ;
	void onFileTimeout() ;
	void onViewerPingTimeout() ;
	virtual void onCommandSocketData( std::string ) override ; // Gv::CommandSocketMixin
	virtual void onViewerEvent( Gv::ViewerEvent::Info ) override ; // Gv::ViewerEventMixin
	virtual void onException( std::exception & ) override ; // GNet::EventExceptionHandler
	static void set( G::Item & , const std::string & , const std::string & ) ;

private:
	bool m_rerootable ;
	std::string m_name ;
	Skippage m_skip ;
	Sleepage m_sleepage ;
	int m_scale ;
	bool m_monochrome ;
	unsigned int m_fade_timeout_ms ;
	Gv::ImageOutput m_image_output ;
	ImageFader m_image_fader ;
	Image m_image ;
	//
	RibbonConfig m_ribbon_config ;
	Gv::Ribbon m_ribbon ;
	RibbonScanner m_ribbon_scanner ;
	//
	FileTreeIgnore m_tree_ignore ;
	G::FileTree m_tree ;
	unsigned int m_file_count ;
	bool m_stopped ;
	bool m_loop ;
	GNet::Timer<FilePlayer> m_ribbon_timer ;
	GNet::Timer<FilePlayer> m_file_timer ;
	std::string m_command_socket ;
} ;

// ==

FilePlayer::FilePlayer( const G::Path & root , const G::Path & path , bool rerootable , const std::string & name , const std::string & channel , 
	const std::string & command_socket , bool no_ribbon , bool with_viewer , const std::string & viewer_event_channel , 
	std::pair<int,int> dx_range , std::pair<int,int> dy_range , int scale , bool monochrome , 
	const Gv::Timezone & ribbon_tz , int skip , unsigned int sleep_ms , bool stopped , bool loop ,
	unsigned int fade_timeout_ms , bool fade_fine ) :
		Gv::ViewerEventMixin(viewer_event_channel) ,
		Gv::CommandSocketMixin(command_socket) ,
		m_rerootable(rerootable) ,
		m_name(name) ,
		m_skip(skip) ,
		m_sleepage(sleep_ms,10U) , // 10ms minimum => 100fps max
		m_scale(scale) ,
		m_monochrome(monochrome) ,
		m_fade_timeout_ms(fade_timeout_ms) ,
		m_image_output(*this) ,
		m_image_fader(m_image_output,m_fade_timeout_ms,fade_fine) ,
		m_image(m_image_fader,dx_range,dy_range) ,
		m_ribbon_config(no_ribbon,name,ribbon_tz) ,
		m_ribbon_scanner(m_ribbon_config) ,
		m_tree_ignore(name) ,
		m_tree(root,&m_tree_ignore) ,
		m_file_count(0U) ,
		m_stopped(stopped) ,
		m_loop(loop) ,
		m_ribbon_timer(*this,&FilePlayer::onRibbonTimeout,*this) ,
		m_file_timer(*this,&FilePlayer::onFileTimeout,*this) ,
		m_command_socket(command_socket)
{
	m_tree_ignore.disarm() ;
	G_LOG( "FilePlayer::ctor: root=[" << root << "] start=[" << path << "]" ) ;

	if( channel.empty() || with_viewer )
	{
		std::string viewer_title = root.str() ;
		m_image_output.startViewer( viewer_title , 1U , viewer_event_channel ) ;
	}

	if( !channel.empty() )
	{
		G::Item info = G::Item::map() ;
		set( info , "socket" , command_socket ) ;
		set( info , "port" , G::Str::fromUInt(Gv::CommandSocket::parse(command_socket).port) ) ;
		set( info , "tz" , ribbon_tz.str() ) ;
		set( info , "viewer" , viewer_event_channel ) ;
		set( info , "dirname" , root.basename() ) ;
		m_image_output.startPublisher( channel , info ) ;
	}

	if( m_skip.inReverse() )
	{
		m_tree.last() ;
		m_tree.reverse() ;
		G_ASSERT( m_tree.reversed() == m_skip.inReverse() ) ;
	}

	if( root != path )
	{
		int rc = m_tree.reposition( path , 0 ) ;
		if( rc == 1 ) // out-of-tree
			throw std::runtime_error( "invalid path: [" + path.str() + "] "
				"is not valid under root path [" + root.str() + "]" ) ;
		else if( rc == 2 ) // off-the-end
			throw std::runtime_error( "no files at [" + path.str() + "]" ) ;
	}

	m_file_timer.startTimer( 0 ) ;
}

void FilePlayer::set( G::Item & item , const std::string & key , const std::string & value )
{
	if( !value.empty() )
		item.add( key , value ) ;
}

void FilePlayer::resend()
{
	if( m_image.valid() )
		m_image.resend() ;
}

bool FilePlayer::process( const G::Path & path , bool show_blank_on_error )
{
	// skip or process
	if( m_skip.skip() )
	{
		G_LOG( "FilePlayer::process: file=[" << path << "]: skipped" ) ;
		return false ;
	}

	// read the image -- decode it locally rather than in the viewer, so we can 
	// crop every image to the same size, and add overlays etc. -- if the file
	// is bad then we either just go round the loop again and process the 
	// next file, or we display a blank image -- a blank image is typically
	// required after a move command since hunting for the next displayable
	// file could take a long time and still fail
	//
	if( !m_image.read(path,m_scale,m_monochrome,show_blank_on_error) )
	{
		G_LOG( "FilePlayer::process: file=[" << path << "]: failed to process file: " << m_image.reason() ) ;
		return false ;
	}
	G_LOG( "FilePlayer::process: file=[" << path << "]: processing [" << m_image.imageType() << "]" ) ;

	// lazy construction of the ribbon now we know the width
	if( m_ribbon_config.enabled() && m_ribbon.size() == 0U )
	{
		if( m_ribbon_scanner.timestamped(path) )
		{
			m_ribbon = Gv::Ribbon( m_image.dx() , m_tree.root() , m_name , m_ribbon_config.tz() ) ;
			m_ribbon_scanner.start( path , &m_ribbon ) ;
		}
	}

	// let the ribbon see the file and allow it to switch to a new day
	if( m_ribbon.apply( path ) )
	{
		m_ribbon_scanner.start( path ) ;
	}

	// schedule some background ribbon scanning
	if( m_ribbon_scanner.busy() )
		m_ribbon_timer.startTimer( 0 ) ;

	// add the ribbon strip at the bottom
	m_image.addRibbon( m_ribbon ) ;

	G_LOG( "FilePlayer::process: file=[" << path << "]: sending as [" << m_image.imageType() << "] size=" << m_image.size() ) ;
	m_image.send() ;
	return true ;
}

void FilePlayer::onException( std::exception & )
{
	throw ;
}

void FilePlayer::onFileTimeout()
{
	// the file-timer is used for each file iteration (ie. m_tree.next()), with a 
	// zero-length timeout used after repositioning or after trying to load a 
	// non-image file, and a slow idle timeout to resend the current image 
	// when playback is stopped -- the use of a timer callback allows ribbon 
	// scanning etc. to proceed in parallel

	bool moved = m_tree.moved() ;
	if( m_stopped && moved ) 
	{
		process( m_tree.current() , true/*blank-on-error*/ ) ;
		m_file_timer.startTimer( m_sleepage.idleTime() ) ;
	}
	else if( m_stopped )
	{
		resend() ;
		m_file_timer.startTimer( m_sleepage.idleTime() ) ;
	}
	else
	{
		bool sent = false ;
		G::Path previous_path = m_tree.current() ;
		G::Path path = m_tree.next() ;

		if( path == G::Path() )
		{
			// check for files created since the last directory read
			if( !m_tree.reversed() )
			{
				m_tree.last() ;
				G::Path new_path = m_tree.next() ;
				if( new_path != previous_path )
					path = new_path ;
			}
		}

		if( path == G::Path() ) // if end of file list
		{
			if( m_loop )
			{
				if( m_file_count == 0U ) 
					G_WARNING_ONCE( "FilePlayer::onFileTimeout: no files found on the first pass, but looping round again" ) ;
				m_file_count = 0U ;
				m_tree.reposition( m_tree.root() ) ;
				path = m_tree.next() ;
				m_skip.reset() ;
			}
			else if( m_file_count == 0U )
			{
				throw std::runtime_error( "no files" ) ;
			}
			else
			{
				// in this case we have come to the end of the set of files -- we stay 
				// up if there is a viewer so that it is the viewer that controls the 
				// process lifetime, and we also stay up if there is a command-socket
				// that might ask us to go round again
				//
				if( !m_image_output.viewing() && m_command_socket.empty() ) 
				{
					throw Gv::Exit() ;
				}
			}
		}

		if( path == G::Path() )
		{
			// keep the channel alive
			resend() ;
			m_file_timer.startTimer( m_sleepage.idleTime() ) ;
		}
		else
		{
			const bool blank_on_error = moved && m_image.count() != 0U ;
			sent = process( path , blank_on_error ) ;
			if( sent ) 
				m_file_count++ ;

			m_file_timer.startTimer( sent ? m_sleepage.time() : G::EpochTime(0) ) ;
		}
	}
}

void FilePlayer::onRibbonTimeout()
{
	m_ribbon_scanner.work() ;
	if( m_ribbon_scanner.busy() )
	{
		m_ribbon_timer.startTimer( 0 ) ;
	}
	else
	{
		m_image.addRibbon( m_ribbon ) ;
		m_image.send() ;
	}
}

void FilePlayer::onCommandSocketData( std::string line )
{
	G_LOG( "FilePlayer::onCommandSocketData: socket-command=[" << G::Str::printable(line) << "]" ) ;

	// allow multiple commands in one datagram
	G::StringArray commands ;
	G::Str::splitIntoFields( line , commands , ";\n" , '\\' ) ;
	for( G::StringArray::iterator p = commands.begin() ; p != commands.end() ; ++p )
	{
		doCommand( *p ) ;
	}
}

void FilePlayer::onViewerEvent( Gv::ViewerEvent::Info event )
{
	G_LOG( "FilePlayer::onViewerEvent: " << event ) ; 
	if( m_ribbon.size() && event.type == Gv::ViewerEvent::Down && m_image.imageType().valid() )
	{
		int dx = m_image.imageType().dx() ;
		int dy = m_image.imageType().dy() ;
		bool on_ribbon = event.y > (dy-Gv::Ribbon::height()) ;
		bool lhs = event.x < (dx/5) ;
		bool rhs = event.x > (dx-dx/5) ;
		if( on_ribbon )
		{
			doCommandRibbonMove( event.x ) ;
		}
		else if( lhs )
		{
			doCommandPrevious() ;
		}
		else if( rhs )
		{
			doCommandNext() ;
		}
		m_skip.noskip() ;
	}
}

void FilePlayer::doCommand( const std::string & line )
{
	try
	{
		Command command( line ) ;
		if( command() == "play" ) 
		{
			m_skip.update( command.forwardsOption() , command.backwardsOption() , command.hasSkipValue() , command.skipValue() , m_tree ) ;
			m_sleepage.update( command.hasSleepValue() , command.sleepValue() ) ;
			m_stopped = false ;
		}
		else if( command() == "stop" ) 
		{
			m_stopped = true ;
		}
		else if( command() == "move" ) 
		{
			doCommandMove( command.arg() , command.rootOption() , command.matchNameOption() ) ;
		}
		else if( command() == "ribbon" )
		{
			G_ASSERT( !command.arg().empty() && G::Str::isUInt(command.arg()) ) ;
			doCommandRibbonMove( G::Str::toUInt(command.arg()) ) ;
		}
	}
	catch( std::exception & e )
	{
		G_WARNING( "FilePlayer::doCommand: " << e.what() ) ;
	}
}

void FilePlayer::doCommandMove( const std::string & path , const std::string & new_root , const std::string & new_name )
{
	G_ASSERT( m_tree.reversed() == m_skip.inReverse() ) ;
	G_LOG( "FilePlayer::doCommandMove: move path=[" << path << "]" ) ;

	if( !new_root.empty() )
	{
		if( !m_rerootable )
			throw Command::Error( "requires \"--rerootable\"" ) ;
		else if( !G::Path(new_root).isAbsolute() )
			throw Command::Error( "must be an absolute path" ) ;
		else if( (m_tree.root()+"..").collapsed() != (G::Path(new_root)+"..").collapsed() )
			throw Command::Error( "not a sibling directory" ) ;
		else
			m_tree.reroot( new_root ) ;
	}

	if( !new_name.empty() )
	{
		m_tree.reroot( new_name ) ;
	}

	if( ( path == "last" && !m_tree.reversed() ) || ( path == "first" && m_tree.reversed() ) )
	{
		m_tree.last() ;
	}
	else if( ( path == "first" && !m_tree.reversed() ) || ( path == "last" && m_tree.reversed() ) )
	{
		m_tree.first() ;
	}
	else if( path == "next" )
	{
		doCommandNext() ;
	}
	else if( path == "previous" )
	{
		doCommandPrevious() ;
	}
	else if( path == "." )
	{
		; // no-op
	}
	else if( !path.empty() )
	{
		if( !m_tree.reposition(path) )
			throw Command::Error( "respositioning failed" ) ;
	}

	if( m_tree.moved() )
	{
		m_skip.noskip() ;
		m_file_timer.startTimer( 0 ) ;
	}
}

void FilePlayer::reposition( const G::Path & path )
{
	m_tree.reposition( path ) ;
	if( m_tree.moved() )
	{
		m_skip.noskip() ;
		m_file_timer.startTimer( 0 ) ; // esp. if stopped
	}
}

void FilePlayer::doCommandRibbonMove( int event_x )
{
	// the ribbon has one bucket per pixel horizontally, so the event 
	// x position is used directly as the ribbon bucket index, with just 
	// some overflow checks
	//
	size_t ribbon_offset = std::min(m_ribbon.size()-1U,static_cast<size_t>(std::max(0,event_x))) ; 
	G::Path ribbon_path = m_ribbon.find( ribbon_offset , m_tree.reversed() ) ;
	reposition( ribbon_path ) ;
}

void FilePlayer::doCommandPrevious()
{
	if( m_ribbon.size() == 0U )
		throw Command::Error( "no ribbon" ) ;

	if( m_tree.reversed() )
		reposition( m_ribbon.range().startpath() ) ;
	else
		reposition( m_ribbon.range(-1).startpath() ) ;
}

void FilePlayer::doCommandNext()
{
	if( m_ribbon.size() == 0U )
		throw Command::Error( "no ribbon" ) ;

	if( m_tree.reversed() )
		reposition( m_ribbon.range(+1).endpath() ) ;
	else
		reposition( m_ribbon.range().endpath() ) ;
}

// ==

ImageFader::ImageFader( Gv::ImageOutput & output , unsigned int timeout_ms , bool fine ) :
	m_output(output) ,
	m_fine(fine) ,
	m_timeout_ms(timeout_ms) ,
	m_timeout(timeout_ms/1000UL,(timeout_ms%1000UL)*1000UL) ,
	m_image_data_new(m_image_buffer_new) ,
	m_image_data_old(m_image_buffer_old) ,
	m_image_data_out(m_image_buffer_out) ,
	m_timer(*this,&ImageFader::onTimeout,*this) ,
	m_i(0U) ,
	m_n(m_fine?20U:6U)
{
}

void ImageFader::resend( const Gr::ImageBuffer & image_buffer_in , const Gr::ImageData & image_data_in , Gr::ImageType type )
{
	if( m_timeout_ms != 0U && ( m_i < m_n || m_timer.active() ) )
		; // no-op still fading in
	else
		m_output.send( image_buffer_in , type ) ;
}

void ImageFader::send( const Gr::ImageBuffer & image_buffer_in , const Gr::ImageData & image_data_in , Gr::ImageType type )
{
	if( m_timeout_ms == 0U )
	{
		m_output.send( image_buffer_in , type ) ;
	}
	else
	{
		save( image_buffer_in , image_data_in , type ) ;
		m_i = 1U ;
		m_timer.startTimer( 0 ) ;
	}
}

void ImageFader::onTimeout()
{
	merge() ;
	send() ;
	if( m_i < m_n )
	{
		m_i++ ;
		m_timer.startTimer( m_timeout ) ;
	}
}

void ImageFader::save( const Gr::ImageBuffer & image_buffer_in , const Gr::ImageData & image_data_in , Gr::ImageType type )
{
	G_ASSERT( type.isRaw() ) ;
	int dx = image_data_in.dx() ;
	int dy = image_data_in.dy() ;
	int channels = image_data_in.channels() ;

	m_image_data_new.resize( dx , dy , channels ) ;
	m_image_data_old.resize( dx , dy , channels ) ;
	m_image_data_out.resize( dx , dy , channels ) ;

	m_image_data_old.copyIn( m_image_buffer_new , dx , dy , channels ) ;
	m_image_data_new.copyIn( image_buffer_in , dx , dy , channels ) ;
}

void ImageFader::merge()
{
	merge( m_i ) ;
}

void ImageFader::merge( unsigned int i )
{
	G_ASSERT( i >= 1U && i <= m_n ) ;
	if( m_fine )
	{
		static const unsigned int mix_in[] = { 0,0,3,5,7,8,9,10,11,12,13,14,15,16,17,18,19,20 } ;
		static const unsigned int mix_out[] = { 20,20,19,18,17,16,15,14,13,12,11,10,9,8,7,5,3,0 } ;
		static const unsigned int mix_in_size = sizeof(mix_in)/sizeof(mix_in[0]) ;
		static const unsigned int mix_out_size = sizeof(mix_out)/sizeof(mix_out[0]) ;

		static unsigned int mix_denominator = 20U ;
		unsigned int mix_in_value = mix_in[i>=mix_in_size?(mix_in_size-1U):i] ;
		unsigned int mix_out_value = mix_out[i>=mix_out_size?(mix_out_size-1U):i] ;

		m_image_data_out.mix( m_image_data_new , m_image_data_old , mix_in_value , mix_out_value , mix_denominator ) ;
	}
	else
	{
		static const unsigned int mix_in[] = { 0,0,4,8,9,10 } ;
		static const unsigned int mix_out[] = { 10,10,8,6,4,0 } ;
		static const unsigned int mix_in_size = sizeof(mix_in)/sizeof(mix_in[0]) ;
		static const unsigned int mix_out_size = sizeof(mix_out)/sizeof(mix_out[0]) ;

		static unsigned int mix_denominator = 10U ;
		unsigned int mix_in_value = mix_in[i>=mix_in_size?(mix_in_size-1U):i] ;
		unsigned int mix_out_value = mix_out[i>=mix_out_size?(mix_out_size-1U):i] ;

		m_image_data_out.mix( m_image_data_new , m_image_data_old , mix_in_value , mix_out_value , mix_denominator ) ;
	}
}

void ImageFader::send()
{
	const int dx = m_image_data_out.dx() ;
	const int dy = m_image_data_out.dy() ;
	const int channels = m_image_data_out.channels() ;
	m_output.send( m_image_buffer_out , Gr::ImageType::raw(dx,dy,channels) ) ;
}

void ImageFader::onException( std::exception & )
{
	throw ;
}

// ==

Image::Image( ImageFader & fader , std::pair<int,int> dx_range , std::pair<int,int> dy_range ) :
	m_fader(fader) ,
	m_count(0U) ,
	m_fit_dx_range(dx_range) ,
	m_fit_dy_range(dy_range) ,
	m_fit_dx(0) ,
	m_fit_dy(0) ,
	m_ribbon_added(false) ,
	m_image_data(m_image_buffer,Gr::ImageData::Contiguous) // contiguous to reduce reallocs esp. when dy changes
{
	G_ASSERT( !valid() ) ;
}

int Image::dx() const
{
	return m_fit_dx ;
}

const Gr::ImageType & Image::imageType() const
{
	return m_image_type ;
}

bool Image::read( const G::Path & path , int scale , bool monochrome , bool blank_on_error )
{
	m_reason.clear() ;
	Gr::ImageType image_type = readType( path ) ;
	if( !image_type.valid() )
	{
		if( blank_on_error )
			blank( path ) ;
		else
			return false ;
	}

	if( !decode( path , image_type , scale , monochrome ) )
		return false ;

	m_path = path ;
	m_ribbon_added = false ;

	return true ;
}

std::string Image::reason() const
{
	return m_reason ;
}

Gr::ImageType Image::readType( const G::Path & path )
{
	// determine the image size early so that we can scale-to-fit
	Gr::ImageType image_type ;
	try
	{
		image_type = Gr::ImageDecoder::readType( path , true/*do_throw*/ ) ;
		if( !image_type.valid() )
			m_reason = "not an image file" ;
	}
	catch( std::exception & e )
	{
		m_reason = e.what() ; // eg. can't open file
	}
	return image_type ;
}

bool Image::decode( const G::Path & path , Gr::ImageType image_type_in , int scale , bool monochrome )
{
	try
	{
		G_LOG( "FilePlayer::process: file=[" << path << "]: file type  [" << image_type_in << "] tofit=" << m_fit_dx << "x" << m_fit_dy ) ;

		// the first image (after fixed scaling) defines the image-fit dimensions, but bounded by the given range
		if( m_count == 0U ) 
		{
			Gr::ImageType real_type = Gr::ImageDecoder::readType( path ) ;
			int dx = Gr::scaled( real_type.dx() , scale ) ;
			int dy = Gr::scaled( real_type.dy() , scale ) ;
			m_fit_dx = std::min( m_fit_dx_range.second , std::max(m_fit_dx_range.first,dx) ) ;
			m_fit_dy = std::min( m_fit_dy_range.second , std::max(m_fit_dy_range.first,dy) ) ;
		}

		const int fudge_factor = 3 ;
		Gr::ImageDecoder::ScaleToFit scale_to_fit( m_fit_dx , m_fit_dy , fudge_factor ) ;

		m_decoder.setup( scale , monochrome ) ;
		m_image_type = m_decoder.decode( image_type_in , path , m_image_data , scale_to_fit ) ;

		m_image_data.crop( m_fit_dx , m_fit_dy ) ;
		m_image_data.expand( m_fit_dx , m_fit_dy ) ;
		m_image_type = Gr::ImageType::raw( m_image_data.dx() , m_image_data.dy() , m_image_data.channels() ) ;

		m_count++ ; if(m_count==0U) m_count=1U ;
		return true ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "Image::decode: image decode failed: " << e.what() ) ;
		m_reason = e.what() ;
		return false ;
	}
}

void Image::blank( const G::Path & path )
{
	m_path = path ;
	m_image_data.resize( 1 , 1 , 3 ) ;
	m_image_type = Gr::ImageType::raw( 1 , 1 , 3 ) ;
	m_ribbon_added = false ;
}

bool Image::valid() const
{
	return m_image_type.valid() ;
}

void Image::addRibbon( Gv::Ribbon & ribbon )
{
	G_ASSERT( m_fit_dx > 0 && m_fit_dy > 0 ) ;
	G_ASSERT( ribbon.size() == 0U || ribbon.size() == static_cast<size_t>(m_fit_dx) ) ;
	G_ASSERT( m_image_data.size() == m_image_type.size() ) ;

	if( ribbon.size() == 0U )
		return ;

	// resize the image to make room for the ribbon
	if( !m_ribbon_added )
	{
		m_image_type = Gr::ImageType::raw( m_image_type.dx() , m_image_type.dy() + Gv::Ribbon::height() , m_image_type.channels() ) ;
		m_image_data.resize( m_image_type.dx() , m_image_type.dy() , m_image_type.channels() ) ;
		m_ribbon_added = true ;
	}

	// for each row of the ribbon...
	G_ASSERT( m_image_type.dy() > Gv::Ribbon::height() ) ;
	int row = 0 ;
	const int dx = m_image_type.dx() ;
	for( int y = m_image_type.dy()-Gv::Ribbon::height() ; y < m_image_type.dy() ; y++ , row++ )
	{
		Gv::Ribbon::List::const_iterator ribbon_p = ribbon.begin() ;
		for( int x = 0 ; x < dx && ribbon_p != ribbon.end() ; x++ , ++ribbon_p )
		{
			// set a green or yellow pixel
			const bool set = (*ribbon_p).set() ;
			const bool marked = (*ribbon_p).marked() ;
			if( marked )
				m_image_data.rgb( x , y , 255U , 255U , 0U ) ;
			else if( set )
				m_image_data.rgb( x , y , 0U , 255U , 0U ) ;
			else
				m_image_data.rgb( x , y , 0U , 0U , 0U ) ;
		}

		// add a blob
		if( row <= 2 )
		{
			ribbon_p = ribbon.begin() ;
			for( int x = 0 ; x < dx && ribbon_p != ribbon.end() ; x++ , ++ribbon_p )
			{
				if( (*ribbon_p).marked() )
				{
					m_image_data.rgb( std::max(0,x-1) , y , 255U , 0U , 0U ) ;
					m_image_data.rgb( x , y , 255U , 0U , 0U ) ;
					m_image_data.rgb( std::min(dx-1,x+1) , y , 255U , 0U , 0U ) ;
				}
			}
		}
	}
}

void Image::resend() const
{
	m_fader.resend( m_image_buffer , m_image_data , m_image_type ) ;
}

void Image::send() const
{
	m_fader.send( m_image_buffer , m_image_data , m_image_type ) ;
}

size_t Image::size() const
{
	return m_image_type.size() ;
}

size_t Image::count() const
{
	return m_count ;
}

// ==

Skippage::Skippage( int n ) : 
	m_in_reverse(n<0) , 
	m_skip(static_cast<unsigned int>(n<0?(-n-1):n)) , 
	m_skipped(0U) 
{
}

bool Skippage::inReverse() const
{
	return m_in_reverse ;
}

void Skippage::reset()
{
	m_skipped = 0U ;
}

void Skippage::update( bool forwards , bool backwards , bool n_set , unsigned int n , G::FileTree & tree )
{
	G_DEBUG( "Skippage::update: " << forwards << " " << backwards << " " << n_set << " " << n << " " << m_in_reverse ) ;
	G_ASSERT( m_in_reverse == tree.reversed() ) ;

	if( n_set )
	{
		m_skip = n ;
		m_skipped = std::max( m_skipped , m_skip ) ;
	}

	if( forwards && m_in_reverse )
	{
		tree.reverse( false ) ;
		m_in_reverse = false ;
		noskip() ;
	}
	else if( backwards && !m_in_reverse )
	{
		tree.reverse( true ) ;
		m_in_reverse = true ;
		noskip() ;
	}
}

void Skippage::noskip()
{
	// make the next skip() return false
	m_skipped = m_skip ; 
}

bool Skippage::skip()
{
	if( m_skip == 0U )
		return false ;

	m_skipped++ ;
	if( m_skipped >= (m_skip+1U) )
	{
		m_skipped = 0U ;
		return false ; // process
	}
	else
	{
		return true ; // skip
	}
}

// ==

Sleepage::Sleepage( unsigned int ms , unsigned int minimum_ms ) :
	m_sleep_minimum(from_ms(minimum_ms)) ,
	m_sleep(std::max(m_sleep_minimum,from_ms(ms)))
{
}

G::EpochTime Sleepage::idleTime() const
{
	return G::EpochTime(1) ;
}

G::EpochTime Sleepage::time( bool stopped ) const
{
	return stopped ? idleTime() : m_sleep ;
}

G::EpochTime Sleepage::from_ms( unsigned long ms )
{
	return G::EpochTime( ms/1000UL , (ms%1000UL)*1000UL ) ;
}

bool Sleepage::update( bool time_set , unsigned int time_ms )
{
	if( time_set )
		return update( time_ms ) ;
	else
		return false ;
}

bool Sleepage::update( unsigned int time_ms )
{
	G::EpochTime old = m_sleep ;
	m_sleep = std::max( m_sleep_minimum , from_ms(time_ms) ) ;
	return m_sleep != old ;
}

// ==

RibbonScanner::RibbonScanner( const RibbonConfig & config ) :
	m_config(config) ,
	m_ribbon(nullptr) ,
	m_busy(false) 
{
}

void RibbonScanner::start( const G::Path & path , Gv::Ribbon * ribbon )
{
	if( ribbon ) m_ribbon = ribbon ;
	G_ASSERT( m_ribbon != nullptr ) ;
	G_ASSERT( m_tpos != 0U ) ;
	m_tree = G::FileTree() ;
	m_busy = m_ribbon->scanStart(m_tree,path,m_tpos) && !m_ribbon->scanSome(m_tree,m_config.scanTimeFirst()) ;
}

void RibbonScanner::work()
{
	G_ASSERT( m_ribbon != nullptr ) ;
	G_ASSERT( m_busy ) ;
	if( m_ribbon->scanSome( m_tree , m_config.scanTime() ) )
		m_busy = false ;
}

bool RibbonScanner::timestamped( const G::Path & path )
{
	m_tpos = Gv::Ribbon::timepos( path , m_config.matchName() ) ;
	return m_tpos != 0U ;
}

bool RibbonScanner::busy() const
{
	return m_busy ;
}

// ==

RibbonConfig::RibbonConfig( bool no_ribbon , const std::string & match_name , Gv::Timezone tz ) :
	m_no_ribbon(no_ribbon) ,
	m_match_name(match_name) ,
	m_ribbon_scan_interval_first(0,600000U) ,
	m_ribbon_scan_interval(0,300000U) ,
	m_ribbon_tz(tz)
{
}

bool RibbonConfig::enabled() const
{
	return !m_no_ribbon ;
}

G::EpochTime RibbonConfig::scanTimeFirst() const
{
	return m_ribbon_scan_interval_first ;
}

G::EpochTime RibbonConfig::scanTime() const
{
	return m_ribbon_scan_interval ;
}

Gv::Timezone RibbonConfig::tz() const
{
	return m_ribbon_tz ;
}

const std::string & RibbonConfig::matchName() const
{
	return m_match_name ;
}

// ==

FileTreeIgnore::FileTreeIgnore( const std::string & match_name ) :
	m_match_name(match_name) ,
	m_time_limit(G::DateTime::now()+3U) ,
	m_count(0U) ,
	m_armed(true)
{
}

bool FileTreeIgnore::directoryTreeIgnore( const G::DirectoryList::Item & item , size_t )
{
	const bool ignore =
		item.m_name.find('.') == 0U || 
		( !m_match_name.empty() && !item.m_is_dir && item.m_name.find(m_match_name) != 0U ) ;

	if( ignore && m_armed )
	{
		m_count++ ;
		if( (m_count%100U) == 0U )
		{
			G::EpochTime now = G::DateTime::now() ;
			if( now > m_time_limit )
			{
				G_WARNING( "FileTreeIgnore::directoryTreeIgnore: no files matching [" 
					<< m_match_name << "] in the first " << m_count ) ;
				m_time_limit = now + 3U ;
			}
		}
	}

	return ignore ;
}

void FileTreeIgnore::disarm()
{
	m_armed = false ;
}

void FileTreeIgnore::set( const std::string & match_name )
{
	m_match_name = match_name ;
}

// ==

Command::Command( const std::string & line ) :
	m_skip_value_set(false) ,
	m_sleep_value_set(false) ,
	m_forwards(false) ,
	m_backwards(false)
{
	static std::map<std::string,std::pair<G::Options,unsigned int> > commands ;
	if( commands.empty() )
	{
		static G::Options play_options( 
			"_sleep_x__1_x_1," 
			"_skip_x__1_x_1," 
			"_forwards_x__0__1," 
			"_backwards_x__0__1," ,
			',' , '_' ) ;

		static G::Options move_options(
			"_match-name_x__1_x_1," 
			"_root_x__1_x_1," ,
			',' , '_' ) ;

		commands["move"] = std::make_pair( move_options , 1U ) ; // path, etc
		commands["ribbon"] = std::make_pair( G::Options() , 1U ) ; // xpos
		commands["play"] = std::make_pair( play_options , 0U ) ;
		commands["stop"] = std::make_pair( G::Options() , 0U ) ;
	}

	G::StringArray words ;
	G::Str::splitIntoTokens( line , words , G::Str::ws() ) ;
	if( words.empty() ) 
		throw Error( "empty command" ) ;

	m_command = words.at(0U) ;
	if( commands.find(m_command) == commands.end() )
		throw Error( "unknown command" , m_command ) ;

	G::OptionMap value_map ;
	G::StringArray errors ;
	G::OptionParser parser( commands[m_command].first , value_map , errors ) ;
	size_t pos = parser.parse( words , 1U ) ;
	if( !errors.empty() )
		throw Error( "invalid options" , G::Str::join(", ",errors) ) ;

	if( value_map.contains("forwards") && value_map.contains("backwards") )
		throw Error( "incompatible options" ) ;

	if( pos < words.size() )
		m_command_arg = words.at(pos) ;

	if( (pos+commands[m_command].second) != words.size() )
		throw Error( "usage error" ) ;

	if( m_command == "ribbon" && ( m_command_arg.empty() || !G::Str::isUInt(m_command_arg) ) )
		throw Error( "invalid x-position" ) ;

	if( value_map.contains("skip") )
	{
		if( value_map.value("skip").empty() || !G::Str::isUInt(value_map.value("skip")) )
			throw Error( "invalid skip value" ) ;
		m_skip_value = G::Str::toUInt( value_map.value("skip") ) ;
		m_skip_value_set = true ;
	}

	if( value_map.contains("sleep") )
	{
		if( value_map.value("sleep").empty() || !G::Str::isUInt(value_map.value("sleep")) )
			throw Error( "invalid sleep value" ) ;
		m_sleep_value = G::Str::toUInt( value_map.value("sleep") ) ;
		m_sleep_value_set = true ;
	}

	m_forwards = value_map.contains("forwards") ;
	m_backwards = value_map.contains("backwards") ;

	if( m_forwards && m_backwards )
		throw Error( "incompatible options" ) ;

	if( value_map.contains("root") )
	{
		m_root = value_map.value("root") ;
		if( m_root.empty() )
			throw Error( "invalid root path" ) ;
	}

	if( value_map.contains("match-name") )
	{
		m_match_name = value_map.value("match-name") ;
		if( m_match_name.empty() )
			throw Error( "invalid match name" ) ;
	}
}

std::string Command::operator()() const
{
	return m_command ;
}

std::string Command::arg() const
{
	return m_command_arg ;
}

std::string Command::rootOption() const
{
	return m_root ;
}

std::string Command::matchNameOption() const
{
	return m_match_name ;
}

bool Command::forwardsOption() const
{ 
	return m_forwards ; 
}

bool Command::backwardsOption() const
{ 
	return m_backwards ; 
}

bool Command::hasSkipValue() const
{ 
	return m_skip_value_set ; 
}

unsigned int Command::skipValue() const
{ 
	return m_skip_value ; 
}

bool Command::hasSleepValue() const
{ 
	return m_sleep_value_set ; 
}

unsigned int Command::sleepValue() const
{ 
	return m_sleep_value ; 
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
			"i!interactive!respond to viewer mouse events!!0!!1" "|"
			"W!viewer-channel!override the default viewer channel name!!1!channel!1" "|"
			"c!channel!publish the playback video to the named channel!!1!channel!1" "|"
			"D!sleep!inter-frame delay time! for slower playback!1!ms!1" "|"
			"S!skip!file skip factor! for faster playback!1!count!1" "|"
			"C!command-socket!socket for playback commands!!1!path!1" "|"
			"r!root!base directory!!1!path!1" "|"
			"R!rerootable!allow the root to be changed over the command-socket!!0!!1" "|"
			"O!stopped!begin in the stopped state!!0!!1" "|"
			"g!loop!loop! back to the beginning when reaching the end!0!!1" "|"
			"s!scale!reduce the image size!!1!divisor!1" "|"
			"z!tz!timezone offset! for ribbon!1!hours!1" "|"
			"n!match-name!prefix for matching image files!!1!prefix!1" "|"
			"o!monochrome!convert to monochrome!!0!!1" "|"
			"!no-ribbon!disable the ribbon!!0!!1" "|"
			"X!width!output image width!!1!pixels!1" "|"
			"Y!height!output image height!!1!pixels!1" "|"
			"F!fade!fade image transitions!!0!!1" "|"
		) ;
		std::string args_help = "<directory>" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 2U ) ;
		try
		{
			G::Path path = opt.args().v( 1U ) ;
			G::Path root = opt.value( "root" , path.str() ) ;
			int skip = opt.contains("skip") ? G::Str::toInt(opt.value("skip")) : 0 ;
			unsigned int sleep_ms = opt.contains("sleep") ? G::Str::toUInt(opt.value("sleep")) : 40U ;
			std::string channel = opt.value("channel",std::string()) ;
			std::string viewer_channel = opt.value("viewer-channel","") ;
			bool with_viewer = opt.contains("viewer") ;
			int scale = G::Str::toInt(opt.value("scale","1")) ; // scale locally, not passed to viewer
			std::string ribbon_tz = opt.value("tz","0") ;
			bool no_ribbon = opt.contains("no-ribbon") ;
			std::string match_name = opt.value("match-name") ;
			int width = opt.contains("width") ? G::Str::toInt(opt.value("width")) : 0 ;
			int height = opt.contains("height") ? G::Str::toInt(opt.value("height")) : 0 ;
			bool fade = opt.contains("fade") ;
			if( fade && !opt.contains("sleep") ) sleep_ms = 700U ;
			bool slower_fade = G::Environment::get("DISPLAY","").empty() ; // heuristic for slower and cruder fade
			unsigned int fade_timeout_ms = fade ? (slower_fade?60U:15U) : 0U ;
			bool fade_fine = !slower_fade ;

			if( opt.contains("interactive") ) // convenience for --viewer and --viewer-channel
			{
				with_viewer = true ;
				if( viewer_channel.empty() )
 					viewer_channel = "fileplayer-viewer." + G::Str::fromInt(static_cast<int>(::getpid())) ;
			}

			if( opt.contains("rerootable") && !opt.contains("command-socket") )
				G_WARNING( "fileplayer: the \"--rerootable\" option does nothing without \"--command-socket\"" ) ;

			if( path == G::Path() )
				throw std::runtime_error( "invalid path" ) ;

			if( ( opt.contains("daemon") || opt.contains("command-socket") ) && ( ( root != G::Path() && root.isRelative() ) || path.isRelative() ) )
				throw std::runtime_error( "use absolute paths if also using \"--daemon\" or \"--command-socket\"" ) ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			// if no --scale then impose a reasonable limit on the window size
			std::pair<int,int> dx_range = opt.contains("scale") ? std::make_pair(1,99999) : std::make_pair(120,3000) ;
			std::pair<int,int> dy_range = opt.contains("scale") ? std::make_pair(1,99999) : std::make_pair(80,2000) ;

			// force the exact window size for --width and --height
			if( width ) dx_range = std::make_pair( width , width ) ;
			if( height ) dy_range = std::make_pair( height , height ) ;

			FilePlayer file_player( root , path , opt.contains("rerootable") , match_name , channel , opt.value("command-socket") , 
				no_ribbon , with_viewer , viewer_channel , dx_range , dy_range , scale , opt.contains("monochrome") , 
				Gv::Timezone(ribbon_tz) , skip , sleep_ms , opt.contains("stopped") , opt.contains("loop") ,
				fade_timeout_ms , fade_fine ) ;

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

/// \file fileplayer.cpp

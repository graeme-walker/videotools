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
// viewer.cpp
// 
// Reads images from a publication channel and displays them in a window. 
//
// The viewer is also used when a parent process such as a the `vt-fileplayer`
// needs a display window, and in that case the video is sent over a
// private pipe rather than a publication channel and the two file descriptors
// are passed on the command-line.
//
// If there is no windowing system then the video images are displayed in the 
// terminal using text characters for pixels. It looks better if you stand
// back and squint.
//
// Mouse events can be sent to a publication channel by using the `--channel` 
// option. The `vt-maskeditor` and `vt-fileplayer` programs use this feature, and 
// they have a `--viewer-channel` option to override the name of the publication 
// channel used for these mouse events.
//
// A pbm-format mask file can be used to dim fixed regions of the image.
// The mask is stretched as necessary to fit the image.
//
// usage: viewer [<options>] { <input-channel> | <shmem-fd> <pipe-fd> }
//

#include "gdef.h"
#include "gvviewerinput.h"
#include "gvviewerwindow.h"
#include "gvimageoutput.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "gfile.h"
#include "gtimerlist.h"
#include "geventloop.h"
#include "geventhandler.h"
#include "gcleanup.h"
#include "garg.h"
#include "genvironment.h"
#include "ggetopt.h"
#include "gassert.h"
#include "glogoutput.h"
#include <exception>
#include <iostream>
#include <signal.h> // signal(2)

namespace Gv
{
	class Viewer ;
	struct ViewerConfig ;
}

struct Gv::ViewerConfig
{
	ViewerConfig() : q_for_quit(true) {}
	bool q_for_quit ;
} ;

/// \class Viewer
/// An image viewer gui class.
///
class Gv::Viewer : public Gv::ViewerInput::Handler , public Gv::ViewerWindow::Handler
{
public:
	Viewer( const ViewerConfig & , const ViewerWindowConfig & , const ViewerInputConfig & , 
		const std::string & input_channel , const std::string & event_output_channel , 
		int shmem_fd , int pipe_fd ) ;
	~Viewer() ;
	virtual void onInput( int , int , int , const char * p , size_t ) override ;
	virtual void onChar( char ) override ;
	virtual void onInvalid() override ;
	virtual void onMouseButtonDown( int , int , bool , bool ) override ;
	virtual void onMouseButtonUp( int , int , bool , bool ) override ;
	virtual void onMouseMove( int , int ) override ;

private:
	void display( int dx , int dy , int channels , const char * p , size_t n ) ;
	void send( const std::string & event , int x , int y ) ;

private:
	ViewerConfig m_config ;
	ViewerWindowConfig m_window_config ;
	unique_ptr<ViewerWindow> m_window ;
	unique_ptr<ViewerInput> m_input ;
	Gv::ImageOutput m_event_output ;
	bool m_mouse_button_down ;
	int m_mouse_button_down_x ;
	int m_mouse_button_down_y ;
	bool m_mouse_button_down_shift ;
	bool m_mouse_button_down_control ;
} ;

// ==

Gv::Viewer::Viewer( const ViewerConfig & viewer_config , const ViewerWindowConfig & window_config , 
	const ViewerInputConfig & input_config , const std::string & input_channel , 
	const std::string & event_output_channel , int shmem_fd , int pipe_fd ) :
		m_config(viewer_config) ,
		m_window_config(window_config) ,
		m_mouse_button_down(false) ,
		m_mouse_button_down_x(-1) ,
		m_mouse_button_down_y(-1) ,
		m_mouse_button_down_shift(false) ,
		m_mouse_button_down_control(false)
{
	m_input.reset( new ViewerInput( *this , input_config , input_channel , shmem_fd , pipe_fd ) ) ;

	if( !event_output_channel.empty() )
	{
		G::Item info = G::Item::map() ;
		info.add( "images" , input_channel ) ;
		m_event_output.startPublisher( event_output_channel , info ) ;
	}
}

Gv::Viewer::~Viewer()
{
}

void Gv::Viewer::onInput( int dx , int dy , int channels , const char * p , size_t n )
{
	G_DEBUG( "Gv::Viewer::onInput: " << dx << " " << dy << " " << channels << " " << n ) ;
	if( m_window.get() == nullptr )
	{
		// create the window, now we know the size
		m_window.reset( ViewerWindow::create( *this , m_window_config , dx , dy ) ) ;
		m_window->init() ;
	}
	display( dx , dy , channels , p , n ) ;
}

void Gv::Viewer::display( int dx , int dy , int channels , const char * p , size_t n )
{
	m_window->display( dx , dy , channels , p , n ) ;
}

void Gv::Viewer::onChar( char c )
{
	if( c == 'q' && m_config.q_for_quit )
		GNet::EventLoop::instance().quit("quit") ;
}

void Gv::Viewer::onMouseButtonDown( int x , int y , bool shift , bool control )
{
	G_DEBUG( "Gv::Viewer::onMouseButtonDown: button down" ) ;
	m_mouse_button_down = true ;
	m_mouse_button_down_x = x ;
	m_mouse_button_down_y = y ;
	m_mouse_button_down_shift = shift ;
	m_mouse_button_down_control = control ;
	send( "down" , x , y ) ;
}

void Gv::Viewer::onMouseButtonUp( int x , int y , bool shift , bool control )
{
	G_DEBUG( "Gv::Viewer::onMouseButtonUp: button up" ) ;
	m_mouse_button_down = false ;
	send( "up" , x , y ) ;
	m_mouse_button_down_x = -1 ;
	m_mouse_button_down_y = -1 ;
	m_mouse_button_down_shift = false ;
	m_mouse_button_down_control = false ;
}

void Gv::Viewer::onMouseMove( int x , int y )
{
	//if( m_mouse_button_down ) // moot
		send( "move" , x , y ) ;
}

void Gv::Viewer::send( const std::string & event , int x , int y )
{
	G_ASSERT( m_window.get() != nullptr ) ;
	std::ostringstream ss ;
	ss
			<< "{ "
			<< "'app': 'viewer', "
			<< "'version': 1, "
			<< "'pid': " << ::getpid() << ", "
			<< "'time': " << G::DateTime::now() << ", "
			<< "'event': '" << event << "', "
			<< "'button': '" << (m_mouse_button_down?"down":"up") << "', "
			<< "'shift': " << (m_mouse_button_down_shift?1:0) << ", "
			<< "'control': " << (m_mouse_button_down_control?1:0) << ", "
			<< "'x': " << x << ", "
			<< "'y': " << y << ", "
			<< "'dx': " << m_window->dx() << ", "
			<< "'dy': " << m_window->dy() << ", "
			<< "'x0': " << m_mouse_button_down_x << ", "
			<< "'y0': " << m_mouse_button_down_y << ", "
			<< "}" ;
	std::string s = ss.str() ;
	m_event_output.sendText( s.data() , s.size() , "application/json" ) ;
}

void Gv::Viewer::onInvalid()
{
	// the window needs the current image to refresh itself
	display( m_input->dx() , m_input->dy() , m_input->channels() , m_input->data() , m_input->size() ) ;
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
			"c!channel!publish interaction events to the named channel!!1!channel!1" "|"
			"s!scale!reduce the image size!!1!divisor!1" "|"
			"S!static!view only the first image!!0!!1" "|"
			"M!mask!use a mask file!!1!file!1" "|"
			"t!title!sets the window title!!1!title!1" "|"
			"q!quit!quit on q keypress!!0!!1" "|"
		) ;
		std::string args_help = "{ <channel> | <shmem-fd> <pipe-fd> }" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 2U || opt.args().c() == 3U ) ;
		try
		{
			std::string input_channel = opt.args().c() == 2U ? opt.args().v(1U) : std::string() ;
			int shmem_fd = opt.args().c() == 3U ? static_cast<int>( G::Str::toUInt( opt.args().v(1U) ) ) : -1 ;
			int pipe_fd = opt.args().c() == 3U ? static_cast<int>( G::Str::toUInt( opt.args().v(2U) ) ) : -1 ;
			int scale = G::Str::toInt(opt.value("scale","1")) ;
			std::string title = opt.value("title") ;

			// dont allow daemonisation to close our fat-pipe file descriptors
			if( opt.contains("daemon") && ( shmem_fd >= 0 || pipe_fd >= 0 ) )
				throw std::runtime_error( "do not use \"--daemon\" when passing file descriptors" ) ;

			Gv::ViewerWindowConfig window_config ;
			window_config.m_title = title.empty() ? input_channel : title ;
			window_config.m_mask_file = opt.value("mask") ;
			window_config.m_mouse_moves = opt.contains("channel") ;

			Gv::ViewerInputConfig input_config ;
			input_config.m_static = opt.contains("static") ;
			input_config.m_scale = scale ;
			input_config.m_rate_limit = G::EpochTime(0,70000) ; // 70ms => ~14fps

			Gv::ViewerConfig viewer_config ;
			viewer_config.q_for_quit = opt.contains("quit") || !G::Environment::get("VIEWER_Q_FOR_QUIT",std::string()).empty() ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			Gv::Viewer viewer( viewer_config , window_config , input_config , input_channel , opt.value("channel","") , shmem_fd , pipe_fd ) ;

			startup.start() ;

			if( input_channel.empty() )
				::signal( SIGINT , SIG_IGN ) ; // ignore ^C when fork()ed and exec()ed

			G::Cleanup::atexit( true ) ; // in case xlib calls exit()
			std::string e = event_loop->run() ;
			G::Cleanup::atexit( false ) ;
		}
		catch( std::exception & e )
		{
			startup.report( arg.prefix() , e ) ;
			throw ;
		}
		return EXIT_SUCCESS ;
	}
	catch( Gv::ViewerInput::Closed & )
	{
		// no-op
	}
	catch( Gv::Exit & e )
	{
		// no-op
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
	}
	G::Cleanup::atexit( false ) ;
	return EXIT_FAILURE ;
}


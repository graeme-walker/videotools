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
// maskeditor.cpp
//
// Creates and edits a mask file that can be used by the `vt-watcher` program.
//
// The `vt-watcher` program analyses a video stream for moving objects, and it uses
// a mask file to tell it what parts of the image to ignore.
//
// The mask is edited over the top a background image, and that image can come 
// from a static image file or from a publication channel (`--image-channel`).
//
// Click and drag the mouse to create masked areas, and hold the shift key to
// revert. The mask file is continuously updated in-place; there is no "save" 
// operation.
//
// usage: maskeditor [<options>] {--image-file=<image> | --image-channel=<channel>} <mask-file>
// 

#include "gdef.h"
#include "gnet.h"
#include "gvimageoutput.h"
#include "gvviewerevent.h"
#include "grimageconverter.h"
#include "grimage.h"
#include "glocalsocket.h"
#include "gpublisher.h"
#include "gvmask.h"
#include "gtimer.h"
#include "geventloop.h"
#include "geventhandler.h"
#include "gfile.h"
#include "genvironment.h"
#include "gexception.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "garg.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "gassert.h"
#include <exception>
#include <fstream>
#include <iostream>

class MaskEditor : private Gv::ViewerEventMixin , private GNet::EventExceptionHandler
{
public:
	MaskEditor( std::string mask_file , std::string viewer_channel , Gr::Image background_image ) ;
	void run() ;

private:
	MaskEditor( const MaskEditor & ) ;
	void operator=( const MaskEditor & ) ;
	virtual void onViewerEvent( Gv::ViewerEvent::Info ) override ;
	virtual void onException( std::exception & ) override ;
	void onButtonUp( int x , int y , bool shift , bool control ) ;
	void onButtonDown( int x , int y , bool shift , bool control ) ;
	void onDrag( int x , int y , int , int ) ;
	void sendImage() ;
	void onUpdateTimeout() ;

private:
	Gr::Image m_image ;
	Gr::Image m_image_out ;
	Gv::ImageOutput m_viewer_output ;
	std::string m_mask_file ;
	Gv::Mask m_mask ;
	GNet::Timer<MaskEditor> m_update_timer ;
	unsigned int m_update_timeout_ms ;
} ;

MaskEditor::MaskEditor( std::string mask_file , std::string viewer_channel , Gr::Image background_image ) :
	Gv::ViewerEventMixin(viewer_channel) ,
	m_image(background_image) ,
	m_mask_file(mask_file) ,
	m_mask(background_image.type().dx(),background_image.type().dy(),mask_file,true) ,
	m_update_timer(*this,&MaskEditor::onUpdateTimeout,*this) ,
	m_update_timeout_ms(500U)
{
	std::string viewer_title = G::Path(mask_file).basename() ;
	m_viewer_output.startViewer( viewer_title , 1U , viewer_channel ) ;
	sendImage() ;
	m_update_timer.startTimer( 0 , m_update_timeout_ms*1000U ) ;
}

void MaskEditor::onException( std::exception & )
{
	throw ;
}

void MaskEditor::sendImage()
{
	G_ASSERT( m_image.type().isRaw() ) ;
	G_ASSERT( m_image.type().channels() == 3 ) ;

	int dx = m_image.type().dx() ;
	int dy = m_image.type().dy() ;

	Gr::ImageDataWrapper in( m_image.data() , dx , dy , 3 ) ;
	Gr::ImageBuffer & out_buffer = *Gr::Image::blank(m_image_out,m_image.type()) ;
	Gr::ImageData out( out_buffer , dx , dy , 3 ) ;

	for( int y = 0 ; y < m_image.type().dy() ; y++ )
	{
		const unsigned char * p_in = in.row( y ) ;
		unsigned char * p_out = out.row( y ) ;
		for( int x = 0 ; x < m_image.type().dx() ; x++ )
		{
			bool masked = m_mask.masked( x , y ) ;
			for( int c = 0 ; c < 3 ; c++ )
			{
				unsigned int rgb = static_cast<unsigned char>(*p_in++) ;
				if( masked && c == 0 )
					rgb = rgb / 2U ;
				else if( masked )
					rgb = rgb / 8U ;
				*p_out++ = static_cast<char>(rgb) ;
			}
		}
	}

	m_viewer_output.send( out_buffer , m_image.type() ) ;
}

void MaskEditor::onViewerEvent( Gv::ViewerEvent::Info info )
{
	G_DEBUG( "MaskEditor::onViewerEvent" ) ;
	if( info.type == Gv::ViewerEvent::Down )
		onButtonDown( info.x , info.y , info.shift , info.control ) ;
	else if( info.type == Gv::ViewerEvent::Drag )
		onDrag( info.x , info.y , info.x_down , info.y_down ) ;
	else if( info.type == Gv::ViewerEvent::Up )
		onButtonUp( info.x , info.y , info.shift , info.control ) ;
}

void MaskEditor::onButtonUp( int x , int y , bool shift , bool control )
{
	G_LOG( "MaskEditor::onButtonUp: " << x << " " << y ) ;
	m_mask.up( x , y , shift , control ) ;
}

void MaskEditor::onButtonDown( int x , int y , bool shift , bool control )
{
	G_LOG( "MaskEditor::onButtonDown: " << x << " " << y << " " << shift << " " << control ) ;
	m_mask.down( x , y , shift , control ) ;
}

void MaskEditor::onDrag( int x , int y , int x_down , int y_down )
{
	G_LOG( "MaskEditor::onDrag: " << x << " " << y ) ;
	m_mask.move( x , y ) ;
}

void MaskEditor::onUpdateTimeout()
{
	sendImage() ; // also detects the viewer going away
	m_mask.write( m_mask_file ) ; // keep the file up-to-date
	m_update_timer.startTimer( 0 , m_update_timeout_ms*1000U ) ;
}

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
			"i!image-file!background image file!!1!path!1" "|"
			"c!image-channel!channel for capturing the initial backgound image!!1!channel!1" "|"
			"W!viewer-channel!override the default viewer channel name!!1!channel!1" "|"
		) ;
		std::string args_help = "<mask-file>" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 2U ) ;
		try
		{
			std::string mask_file = opt.args().v(1U) ;
			std::string image_file = opt.value("image-file","") ;
			std::string image_channel = opt.value("image-channel","") ;
			std::string viewer_channel = opt.value("viewer-channel","maskeditor-viewer."+G::Str::fromInt(static_cast<int>(::getpid()))) ;

			if( G::Environment::get("DISPLAY","").empty() )
			{
				G_WARNING( "maskeditor: no DISPLAY environment variable: mask editing requires X11" ) ;
				sleep( 1 ) ;
			}

			if( image_file.empty() && image_channel.empty() )
				throw std::runtime_error( "please specify a background image with \"--image-file\" or \"--image-channel\"" ) ;

			// obtain a background image
			Gr::Image background_image ;
			if( !image_file.empty() )
			{
				std::ifstream file( image_file.c_str() ) ;
				Gr::Image::read( file , background_image , image_file ) ;
				if( !background_image.valid() )
					throw std::runtime_error( "cannot read image file [" + image_file + "]" ) ;
			}
			else
			{
				try
				{
					G::PublisherSubscription channel( image_channel ) ;
					std::string type_str ;
					if( ! Gr::Image::peek( channel , background_image , type_str ) )
						throw std::runtime_error( "no image available" ) ;
				}
				catch( std::exception & e )
				{
					throw std::runtime_error( "failed to capture initial background image "
						"from channel [" + image_channel + "]: " + std::string(e.what()) ) ;
				}
			}

			std::cout << arg.prefix() << ": background image: " << "type=[" << background_image.type() << "]\n" ;

			// decode the background image into a raw format
			Gr::ImageConverter converter ;
			if( !converter.toRaw(background_image,background_image) || background_image.type().channels() != 3 )
				throw std::runtime_error( "invalid background image" + (image_file.empty()?(" file ["+image_file+"]"):std::string()) ) ;

			// check/create the mask file
			if( G::File::exists(mask_file) )
			{
				Gv::Mask mask( background_image.type().dx() , background_image.type().dy() , mask_file ) ; // check readable
				mask.write( mask_file ) ; // check writable
			}
			else
			{
				std::cout 
					<< arg.prefix() << ": creating mask file [" << mask_file << "] "
					<< "(" << background_image.type().dx() << "x" << background_image.type().dy() << ")\n" ;
				Gv::Mask mask( background_image.type().dx() , background_image.type().dy() ) ;
				mask.write( mask_file ) ; // create
			}

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			MaskEditor mask_editor( mask_file , viewer_channel , background_image ) ;

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

/// \file maskeditor.cpp

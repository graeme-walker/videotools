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
// watcher.cpp
//
// Performs motion detection on a video stream received over a shared-memory 
// publication channel.
//
// If motion detection exceeds some threshold then an event is published on the 
// `--event-channel` channel as a json formatted text message. These messages are 
// normally read by a `vt-alarm` process that can run other external programs.
//
// Motion detection events can also be used to control a `vt-recorder` process
// by sending a `fast` command to the recorder's command socket.
//
// The `--viewer` or `--image-channel` options can be used to visualise the 
// motion detection. The viewer window shows in green the pixels that change in 
// brightness by more than the "squelch" value, and it also shows in red the 
// pixels that are ignored as the result of the mask.
//
// The motion detection algorithm is extremely simple; one video frame is 
// compared to the next, and pixels that change their brightness more that the 
// squelch value (0..255) are counted up, and if the total count is more than 
// the given threshold value then a motion detection event is emitted.
//
// The `--scale` option also has an important effect because it increases the
// size of the motion detection pixels (by decreasing the resolution of the 
// image), and it affects the interpretation of the threshold value.
//
// Histogram equalisation is enabled with the `--equalise` option and this can
// be particularly useful for cameras that loose contrast in infra-red mode.
//
// The command socket, enabled with the `--command-socket` option, accepts
// `squelch`, `threshold` and `equalise` commands. Multiple commands can
// be sent in one datagram by using semi-colon separators.
//
// The squelch and threshold values have default values that will need
// fine-tuning for your camera. This can be done interactively by temporarily
// using the `--verbose`, `--viewer` and `--command-socket` options, and then 
// sending `squelch`, `threshold` and `equalise` commands to the command socket
// with the `vt-socket` tool (eg. `vt-watcher -C cmd --viewer -v <channel>`
// and `vt-socket cmd squelch=20\;threshold=1\;equalise=off`).
// Adjust the squelch value to eliminate the green highlights when there is 
// no motion. Repeat in different lighting conditions.
//
// usage: watcher [<options>] [--viewer] [--event-channel <event-channel-out>] <video-channel-in>
//

#include "gdef.h"
#include "gnet.h"
#include "geventloop.h"
#include "gtimerlist.h"
#include "gtimer.h"
#include "gvimageoutput.h"
#include "gvcommandsocket.h"
#include "gvmask.h"
#include "gvstartup.h"
#include "gvcommandsocket.h"
#include "gvexit.h"
#include "grimageconverter.h"
#include "grimagebuffer.h"
#include "grimage.h"
#include "grhistogram.h"
#include "gpublisher.h"
#include "gmsg.h"
#include "groot.h"
#include "gfile.h"
#include "gstr.h"
#include "garg.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "gtest.h"
#include "gassert.h"
#include <exception>
#include <iostream>
#include <sstream>

struct DiffParameters
{
	G::EpochTime m_interval ;
	unsigned int m_decoder_scale ;
	unsigned int m_squelch ;
	unsigned int m_threshold ;
	int m_log_threshold ;
	bool m_equalise ;
	DiffParameters( G::EpochTime interval , unsigned int decoder_scale , unsigned int squelch , 
		unsigned int threshold , int log_threshold , bool equalise ) :
			m_interval(interval) ,
			m_decoder_scale(decoder_scale) ,
			m_squelch(squelch) ,
			m_threshold(threshold) ,
			m_log_threshold(log_threshold) ,
			m_equalise(equalise)
	{
	}
} ;

struct DiffInfo
{
	unsigned int m_dx ;
	unsigned int m_dy ;
	unsigned int m_count ; // number of unmasked pixels changed more than squelch
	unsigned int m_noise ; // number of unmasked pixels changed less than squelch
	DiffInfo() :
		m_dx(0) ,
		m_dy(0) ,
		m_count(0U) ,
		m_noise(0U)
	{
	}
	DiffInfo( unsigned int dx , unsigned int dy ) :
		m_dx(dx) ,
		m_dy(dy) ,
		m_count(0U) ,
		m_noise(0U)
	{
	}
	bool valid() const
	{
		return m_dx && m_dy ;
	}
} ;

class Comparator
{
public:
	Comparator( const std::string & mask_file , bool plain , bool equalise ) ;
	DiffInfo apply( Gr::Image , const std::string & , const DiffParameters & ) ;
	Gr::Image image() const ;
	G::EpochTime maskTime() const ;

private:
	void equalise( Gr::ImageType , const Gr::ImageBuffer & , Gr::ImageBuffer & , const Gv::Mask & ) ;
	DiffInfo compare( const DiffParameters & params , Gr::Image image_new ) ;
	static DiffInfo compareImp( const DiffParameters & , int dx , int dy , 
		const Gr::ImageDataWrapper & , const Gr::ImageDataWrapper & , Gr::ImageData & ,
		const Gv::Mask & , bool plain ) ;

private:
	Gr::ImageConverter m_converter ;
	Gr::Histogram m_histogram ;
	Gr::Image m_image_in ;
	Gr::Image m_image_raw ;
	Gr::Image m_image_eq ;
	Gr::Image m_image_old ;
	Gr::Image m_image_out ;
	Gr::ImageType m_image_in_type ;
	std::string m_type_str ;
	std::string m_mask_file ;
	unique_ptr<Gv::Mask> m_mask ;
	G::EpochTime m_mask_time ;
	bool m_plain ;
} ;

class Watcher : private GNet::EventHandler , private Gv::CommandSocketMixin
{
public:
	Watcher( const std::string & input_channel , const std::string & event_channel , 
		const std::string & images_channel , const std::string & command_socket ,
		const std::string & recorder_socket_path , bool viewer , 
		const std::string & viewer_title , int decoder_scale , const std::string & mask_file , 
		bool once , unsigned int interval , unsigned int squelch , unsigned int threshold , 
		int log_threshold , bool equalise , bool plain , unsigned int repeat_timeout ) ;

private:
	void emitImage( Gr::Image ) ;
	void emitChangesEvent( G::EpochTime , const DiffInfo & ) ;
	void emitStartEvent() ;
	void emitEventText( std::string ) ;
	void emitRecorderCommand( G::EpochTime , const DiffInfo & ) ;
	static void set( G::Item & , const std::string & , const std::string & ) ;
	void onReopenTimeout() ;
	void onRepeatTimeout() ;
	virtual void readEvent() override ;
	virtual void onException( std::exception & ) override ;
	virtual void onCommandSocketData( std::string ) override ; // Gv::CommandSocketMixin

private:
	Comparator m_comparator ;
	DiffParameters m_params ;
	std::string m_input_channel_name ;
	G::PublisherSubscription m_input_channel ;
	std::string m_mask_file ;
	Gv::ImageOutput m_output_viewer ;
	Gv::ImageOutput m_output_images ;
	Gv::ImageOutput m_output_events ;
	std::string m_recorder_socket_path ;
	Gv::CommandSocket m_recorder_socket ;
	bool m_publish_images ;
	bool m_once ;
	unsigned int m_image_number ;
	G::EpochTime m_interval_start_time ;
	Gr::Image m_image_in ;
	std::string m_image_type_str ;
	GNet::Timer<Watcher> m_reopen_timer ;
	unsigned int m_repeat_timeout ;
	std::string m_repeat_text ;
	unsigned int m_repeat_count ;
	GNet::Timer<Watcher> m_repeat_timer ;
} ;

// ==

Watcher::Watcher( const std::string & input_channel , const std::string & event_channel , 
	const std::string & images_channel , const std::string & command_socket ,
	const std::string & recorder_socket_path , bool viewer , 
	const std::string & viewer_title , int decoder_scale , const std::string & mask_file , 
	bool once , unsigned int interval , unsigned int squelch , unsigned int threshold ,
	int log_threshold , bool equalise , bool plain , unsigned int repeat_timeout ) :
		Gv::CommandSocketMixin(command_socket) ,
		m_comparator(mask_file,plain,equalise) ,
		m_params(G::EpochTime(0,interval*1000U),decoder_scale,squelch,threshold,log_threshold,equalise) ,
		m_input_channel_name(input_channel) ,
		m_input_channel(m_input_channel_name,!once/*lazy*/) ,
		m_mask_file(mask_file) ,
		m_output_viewer(*this) ,
		m_recorder_socket_path(recorder_socket_path) ,
		m_publish_images(!images_channel.empty()) ,
		m_once(once) ,
		m_image_number(0U) ,
		m_interval_start_time(0) ,
		m_reopen_timer(*this,&Watcher::onReopenTimeout,*this) ,
		m_repeat_timeout(repeat_timeout) ,
		m_repeat_count(0U) ,
		m_repeat_timer(*this,&Watcher::onRepeatTimeout,*this)
{
	if( m_once && !m_recorder_socket_path.empty() )
	{
		// fail early -- check recorder socket is available -- throw on error
		m_recorder_socket.connect( m_recorder_socket_path ) ;
	}
	if( viewer )
	{
		m_output_viewer.startViewer( viewer_title ) ;
	}
	if( !event_channel.empty() )
	{
		G::Item info = G::Item::map() ;
		set( info , "recorder" , m_recorder_socket_path ) ;
		set( info , "images" , images_channel ) ;
		m_output_events.startPublisher( event_channel , info ) ;
		emitStartEvent() ;
	}
	if( !images_channel.empty() )
	{
		G::Item info = G::Item::map() ;
		set( info , "recorder" , m_recorder_socket_path ) ;
		set( info , "events" , event_channel ) ;
		m_output_images.startPublisher( images_channel , info ) ;
	}

	if( !m_once )
	{
		m_reopen_timer.startTimer( 0U ) ;
	}

	if( m_input_channel.fd() != -1 )
		GNet::EventLoop::instance().addRead( GNet::Descriptor(m_input_channel.fd()) , *this ) ;
}

void Watcher::set( G::Item & item , const std::string & key , const std::string & value )
{
	if( !value.empty() )
		item.add( key , value ) ;
}

void Watcher::onReopenTimeout()
{
	G_DEBUG( "Watcher::onReopenTimeout: trying to open channel [" << m_input_channel_name << "]" ) ;
	if( m_input_channel.open() )
	{
		GNet::EventLoop::instance().addRead( GNet::Descriptor(m_input_channel.fd()) , *this ) ;
	}
	else
	{
		G_WARNING_ONCE( "Watcher::onReopenTimeout: failed to open input channel [" << m_input_channel_name << "]" ) ;
		m_reopen_timer.startTimer( 1U ) ;
	}
}

void Watcher::onRepeatTimeout()
{
	m_repeat_count++ ;
	G_DEBUG( "Watcher::onRepeatTimeout: repeat " << m_repeat_count ) ;
	std::string text = m_repeat_text ;
	G::Str::replaceAll( text , "_""_repeat_""_" , G::Str::fromUInt(m_repeat_count) ) ;
	m_output_events.sendText( text.data() , text.size() , "application/json" ) ;
	m_repeat_timer.startTimer( m_repeat_timeout ) ;
}

void Watcher::readEvent()
{
	G_DEBUG( "Watcher::readEvent: read event on channel [" << m_input_channel_name << "]" ) ;

	// read the image
	if( !Gr::Image::receive( m_input_channel , m_image_in , m_image_type_str ) )
	{
		if( m_once )
		{
			throw std::runtime_error( "publisher has gone away" ) ;
		}
		else
		{
			G_WARNING( "Watcher::readEvent: the image channel has gone away: [" << m_input_channel_name << "]" ) ;
			GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_input_channel.fd()) ) ;
			m_input_channel.close() ;
			m_reopen_timer.startTimer( 1U ) ;
		}
		return ;
	}

	// only compare if the old image is old enough
	//
	G::EpochTime now = G::DateTime::now() ;
	G::EpochTime interval = now - m_interval_start_time ;
	if( interval < m_params.m_interval )
		return ;
	m_interval_start_time = now ;

	// compare
	DiffInfo diff_info = m_comparator.apply( m_image_in , m_image_type_str , m_params ) ;
	if( diff_info.valid() )
	{
		// publish the results
		emitImage( m_comparator.image() ) ;
		emitChangesEvent( now , diff_info ) ;
		emitRecorderCommand( now , diff_info ) ;
	}
}

void Watcher::emitImage( Gr::Image image )
{
	m_output_viewer.send( image.data() , image.type() ) ;
	if( m_publish_images )
		m_output_images.send( image.data() , image.type() ) ;
}

void Watcher::emitChangesEvent( G::EpochTime now , const DiffInfo & diff_info )
{
	const bool emit_event = diff_info.m_count >= m_params.m_threshold ;
	const bool emit_log = m_params.m_log_threshold >= 0 && diff_info.m_count >= static_cast<unsigned int>(m_params.m_log_threshold) ;

	if( emit_event || emit_log )
	{
		std::ostringstream ss ;
		ss
			<< "{ "
			<< "'app': 'watcher', "
			<< "'version': 1, "
			<< "'pid': " << ::getpid() << ", "
			<< "'time': " << now.s << ", "
			<< "'event': 'changes', "
			<< "'squelch': " << m_params.m_squelch << ", "
			<< "'threshold': " << m_params.m_threshold << ", "
			<< "'equalise': " << static_cast<int>(m_params.m_equalise) << ", "
			<< "'mask': '" << m_mask_file << "', "
			<< "'masktime': '" << m_comparator.maskTime() << "', "
			<< "'dx': " << diff_info.m_dx << ", "
			<< "'dy': " << diff_info.m_dy << ", "
			<< "'count': " << diff_info.m_count << ", "
			<< "'repeat': _""_repeat_""_ "
			<< "}" ;
		std::string s = ss.str() ;
		if( emit_log )
			G_LOG_S( "Watcher::emitChangesEvent: event: " << s ) ;
		if( emit_event )
			emitEventText( s ) ;
	}
}

void Watcher::emitStartEvent()
{
	G::EpochTime now = G::DateTime::now() ;
	std::ostringstream ss ;
	ss
		<< "{ "
		<< "'app': 'watcher', "
		<< "'version': 1, "
		<< "'pid': " << ::getpid() << ", "
		<< "'time': " << now.s << ", "
		<< "'event': 'startup', "
		<< "'squelch': " << m_params.m_squelch << ", "
		<< "'threshold': " << m_params.m_threshold << ", "
		<< "'mask': '" << m_mask_file << "', "
		<< "'masktime': '" << m_comparator.maskTime() << "', "
		<< "'repeat': _""_repeat_""_ "
		<< "}" ;
	emitEventText( ss.str() ) ;
}

void Watcher::emitEventText( std::string text )
{
	G::Str::replaceAll( text , "'" , "\"" ) ;
	G::Str::trim( text , "\n" ) ;

	m_repeat_count = 0U ;
	m_repeat_text = text ;
	G::Str::replaceAll( text , "_""_repeat_""_" , G::Str::fromUInt(m_repeat_count) ) ;
	if( m_repeat_timeout != 0U )
		m_repeat_timer.startTimer( m_repeat_timeout ) ;

	G_LOG( "Watcher::run: event: " << text ) ;
	m_output_events.sendText( text.data() , text.size() , "application/json" ) ;
}

void Watcher::emitRecorderCommand( G::EpochTime , const DiffInfo & diff_info )
{
	if( diff_info.m_count >= m_params.m_threshold && !m_recorder_socket_path.empty() )
	{
		m_recorder_socket.close() ;
		std::string reason = m_recorder_socket.connect( m_recorder_socket_path , Gv::CommandSocket::NoThrow() ) ;
		if( reason.empty() )
			G::Msg::send( m_recorder_socket.fd() , "fast" , 4 , 0 ) ;
		else
			G_WARNING( "Watcher::emitRecorderCommand: cannot connect to recorder socket [" << m_recorder_socket_path << "]: " << reason ) ;
	}
}

void Watcher::onCommandSocketData( std::string line )
{
	G_DEBUG( "Watcher::onCommandSocketData: command: [" << G::Str::printable(line) << "]" ) ;
    G::StringArray commands ;
    G::Str::splitIntoFields( line , commands , ";\n" , '\\' ) ; // allow multiple commands in one datagram
    for( G::StringArray::iterator p = commands.begin() ; p != commands.end() ; ++p )
    {
		std::string command = *p ;
		G::Str::replace( command , "=" , " " ) ;
		G::StringArray words = G::Str::splitIntoTokens( command ) ;
		words.push_back( std::string() ) ;
		words.push_back( std::string() ) ;
		if( words[0] == "threshold" && G::Str::isUInt(words[1]) ) 
			m_params.m_threshold = G::Str::toUInt( words[1] ) ;
		else if( words[0] == "squelch" && G::Str::isUInt(words[1]) ) 
			m_params.m_squelch = G::Str::toUInt( words[1] ) ;
		else if( ( words[0] == "equalise" || words[0] == "equalize"/*:-(*/ ) && !words[1].empty() )
			m_params.m_equalise = G::Str::isPositive( words[1] ) ;
		else
			G_WARNING( "Watcher::onCommandSocketData: invalid command: [" << G::Str::printable(*p) << "]" ) ;
	}
}

void Watcher::onException( std::exception & )
{
	throw ;
}

// ==

Comparator::Comparator( const std::string & mask_file , bool plain , bool equalise ) :
	m_histogram(equalise) ,
	m_mask_file(mask_file) ,
	m_mask_time(0) ,
	m_plain(plain)
{
}

Gr::Image Comparator::image() const
{
	return m_image_out ;
}

G::EpochTime Comparator::maskTime() const
{
	return m_mask_time ;
}

DiffInfo Comparator::apply( Gr::Image image_in , const std::string & type_str , const DiffParameters & params )
{
	// decode to monochrome and subsample to a smaller raw image
	if( ! m_converter.toRaw( image_in , m_image_raw , params.m_decoder_scale , true ) )
	{
		G_WARNING( "Watcher:run: invalid image [" << image_in.type() << "]" ) ;
		return DiffInfo() ;
	}

	// update the mask, ensuring it fits the image
	bool first = m_image_in_type != image_in.type() ; // first image or size change
	if( first )
	{
		G_LOG( "Watcher::run: " << (m_image_in_type.valid()?"change of":"initial") << " image type: [" << image_in.type() << "]" ) ;
		m_image_in_type = image_in.type() ;
		m_mask.reset( new Gv::Mask(m_image_raw.type().dx(),m_image_raw.type().dy(),m_mask_file) ) ; 
		m_mask_time = m_mask->time() ;
	}
	else
	{
		G_ASSERT( m_mask.get() != nullptr ) ;
		if( m_mask->update() )
		{
			G_LOG( "Comparator::apply: mask re-read from [" << m_mask_file << "]" ) ;
			m_mask_time = m_mask->time() ;
		}
	}

	// pre-process the image
	if( params.m_equalise )
	{
		G_ASSERT( m_mask.get() != nullptr ) ;
		Gr::ImageType raw_type = m_image_raw.type() ;
		equalise( raw_type , m_image_raw.data() , *Gr::Image::blank(m_image_eq,raw_type) , *m_mask ) ;
	}

	// compare new with old
	DiffInfo diff_info = first ? DiffInfo() : compare( params , params.m_equalise?m_image_eq:m_image_raw ) ;

	// make new old
	m_image_old = ( params.m_equalise ? m_image_eq : m_image_raw ) ;
	return diff_info ;
}

void Comparator::equalise( Gr::ImageType type , const Gr::ImageBuffer & image_buffer_in , 
	Gr::ImageBuffer & image_buffer_out , const Gv::Mask & mask )
{
	G_ASSERT( type.isRaw() && type.channels() == 1 ) ;
	G_ASSERT( m_histogram.active() ) ; if( !m_histogram.active() ) return ;

	const Gr::ImageDataWrapper image_data_in( image_buffer_in , type.dx() , type.dy() , type.channels() ) ;
	Gr::ImageData image_data_out( image_buffer_out , type.dx() , type.dy() , type.channels() ) ;

	const int dx = type.dx() ;
	const int dy = type.dy() ;

	// build the histogram
	m_histogram.clear() ;
	for( int y = 0 ; y < dy ; y++ )
	{
		const unsigned char * row = image_data_in.row( y ) ;
		for( int x = 0 ; x < dx ; x++ )
		{
			bool masked = mask.masked( x , y ) ;
			if( !masked )
				m_histogram.add( row[x] ) ;
		}
	}
	m_histogram.compute() ;

	// map pixel values according to the histogram
	for( int y = 0 ; y < dy ; y++ )
	{
		const unsigned char * row = image_data_in.row( y ) ;
		for( int x = 0 ; x < dx ; x++ )
		{
			image_data_out.rgb( x , y , m_histogram.map(row[x]) , 0 , 0 ) ;
		}
	}
}

DiffInfo Comparator::compare( const DiffParameters & params , Gr::Image image_new )
{
	G_DEBUG( "Comparator::compare: diff images [" << m_image_old.type() << "] [" << image_new.type() << "] [" << m_image_out.type() << "]" ) ;
	G_ASSERT( m_image_old.type().isRaw() ) ;
	G_ASSERT( m_image_old.type() == image_new.type() ) ;
	G_ASSERT( m_image_old.type().channels() == 1 ) ;

	int dx = m_image_old.type().dx() ;
	int dy = m_image_old.type().dy() ;

	Gr::ImageBuffer * image_out_p = Gr::Image::blank( m_image_out , Gr::ImageType::raw(image_new.type().dx(),image_new.type().dy(),3) ) ;
	Gr::ImageData data_out( *image_out_p , dx , dy , 3 ) ;

	return compareImp( params , dx , dy ,
		Gr::ImageDataWrapper(m_image_old.data(),dx,dy,1) ,
		Gr::ImageDataWrapper(image_new.data(),dx,dy,1) ,
		data_out , *m_mask , m_plain ) ;
}

DiffInfo Comparator::compareImp( const DiffParameters & params , int dx , int dy ,
	const Gr::ImageDataWrapper & data_old , const Gr::ImageDataWrapper & data_new , 
	Gr::ImageData & data_out , const Gv::Mask & mask , bool plain_output )
{
	DiffInfo diff_info( dx , dy ) ;
	for( int y = 0 ; y < dy ; y++ )
	{
		const unsigned char * p_old = data_old.row( y ) ;
		const unsigned char * p_new = data_new.row( y ) ;
		unsigned char * p_out = data_out.row( y ) ;
		for( int x = 0 ; x < dx ; x++ , ++p_old , ++p_new , p_out += 3 )
		{
			unsigned int dluma = *p_old > *p_new ? (*p_old - *p_new) : (*p_new - *p_old) ;
			unsigned int luma_dimmed = (*p_new)/4U ;
			bool masked = mask.masked(x,y) ;

			if( plain_output && masked )
			{
				p_out[0] = 0 ;
				p_out[1] = 0 ;
				p_out[2] = 0 ;
			}
			else if( plain_output )
			{
				p_out[0] = p_new[0] ;
				p_out[1] = p_new[0] ;
				p_out[2] = p_new[0] ;
			}
			else if( masked )
			{
				p_out[0] = luma_dimmed ;
				p_out[1] = 0U ;
				p_out[2] = 0U ;
			}
			else
			{
				// dimmed grey or bright green
				p_out[0] = luma_dimmed ;
				p_out[1] = dluma > params.m_squelch ? 255U : luma_dimmed ;
				p_out[2] = luma_dimmed ;
			}

			if( !masked )
			{
				if( dluma > params.m_squelch )
					diff_info.m_count++ ;
				else
					diff_info.m_noise++ ;
			}
		}
	}

	if( G::Test::enabled("watcher-large-diff-count") )
	{
		diff_info.m_count = 999U ;
	}

	return diff_info ;
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
			"y!syslog!log to syslog!!0!!2" "|"
			"b!daemon!run in the background!!0!!2" "|"
			"u!user!user to switch to when idle if run as root!!1!username!2" "|"
			"P!pid-file!write process id to file!!1!path!2" "|"
			"G!log-threshold!threshold for logging motion detection events!!0!!1" "|"
			"v!verbose!verbose logging!!0!!1" "|"
			"w!viewer!run a viewer!!0!!1" "|"
			"t!viewer-title!viewer window title!!1!title!1" "|"
			"M!mask!use a mask file!!1!file!1" "|"
			"s!scale!reduce the image size!!1!divisor!1" "|"
			"E!event-channel!publish analysis events to the named channel!!1!channel!1" "|"
			"c!image-channel!publish analysis images to the named channel!!1!channel!1" "|"
			"R!recorder!recorder socket path!!1!path!1" "|"
			"S!squelch!pixel value change threshold! (0 to 255) (default 50)!1!luma!1" "|"
			"A!threshold!pixel count threshold! in changed pixels per image (default 100)!1!pixels!1" "|"
			"i!interval!minimum time interval between comparisons! (default 250)!1!ms!1" "|"
			"O!once!exit if the watched channel is unavailable!!0!!1" "|"
			"Q!equalise!histogram equalisation!!0!!1" "|"
			"!equalize!!!0!!0" "|"
			"C!command-socket!socket for update commands!!1!path!1" "|"
			"e!plain!do not show changed-pixel highlights! in output images!0!!1" "|"
			"W!repeat-timeout!send events repeatedly! with the given period!1!seconds!1" "|"
		) ;
		std::string args_help = "<input-channel>" ;
		Gv::Startup startup( opt , args_help , opt.args().c() == 2U ) ;
		try
		{
			std::string input_channel_name = opt.args().v(1U) ;
			bool viewer = opt.contains( "viewer" ) ;
			std::string event_channel = opt.value( "event-channel" ) ;
			std::string images_channel = opt.value( "image-channel" ) ;
			std::string recorder = opt.value( "recorder" ) ;
			std::string viewer_title = opt.value( "viewer-title" , input_channel_name ) ;
			std::string mask_file = opt.value( "mask" ) ;
			bool once = opt.contains( "once" ) ;
			bool equalise = opt.contains( "equalise" ) || opt.contains( "equalize" ) ;
			bool plain = opt.contains( "plain" ) ;
			unsigned int interval = G::Str::toUInt( opt.value("interval","250") ) ;
			unsigned int squelch = G::Str::toUInt( opt.value("squelch","50") ) ;
			unsigned int threshold = G::Str::toUInt( opt.value("threshold","100") ) ;
			int log_threshold = G::Str::toInt( opt.value("log-threshold","-1") ) ;
			int scale = static_cast<int>( G::Str::toUInt(opt.value("scale","1")) ) ;
			std::string command_socket = opt.value("command-socket","") ;
			unsigned int repeat_timeout = G::Str::toUInt( opt.value("repeat-timeout","0") ) ;

			GNet::TimerList timer_list ;
			unique_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;

			Watcher watcher( input_channel_name , event_channel , images_channel ,
				command_socket , recorder , viewer , viewer_title ,
				scale , mask_file , once ,
				interval , squelch , threshold , log_threshold , equalise , plain , 
				repeat_timeout ) ;

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

/// \file watcher.cpp

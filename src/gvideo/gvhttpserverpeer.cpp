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
// gvhttpserverpeer.cpp
//

#include "gdef.h"
#include "gvhttpserverpeer.h"
#include "grjpeg.h"
#include "groot.h"
#include "ghexdump.h"
#include "grimagebuffer.h"
#include "grglyph.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

Gv::HttpServerPeer::HttpServerPeer( GNet::Server::PeerInfo peer_info , Sources & sources , const Resources & resources , const Config & config ) :
	GNet::BufferedServerPeer(peer_info,"\r\n") ,
	m_sources(sources) ,
	m_source(*this) ,
	m_resources(resources) ,
	m_config_default(config) ,
	m_config(config) ,
	m_idle_timer(*this,&HttpServerPeer::onIdleTimeout,*this) ,
	m_data_timer(*this,&HttpServerPeer::onDataTimeout,*this) ,
	m_image_size(0U) ,
	m_sending(0U) ,
	m_image_number(1U) ,
	m_state(s_init) ,
	m_content_length(0U)
{
	if( m_config.idleTimeout() != 0U )
		m_idle_timer.startTimer( m_config.idleTimeout() ) ;
}

Gv::HttpServerPeer::~HttpServerPeer()
{
}

void Gv::HttpServerPeer::onSecure( const std::string & )
{
}

void Gv::HttpServerPeer::selectSource( const std::string & url_path )
{
	if( m_sources.valid(url_path) )
	{
		bool new_source = m_sources.select( url_path , m_source ) ;
		if( new_source )
		{
			m_image_size = 0U ;
			m_image_number = 1U ;
			m_image.clear() ;
			m_data_timer.cancelTimer() ;
			G_DEBUG( "Gv::HttpServerPeer::selectSource: new source: source-name=[" << m_source.name() << "]" ) ;
		}
	}
}

bool Gv::HttpServerPeer::onReceive( const std::string & line )
{ 
	if( m_config.moreVerbose() )
		G_LOG( "Gv::HttpServerPeer::onReceive: rx<<: \"" << G::Str::printable(line) << "\"" ) ;

	if( m_config.idleTimeout() != 0U )
		m_idle_timer.startTimer( m_config.idleTimeout() ) ;

	if( ( m_state == s_init || m_state == s_idle ) && line.find("GET") == 0U )
	{
		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " \t" ) ;
		part.push_back( std::string() ) ;
		if( m_config.moreVerbose() )
			G_LOG( "Gv::HttpServerPeer::onReceive: http request: [" << G::Str::printable(line) << "]" ) ;
		m_url = G::Url(part[1U]) ;

		m_config = m_config_default ;
		m_config.init( m_url ) ;

		m_state = s_got_get ;
		m_content_length = 0U ;
		selectSource( m_url.path() ) ;
		G_DEBUG( "Gv::HttpServerPeer::onReceive: got get" ) ;
	}
	else if( ( m_state == s_init || m_state == s_idle ) && line.find("POST") == 0U )
	{
		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " \t" ) ;
		part.push_back( std::string() ) ;
		m_url = G::Url(part[1U]) ;
		if( m_config.moreVerbose() )
			G_LOG( "Gv::HttpServerPeer::onReceive: http request: [" << G::Str::printable(line) << "]" ) ;

		m_state = s_got_post ;
		m_content_length = 0U ;
		selectSource( m_url.path() ) ;
	}
	else if( m_state == s_got_post && !line.empty() )
	{
		// 'post' header line
		if( G::Str::ifind(line,"Content-Length:") == 0U )
			m_content_length = headerValue( line ) ;
	}
	else if( m_state == s_got_post && m_content_length )
	{
		m_state = s_got_post_headers ;
		expect( m_content_length ) ; // GNet::BufferedServerPeer
		m_content_length = 0U ;
	}
	else if( m_state == s_got_post || m_state == s_got_post_headers )
	{
		m_state = s_idle ;
		if( !line.empty() && m_config.moreVerbose() )
			G_LOG( "Gv::HttpServerPeer::onReceive: got post request body: [" << G::Str::printable(line) << "]" ) ;
		doPost() ;
	}
	else if( m_state == s_got_get && !line.empty() )
	{
		// 'get' header line
		if( G::Str::ifind(line,"Content-Length:") == 0U )
			m_content_length = headerValue( line ) ;
	}
	else if( m_state == s_got_get && m_content_length )
	{
		m_state = s_got_get_headers ;
		expect( m_content_length ) ; // GNet::BufferedServerPeer
		m_content_length = 0U ;
	}
	else if( ( m_state == s_got_get || m_state == s_got_get_headers ) )
	{
		if( !line.empty() && m_config.moreVerbose() )
			G_LOG( "Gv::HttpServerPeer::onReceive: got get request body: [" << G::Str::printable(line) << "]" ) ;

		if( m_url.has("send") ) 
			doGatewayMessage( m_url.parameter("send") ) ;

		if( fileRequest() || specialRequest() )
		{
			m_state = s_file ;
			m_data_timer.startTimer( 0 ) ;
		}
		else if( m_source.get() == nullptr )
		{
			m_state = s_idle ;
			G_DEBUG( "Gv::HttpServerPeer::onReceive: http request for invalid channel" ) ;
			doSendResponse( 404 , "No such channel" ) ;
		}
		else if( m_config.type() == "jpeg" && !Gr::Jpeg::available() )
		{
			m_state = s_idle ;
			G_WARNING( "Gv::HttpServerPeer::onReceive: http request for jpeg but no libjpeg built in" ) ;
			doSendResponse( 415 , "Unsupported media type" ) ; // TODO support http content negotiation
		}
		else if( m_image.empty() )
		{
			m_state = s_waiting ;
			G_DEBUG( "Gv::HttpServerPeer::onReceive: image not yet available: "
				<< "waiting up to " << m_config.firstImageTimeout() << "s" ) ;
			if( m_config.quick() ) m_source.resend() ;
			m_data_timer.startTimer( m_config.firstImageTimeout() ) ;
		}
		else if( m_config.streaming() )
		{
			m_state = s_streaming_first_idle ;
			if( m_config.quick() ) m_source.resend() ;
			m_data_timer.startTimer( 0 ) ;
		}
		else
		{
			m_state = s_single_idle ;
			if( m_config.quick() ) m_source.resend() ;
			m_data_timer.startTimer( 0 ) ;
		}
	}
	else
	{
		G_DEBUG( "Gv::HttpServerPeer::onReceive: ignoring unexpected data in state " << m_state ) ;
	}
	return true ;
}

void Gv::HttpServerPeer::buildPduFirst()
{
	G_ASSERT( !m_image.empty() ) ;
	m_pdu.clear() ;
	m_pdu.append( streamingHeader(m_image_size,m_image.type(),m_image_type_str) ) ;
	m_pdu.append( streamingSubHeader(m_image_size,m_image.type(),m_image_type_str) ) ;
	m_pdu.assignBody( m_image.ptr() , m_image_size ) ;
}

void Gv::HttpServerPeer::buildPdu()
{
	G_ASSERT( !m_image.empty() ) ;
	m_pdu.clear() ;
	m_pdu.append( streamingSubHeader(m_image_size,m_image.type(),m_image_type_str) ) ;
	m_pdu.assignBody( m_image.ptr() , m_image_size ) ;
}

void Gv::HttpServerPeer::buildPduSingle()
{
	G_ASSERT( !m_image.empty() ) ;
	m_pdu.clear() ;
	m_pdu.append( simpleHeader(m_image_size,m_image.type(),m_image_type_str,m_source.name(),m_source.info()) ) ;
	m_pdu.assignBody( m_image.ptr() , m_image_size ) ;
}

bool Gv::HttpServerPeer::sendPdu()
{
	m_sending = 0U ;
	if( m_config.moreVerbose() )
		G_LOG( "Gv::HttpServerPeer::sendPdu: sending image: " << m_pdu.size() << " bytes" ) ;
	bool sent = doSend( m_pdu ) ; 
	if( !sent && m_config.moreVerbose() )
		G_LOG( "Gv::HttpServerPeer::sendPdu: flow control asserted for image " << m_image_number ) ;
	return sent ;
}

bool Gv::HttpServerPeer::doSend( const Pdu & pdu )
{
	doSendLogging( pdu ) ;
	bool all_sent = send( pdu.segments() ) ; // GNet::ServerPeer::send()
	if( !all_sent )
		pdu.lock() ;
	return all_sent ;
}

bool Gv::HttpServerPeer::doSend( const std::string & s )
{
	doSendLogging( Pdu(s) ) ;
	return send( s ) ; // GNet::ServerPeer::send()
}

void Gv::HttpServerPeer::doSendLogging( const Pdu & pdu ) const
{
	if( G::Log::at(G::Log::s_LogVerbose) && m_config.moreVerbose() )
	{
		typedef std::string::const_iterator Ptr ;
		std::string s = pdu.head() ; // just the headers
		Ptr old = s.begin() ;
		Ptr const end = s.end() ;
		for( Ptr pos = std::find(old,end,'\n') ; pos != end ; old=pos+1 , pos = std::find(pos+1,end,'\n') )
		{
			std::string line = pos != s.begin() ? std::string(old,pos) : std::string() ;
			G::Str::trimRight( line , "\r" ) ;
			G_LOG( "Gv::HttpServerPeer::doSend: tx>>: \"" << G::Str::printable(line) << "\"" ) ;
		}
	}
}

void Gv::HttpServerPeer::doPost()
{
	std::string reason = doGatewayMessageImp( m_url.parameter("send") ) ;
	if( reason.empty() )
		doSendResponse( 200 , "OK" ) ;
	else
		doSendResponse( 500 , reason ) ;
}

void Gv::HttpServerPeer::doGatewayMessage( const std::string & send_param )
{
	std::string reason = doGatewayMessageImp( send_param ) ;
	if( !reason.empty() )
		G_WARNING( "Gv::HttpServerPeer::doGatewayMessage: gateway message failed: " << reason ) ;
}

std::string Gv::HttpServerPeer::doGatewayMessageImp( const std::string & send_param )
{
	G_LOG( "Gv::HttpServerPeer::doGatewayMessage: send=[" << send_param << "]" ) ;

	if( send_param.empty() )
		return "no 'send' parameter: nothing to do" ;

	if( m_config.gateway() == GNet::Address::defaultAddress() )
		return "no gateway address configured" ;

	// parse parameter value as "port <url-encoded-message>"
	unsigned int port = G::Str::toUInt( G::Str::head(send_param," ") , "0" ) ;
	if( port == 0U )
		return "incorrect 'send' parameter: no port value" ;

	std::string payload = G::Str::tail( send_param , " " ) ; // already urldecoded by G::Url::parameter()
	if( payload.empty() )
		return "incorrect 'send' parameter: no message part" ;

	// create the socket
	GNet::DatagramSocket socket( m_config.gateway().domain() ) ;

	// send the message
	GNet::Address address( m_config.gateway() ) ; 
	address.setPort( port ) ;
	ssize_t n = socket.writeto( payload.data() , payload.size() , address ) ;
	G_LOG( "Gv::HttpServerPeer::doGatewayMessage: gateway: "
		<< "host=[" << m_config.gateway().hostPartString() << "] " 
		<< "port=[" << port << "] "
		<< "payload=[" << G::Str::printable(payload) << "] " 
		<< "sent=" << n << "/" << payload.size() ) ;
	if( n != static_cast<ssize_t>(payload.size()) )
		return "udp send failed" ;

	return std::string() ;
}

bool Gv::HttpServerPeer::fileRequest()
{
	return m_resources.fileResource( m_url.path() ) ;
}

bool Gv::HttpServerPeer::specialRequest()
{
	return m_resources.specialResource( m_url.path() ) ;
}

void Gv::HttpServerPeer::buildPduFromFile()
{
	std::pair<std::string,std::string> pair = m_resources.fileResourcePair( m_url.path() ) ;
	std::string path = pair.first ;
	std::string type = pair.second ;

	m_pdu.clear() ;
	std::ifstream file ;
	if( !path.empty() )
	{
		G_DEBUG( "Gv::HttpServerPeer::buildPduFromFile: url-path=[" << m_url.path() << "] file-path=[" << path << "] type=[" << type << "]" ) ;
		{
			G::Root claim_root ;
			file.open( path.c_str() ) ;
		}
		if( file.good() )
		{
			// read the whole file
			shared_ptr<Gr::ImageBuffer> image_buffer_ptr( new Gr::ImageBuffer ) ;
			file >> *image_buffer_ptr ;
			if( file.fail() )
			{
				G_WARNING( "Gv::HttpServerPeer::buildPduFromFile: cannot read file [" << G::Str::printable(path) << "]" ) ;
			}
			else
			{
				size_t file_size = Gr::imagebuffer::size_of( *image_buffer_ptr ) ;
				m_pdu.append( fileHeader(file_size,type) ) ;
				m_pdu.assignBody( image_buffer_ptr , file_size ) ;
				if( m_config.moreVerbose() )
					G_LOG( "Gv::HttpServerPeer::buildPduFromFile: serving file: path=[" << path << "] type=[" << type << "] size=" << file_size ) ;
			}
		}
		else
		{
			G_WARNING( "Gv::HttpServerPeer::buildPduFromFile: cannot open file [" << G::Str::printable(path) << "]" ) ;
		}
	}
	else
	{
		const std::string & reason = pair.second ;
		G_LOG( "Gv::HttpServerPeer::buildPduFromFile: url-path=[" << m_url.path() << "]: resource not available: " << reason ) ;
	}

	if( m_pdu.empty() )
	{
		m_pdu = errorResponse( 404 , "Not Found" ) ;
	}
}

void Gv::HttpServerPeer::buildPduFromStatus()
{
	G::Item info = G::Item::map() ;
	G::StringArray list = G::Publisher::list() ;
	for( G::StringArray::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		info.add( *p , G::Publisher::info( *p ) ) ;
	}
	std::ostringstream ss ;
	info.out( ss ) ;
	size_t ss_size = static_cast<size_t>(std::max(std::streampos(0),ss.tellp())) ;
	m_pdu.clear() ;
	m_pdu.append( fileHeader(ss_size,"application/json") ) ;
	m_pdu.append( ss.str() ) ;
}

void Gv::HttpServerPeer::onDataTimeout()
{
	// the data timer is short-circuited by a new video image (onImageInput())
	// but it is also used to ensure a minimum frame rate, typically 1fps,
	// when streaming

	G_DEBUG( "Gv::HttpServerPeer::onDataTimeout: image timeout: state " << m_state ) ;
	if( m_state == s_file )
	{
		if( fileRequest() )
			buildPduFromFile() ;
		else
			buildPduFromStatus() ;

		bool all_sent = doSend( m_pdu ) ;
		if( all_sent )
			m_state = s_idle ;
		else
			m_state = s_file_busy ;
	}
	else if( m_state == s_waiting )
	{
		if( m_image.empty() )
		{
			m_state = s_idle ;
			G_DEBUG( "Gv::HttpServerPeer::onDataTimeout: no image available for http get request [" + m_url.str() + "]" ) ;
			doSendResponse( 503 , "Image unavailable" , "Retry-After: 1" ) ;
		}
		else if( m_config.streaming() )
		{
			m_state = s_streaming_first_idle ;
			m_data_timer.startTimer( 0 ) ;
		}
		else
		{
			m_state = s_single_idle ;
			m_data_timer.startTimer( 0 ) ;
		}
	}
	else if( m_state == s_streaming_first_idle )
	{
		m_idle_timer.cancelTimer() ;
		buildPduFirst() ;
		bool all_sent = sendPdu() ;
		if( all_sent )
		{
			m_state = s_streaming_idle ;
			m_data_timer.startTimer( m_config.imageRepeatTimeout() ) ;
		}
		else
		{
			m_state = s_streaming_first_busy ;
			m_sending = m_image_number ;
		}
	}
	else if( m_state == s_streaming_idle )
	{
		m_idle_timer.cancelTimer() ;
		buildPdu() ;
		bool all_sent = sendPdu() ;
		if( all_sent )
		{
			m_state = s_streaming_idle ;
			m_data_timer.startTimer( m_config.imageRepeatTimeout() ) ;
		}
		else
		{
			m_state = s_streaming_busy ;
			m_sending = m_image_number ;
		}
	}
	else if( m_state == s_single_idle )
	{
		buildPduSingle() ;
		bool all_sent = sendPdu() ;
		if( all_sent )
		{
			m_state = s_idle ;
		}
		else
		{
			m_state = s_single_busy ;
			m_sending = m_image_number ;
		}
	}
}

void Gv::HttpServerPeer::onSendComplete()
{
	G_DEBUG( "Gv::HttpServerPeer::onSendComplete: image " << m_sending << " sent" ) ;
	if( m_config.moreVerbose() )
		G_LOG( "Gv::HttpServerPeer::onSendComplete: flow control released for image " << m_sending ) ;
	m_pdu.release() ;
	if( m_state == s_idle )
	{
	}
	else if( m_state == s_streaming_first_busy )
	{
		m_state = s_streaming_idle ;
		startStreamingTimer() ;
		m_sending = 0U ;
	}
	else if( m_state == s_streaming_busy )
	{
		m_state = s_streaming_idle ;
		startStreamingTimer() ;
		m_sending = 0U ;
	}
	else if( m_state == s_single_busy )
	{
		m_state = s_idle ;
	}
	else if( m_state == s_file_busy )
	{
		m_state = s_idle ;
	}
}

void Gv::HttpServerPeer::startStreamingTimer()
{
	G_ASSERT( m_sending != 0U ) ;
	bool sent_image_is_latest = m_sending == m_image_number ;
	if( sent_image_is_latest ) // network output is faster than the camera input - wait for the camera
		m_data_timer.startTimer( m_config.imageRepeatTimeout() ) ;
	else
		m_data_timer.startTimer( 0 ) ;
}

Gr::Image Gv::HttpServerPeer::textToJpeg( const Gr::ImageBuffer & text_buffer )
{
	Gr::ImageBuffer * image_buffer = Gr::Image::blank( m_text_raw_image , Gr::ImageType::raw(160,120,1) ) ;
	Gr::ImageData image_data( *image_buffer , 160 , 120 , 1 ) ;
	image_data.fill( 0 , 0 , 0 ) ;

	typedef Gr::traits::imagebuffer<Gr::ImageBuffer>::const_byte_iterator iterator_t ;
	Gr::ImageDataWriter writer( image_data , 0 , 0 , Gr::Colour(255U,255U,255U) , Gr::Colour(0,0,0) , true , false ) ;
	writer.write( iterator_t(text_buffer) , iterator_t() ) ;

	m_sources.converter().toJpeg( m_text_raw_image , m_text_jpeg_image ) ;

	return m_text_jpeg_image ;
}

void Gv::HttpServerPeer::onNonImageInput( ImageInputSource & , Gr::Image image , const std::string & type_str )
{
	G_ASSERT( !image.empty() && !image.valid() ) ; // we have data but it's not an image
	G_DEBUG( "Gv::HttpServerPeer::onNonImageInput: non-image [" << type_str << "]" ) ;

	if( m_config.type() == "any" )
	{
		doInput( image , type_str ) ;
	}
	else if( type_str == "application/json" && Gr::Jpeg::available() )
	{
		image = textToJpeg( image.data() ) ;
		doInput( image , image.type().str() ) ;
	}
	else
	{
		G_DEBUG( "Gv::HttpServerPeer::onNonImageInput: not serving non-image" ) ;
	}
}

void Gv::HttpServerPeer::onImageInput( ImageInputSource & , Gr::Image image )
{
	G_DEBUG( "Gv::HttpServerPeer::onImageInput: type=[" << image.type() << "](" << image.type() << ") seqno=[" << m_image_number << "]" ) ;
	doInput( image , image.type().str() ) ;
}

void Gv::HttpServerPeer::doInput( Gr::Image image , const std::string & type_str )
{
	m_image_number++ ; 
	if( m_image_number == 0U ) 
		m_image_number = 1U ;

	if( m_state == s_init || m_state == s_idle )
	{
		// in the init and idle states we do not know the client's requested type so we
		// have not told the input class what image type and size to convert to
		m_image.clear() ;
	}
	else
	{
		m_image = image ;
		m_image_size = Gr::imagebuffer::size_of(image.data()) ;
		m_image_type_str = type_str ;
		if( m_state == s_waiting || m_state == s_streaming_idle )
			m_data_timer.startTimer( 0 ) ; // short-circuit
	}
}

Gv::ImageInputConversion Gv::HttpServerPeer::imageInputConversion( ImageInputSource & )
{
	ImageInputConversion conversion ;
	conversion.scale = m_config.scale() ;
	conversion.monochrome = m_config.monochrome() ;

	if( m_config.type() == "raw" || m_config.type() == "pnm" )
		conversion.type = ImageInputConversion::to_raw ;
	else if( m_config.type() == "any" || !Gr::Jpeg::available() )
		conversion.type = ImageInputConversion::none ;
	else
		conversion.type = ImageInputConversion::to_jpeg ;
	return conversion ;
}

void Gv::HttpServerPeer::onDelete( const std::string & reason )
{
	if( !reason.empty() && reason.find("peer disconnected") != 0U )
		G_ERROR( "Gv::HttpServerPeer::onException: exception: " << reason ) ;
	G_LOG( "Gv::HttpServerPeer::onDelete: disconnection of " << peerAddress().second.displayString() ) ;
}

void Gv::HttpServerPeer::onIdleTimeout()
{
	G_LOG( "Gv::HttpServerPeer::onIdleTimeout: timeout" ) ;
	doDelete() ;
}

void Gv::HttpServerPeer::doSendResponse( int e , const std::string & s , const std::string & header )
{
	doSend( errorResponse(e,s,header) ) ;
}

std::string Gv::HttpServerPeer::errorResponse( int e , const std::string & s , const std::string & header ) const
{
	std::string body ;
	{
		std::ostringstream ss ;
		ss << "<!DOCTYPE html>\r\n<html><body><p>" << e << " " << s << "</p></body></html>\r\n" ;
		body = ss.str() ;
	}

	std::ostringstream ss ;
	ss
		<< "HTTP/1.1 " << e << " " << s << "\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Content-Type: text/html\r\n" ;
	if( !header.empty() )
		ss << header << "\r\n" ;
	ss
		<< "Connection: keep-alive\r\n"
		<< "\r\n"
		<< body ;

	return ss.str() ;
}

std::string Gv::HttpServerPeer::fileHeader( size_t content_length , const std::string & content_type ) const
{
	std::ostringstream ss ;
	ss 
		<< "HTTP/1.1 200 OK\r\n" ;
	if( !content_type.empty() )
		ss << "Content-Type: " << content_type << "\r\n" ;
	ss
		<< "Content-Length: " << content_length << "\r\n"
		<< "Connection: keep-alive\r\n"
		<< "\r\n" ;
	return ss.str() ;
}

std::string Gv::HttpServerPeer::pnmHeader( Gr::ImageType type ) const
{
	std::ostringstream ss ;
	if( type.channels() == 1 )
		ss << "P5\n" ;
	else
		ss << "P6\n" ;
	ss << type.dx() << " " << type.dy() << "\n255\n" ;
	return ss.str() ;
}

std::string Gv::HttpServerPeer::pnmType( Gr::ImageType type ) const
{
	return "image/x-portable-anymap" ;
}

std::string Gv::HttpServerPeer::simpleHeader( size_t content_length , Gr::ImageType type , 
	const std::string & type_str , const std::string & source_name , const std::string & source_info ) const
{
	std::string content_type = type.valid() ? ( type.isRaw() ? type.str() : type.simple() ) : type_str ;

	std::string pnm_header ;
	if( type.isRaw() && m_config.type() == "pnm" )
	{
		pnm_header = pnmHeader( type ) ;
		content_type = pnmType( type ) ;
		content_length += pnm_header.size() ;
	}

	std::ostringstream ss ;
	ss << "HTTP/1.1 200 OK\r\n" ;
	if( m_config.refresh() )
		ss << "Refresh: " << m_config.refresh() << "\r\n" ;
	ss 
		<< "Content-Type: " << content_type << "\r\n"
		<< "Content-Length: " << content_length << "\r\n"
		<< "Cache-Control: no-cache\r\n"
		<< "Connection: keep-alive\r\n" ;

	std::string appid( "VT-" ) ;
	if( appid.find("__") == 0U )
		appid.clear() ;

	if( type.valid() && type.dx() != 0 )
		ss << "X-" << appid << "Width: " << type.dx() << "\r\n" ;
	if( type.valid() && type.dy() != 0 )
		ss << "X-" << appid << "Height: " << type.dy() << "\r\n" ;
	if( !source_name.empty() )
		ss << "X-" << appid << "Source: " << G::Url::encode(source_name) << "\r\n" ;
	if( !source_info.empty() )
		ss << "X-" << appid << "Source-Info: " << G::Url::encode(source_info,false) << "\r\n" ;

	ss << "\r\n" << pnm_header ;
	return ss.str() ;
}

namespace
{
	const char * boundary = "29872987349876236436234298656398659642596" ;
}

std::string Gv::HttpServerPeer::streamingHeader( size_t , Gr::ImageType type , const std::string & ) const
{
	std::ostringstream ss ;
	ss 
		<< "HTTP/1.1 200 OK\r\n"
		<< "Connection: keep-alive\r\n" ;
	if( m_config.refresh() ) 
		ss << "Refresh: " << m_config.refresh() << "\r\n" ; // dubious for streaming, but helps fault tolerance
	ss
		<< "Content-Type: multipart/x-mixed-replace;boundary=" << boundary << "\r\n" ;

	std::string appid( "VT-" ) ;
	if( appid.find("__") == 0U ) 
		appid.clear() ;

	if( type.valid() && type.dx() != 0 )
		ss << "X-" << appid << "Width: " << type.dx() << "\r\n" ;
	if( type.valid() && type.dy() != 0 )
		ss << "X-" << appid << "Height: " << type.dy() << "\r\n" ;

	ss
		<< "\r\n" ;
	return ss.str() ;
}

std::string Gv::HttpServerPeer::streamingSubHeader( size_t content_length , Gr::ImageType type , const std::string & type_str ) const
{
	std::string content_type = type.valid() ? ( type.isRaw() ? type.str() : type.simple() ) : type_str ;

	std::string pnm_header ;
	if( type.isRaw() && m_config.type() == "pnm" )
	{
		pnm_header = pnmHeader( type ) ;
		content_type = pnmType( type ) ;
		content_length += pnm_header.size() ;
	}

	std::ostringstream ss ;
	ss 
		<< "\r\n--" << boundary << "\r\n"
		<< "Content-Type: " << content_type << "\r\n"
		<< "Content-Length: " << content_length << "\r\n"
		<< "\r\n" << pnm_header ;
	return ss.str() ;
}

unsigned int Gv::HttpServerPeer::headerValue( const std::string & line )
{
	std::string value = G::Str::trimmed( G::Str::tail(line,":") , G::Str::ws() ) ;
	if( value.empty() || !G::Str::isUInt(value) )
		G_DEBUG( "Gv::HttpServerPeer::headerValue: not a number: [" << line << "]" ) ;
	return G::Str::toUInt( value , "0" ) ;
}

// ==

Gv::HttpServerPeer::Pdu::Pdu() :
	m_body_ptr_size(0U) ,
	m_locked(false)
{
	m_segments.reserve( 3U ) ;
}

Gv::HttpServerPeer::Pdu::Pdu( const std::string & s ) :
	m_head(s) ,
	m_body_ptr_size(0U) ,
	m_locked(false)
{
}

void Gv::HttpServerPeer::Pdu::clear()
{
	G_ASSERT( m_body_ptr.get() == nullptr || !m_locked ) ;
	m_body_ptr.reset() ;
	m_body_ptr_size = 0U ;
	m_head.clear() ;
}

bool Gv::HttpServerPeer::Pdu::empty() const
{
	return m_head.empty() && m_body_ptr_size == 0U ;
}

void Gv::HttpServerPeer::Pdu::append( const std::string & s )
{
	G_ASSERT( m_body_ptr.get() == nullptr ) ; // moot
	m_head.append( s ) ;
}

void Gv::HttpServerPeer::Pdu::assignBody( shared_ptr<const Gr::ImageBuffer> data_ptr , size_t n )
{
	G_ASSERT( m_body_ptr.get() == nullptr || !m_locked ) ;
	m_body_ptr = data_ptr ;
	m_body_ptr_size = n ;
}

size_t Gv::HttpServerPeer::Pdu::size() const
{
	return m_head.size() + m_body_ptr_size ;
}

std::string Gv::HttpServerPeer::Pdu::head() const
{
	return m_head ;
}

void Gv::HttpServerPeer::Pdu::operator=( const std::string & s )
{
	m_body_ptr.reset() ;
	m_body_ptr_size = 0U ;
	m_head = s ;
}

void Gv::HttpServerPeer::Pdu::lock() const
{
	G_ASSERT( !m_locked ) ;
	m_locked = true ;
}

void Gv::HttpServerPeer::Pdu::release()
{
	G_ASSERT( m_locked ) ;
	m_locked = false ;
	m_body_ptr.reset() ;
	m_body_ptr_size = 0U ;
}

const std::vector<std::pair<const char *,size_t> > & Gv::HttpServerPeer::Pdu::segments() const
{
	m_segments.clear() ;
	if( !m_head.empty() ) m_segments.push_back( Segment(m_head.data(),m_head.size()) ) ;
	if( m_body_ptr_size != 0U ) 
	{
		const Gr::ImageBuffer & image_buffer = *m_body_ptr.get() ;

		typedef Gr::traits::imagebuffer<Gr::ImageBuffer>::const_row_iterator row_iterator ;
		for( row_iterator part_p = Gr::imagebuffer::row_begin(image_buffer) ; part_p != Gr::imagebuffer::row_end(image_buffer) ; ++part_p )
		{
			const char * segment_p = Gr::imagebuffer::row_ptr(part_p) ;
			size_t segment_n = Gr::imagebuffer::row_size(part_p) ;
			m_segments.push_back( Segment(segment_p,segment_n) ) ;
		}
	}
	return m_segments ;
}

/// \file gvhttpserverpeer.cpp

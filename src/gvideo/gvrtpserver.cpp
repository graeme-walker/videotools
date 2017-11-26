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
// gvrtpserver.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gvrtpserver.h"
#include "gvmulticast.h"
#include "groot.h"
#include "gaddress.h"
#include "grjpeg.h"
#include "gstr.h"
#include "gfile.h"
#include "gtest.h"
#include "ghexdump.h"
#include "glog.h"
#include <fstream>
#include <sstream>
#include <algorithm>

Gv::RtpServer::RtpServer( RtpServerHandler & handler , int scale , bool monochrome , GNet::Address bind_address , 
	const std::string & group_address , unsigned int packet_type , const G::Path & fmtp_file , 
	int jpeg_fudge_factor , const std::string & filter_spec , unsigned int source_stale_timeout ) :
		m_handler(handler) ,
		m_scale(scale) ,
		m_monochrome(monochrome) ,
		m_packet_type(packet_type) ,
		m_source_id(0) ,
		m_source_time(0) ,
		m_source_stale_timeout(10U) ,
		m_fmtp_file(fmtp_file) ,
		m_jpeg_fudge_factor(jpeg_fudge_factor) ,
		m_filter_spec(filter_spec) ,
		m_socket(bind_address.domain()) ,
		m_packet_buffer(1500U) ,
		m_packet_stream(jpeg_fudge_factor) ,
		m_seq_old(0U) ,
		m_test_timer(*this,&RtpServer::onTimer,*this) ,
		m_test_index(0)
{
	{
		G::Root claim_root ;
		m_socket.bind( bind_address ) ;
	}

	m_socket.addReadHandler( *this ) ;

	if( !group_address.empty() )
		join( group_address ) ;

	if( G::Test::enabled("rtpserver-replay") )
		m_test_timer.startTimer( 2U ) ;

	{
		// TODO rtp packet filtering expressions
		G::StringArray list ;
		G::Str::splitIntoTokens( m_filter_spec , list , "," ) ;
		for( G::StringArray::iterator p = list.begin() ; p != list.end() ; ++p )
		{
			if( G::Str::isNumeric(*p) )
				m_filter_list.push_back( G::Str::toUInt(*p) ) ;
		}
	}
}

Gv::RtpServer::~RtpServer()
{
}

void Gv::RtpServer::join( const std::string & group )
{
	Gv::Multicast::join( m_socket.fd() , group ) ;
}

void Gv::RtpServer::onTimer() 
{ 
	// if we have seen no network packets in the initial startup time period then
	// look for replay files called "test-1.dat" etc.
	G_DEBUG( "Gv::RtpServer::onTimer: timeout: " << m_seq_old << " " << m_test_index ) ;
	if( ( m_seq_old == 0U && m_test_index == 0 ) || m_test_index > 0 )
	{
		std::string test_file = "test-" + G::Str::fromInt(++m_test_index) + ".dat" ;
		if( G::File::exists(test_file) )
		{
			G_LOG_S( "Server::onTimer: replaying test data file [" << test_file << "]" ) ;
			std::ifstream f( test_file.c_str() ) ;
			std::stringstream ss ;
			ss << f.rdbuf() ;
			std::string s = ss.str() ; 
			onData( s.data() , s.size() ) ; 
			m_test_timer.startTimer( 0U ) ;
		}
	}
}

void Gv::RtpServer::readEvent()
{
	G_ASSERT( m_packet_buffer.size() == 1500U ) ;
	GNet::Socket::ssize_type n = m_socket.read( &m_packet_buffer[0] , m_packet_buffer.size() ) ;
	if( n != 0U )
		onData( &m_packet_buffer[0] , n ) ;
}

void Gv::RtpServer::onData( const char * p , std::string::size_type n )
{
 #if 0
	if( G::Test::enabled("rtpserver-save") )
	{
		static int seq = 0 ;
		std::ostringstream ss ;
		ss << "test-" << ++seq << ".dat" ;
		std::ofstream file( ss.str().c_str() ) ;
		file << std::string(p,n) ;
	}
 #endif

	processRtpData( p , n ) ;
}

void Gv::RtpServer::processRtpData( const char * p , std::string::size_type n )
{
	// parse as RTP
	//
	if( n < Gv::RtpPacket::smallest() ) 
	{
		G_WARNING( "Server::processRtpData: ignoring invalid rtp packet: too small" ) ;
		return ;
	}
	Gv::RtpPacket rtp_packet( p , n ) ;
	if( !rtp_packet.valid() )
	{
		G_WARNING( "Server::processRtpData: ignoring invalid rtp packet: " << rtp_packet.reason() ) ;
		return ;
	}

	// only process packets of one payload type
	if( m_packet_type == 0U )
	{
		G_LOG( "Gv::RtpServer::processRtpData: processing packets of type " << rtp_packet.type() ) ;
		m_packet_type = rtp_packet.type() ;
	}
	if( m_packet_type != 0U && rtp_packet.type() != m_packet_type )
	{
		G_WARNING_ONCE( "Gv::RtpServer::processRtpData: ignoring unwanted rtp packet type: " << rtp_packet.type() << " != " << m_packet_type ) ;
		G_DEBUG( "Gv::RtpServer::processRtpData: ignoring unwanted rtp packet type: " << rtp_packet.type() << " != " << m_packet_type ) ;
		return ;
	}

	// only process packets from one source
	if( m_source_id == 0U )
	{
		G_DEBUG( "Gv::RtpServer::processRtpData: source id: " << rtp_packet.src() ) ;
		m_source_id = rtp_packet.src() ;
	}
	if( rtp_packet.src() == m_source_id )
	{
		m_source_time = ::time(nullptr) ;
	}
	else
	{
		if( (m_source_time+m_source_stale_timeout) < ::time(nullptr) )
		{
			G_LOG( "Gv::RtpServer::processRtpData: switching source: " << m_source_id << " -> " << rtp_packet.src() ) ;
			m_source_id = rtp_packet.src() ;
			m_source_time = ::time(nullptr) ;
		}
		else
		{
			G_WARNING( "Gv::RtpServer::processRtpData: ignoring unknown rtp packet source: " << rtp_packet.src() << " != " << m_source_id ) ;
			return ;
		}
	}

	// report missing packets
	if( m_seq_old != 0U && rtp_packet.seq() != (m_seq_old+1U) && rtp_packet.seq() != 0U )
	{
		G_WARNING( "Gv::RtpServer::processRtpData: missing packet(s): " << (m_seq_old+1U) << "-" << (rtp_packet.seq()-1U) ) ;
	}
	m_seq_old = rtp_packet.seq() ;

	// type-specific processing
	//
	if( rtp_packet.typeJpeg() ) // 26 = "JPEG/9000"
	{
		// parse as RTP JPEG

		if( rtp_packet.size() < Gv::RtpJpegPacket::smallest() ) 
		{
			G_WARNING( "Gv::RtpServer::processRtpData: invalid rtp-jpeg packet: too small" ) ;
			return ;
		}

		Gv::RtpJpegPacket jpeg_packet( rtp_packet.begin() , rtp_packet.end() ) ;
		if( !jpeg_packet.valid() )
		{
			G_WARNING( "Gv::RtpServer::processRtpData: invalid rtp-jpeg packet: " << jpeg_packet.reason() ) ;
			return ;
		}

		if( G::Log::at(G::Log::s_LogVerbose) )
			G_LOG( "Gv::RtpPacketStream::add: rtp-jpeg packet: " << rtp_packet.str() << " ; " << jpeg_packet.str() ) ;

		if( !filter(jpeg_packet) )
			m_packet_stream.add( rtp_packet , jpeg_packet ) ;

		while( m_packet_stream.more() )
			processJpegPayload( m_packet_stream.get() ) ;
	}
	else if( rtp_packet.typeDynamic() ) // dynamic mapping using sdp
	{
		// parse as RTP H264 

		if( rtp_packet.size() < Gv::RtpAvcPacket::smallest() ) 
		{
			G_WARNING( "Gv::RtpServer::processRtpData: invalid rtp-avc packet: too small" ) ;
			return ;
		}

		Gv::RtpAvcPacket avc_packet( rtp_packet.ubegin() , rtp_packet.uend() ) ; 
		if( !avc_packet.valid() )
		{
			G_WARNING( "Gv::RtpServer::processRtpData: invalid rtp-avc packet: " << avc_packet.reason() ) ;
			return ;
		}

		if( G::Log::at(G::Log::s_LogVerbose) )
			G_LOG( "Gv::RtpServer::processRtpAvcPacket: rtp-avc packet: " << rtp_packet.str() << " ; " << avc_packet.str(G::Log::at(G::Log::s_Debug)) ) ;

		if( !filter(avc_packet) )
			m_packet_stream.add( rtp_packet , avc_packet ) ;

		while( m_packet_stream.more() )
			processAvcPayload( m_packet_stream.get() ) ;
	}
}

bool Gv::RtpServer::filter( const Gv::RtpJpegPacket & ) const
{
	return false ; 
}

bool Gv::RtpServer::filter( const Gv::RtpAvcPacket & avc_packet ) const
{
	if( !m_filter_list.empty() )
	{
		//unsigned int packet_level = avc_packet.nri() ; // 0..3
		unsigned int nalu_type = avc_packet.type_is_fu() ? avc_packet.fu_type() : avc_packet.type() ;
		bool match = std::find( m_filter_list.begin() , m_filter_list.end() , nalu_type ) != m_filter_list.end() ;
		return !match ; // keep on match
	}
	else
	{
		return false ; // keep
	}
}

void Gv::RtpServer::processJpegPayload( const std::vector<char> & payload )
{
	// parse out the image size
	Gr::JpegInfo jpeg_info( &payload[0] , payload.size() ) ;
	Gr::ImageType image_type = Gr::ImageType::jpeg( jpeg_info.dx() , jpeg_info.dy() , jpeg_info.channels() ) ;
	G_DEBUG( "Gv::RtpServer::processJpegFrame: processing jpeg image: " << image_type ) ;

	int scale = autoscale( m_scale , jpeg_info.dx() ) ;
	if( scale > 1 || m_monochrome )
	{
		m_jpeg_reader.setup( scale , m_monochrome ) ;
		m_jpeg_reader.decode( m_jpeg_image_data , &payload[0] , payload.size() ) ;
		m_jpeg_writer.encode( m_jpeg_image_data , m_jpeg_buffer ) ;
		m_handler.onImage( m_jpeg_buffer , Gr::ImageType::jpeg(image_type,scale,m_monochrome) , true/*keyframe*/ ) ;
	}
	else
	{
		m_handler.onImage( payload , image_type , true/*keyframe*/ ) ;
	}
}

void Gv::RtpServer::processAvcPayload( const std::vector<char> & payload )
{
	// lazy decoding of the fmtp into the avcc and lazy construction of the decoder reader stream
	if( m_avcc.get() == nullptr && m_fmtp_file != G::Path() )
	{
		G_DEBUG( "Gv::RtpServer::processAvcPayload: fmtp processing" ) ;
		std::string fmtp = readFmtpFile( m_fmtp_file ) ;
		if( !fmtp.empty() )
		{
			Gr::Avc::Configuration avcc = Gr::Avc::Configuration::fromFmtp( fmtp ) ;
			if( !avcc.valid() )
				throw InvalidFmtp( avcc.reason() ) ;

			m_avcc.reset( new Gr::Avc::Configuration(avcc) ) ;

			if( m_avc_reader_stream.get() == nullptr )
				m_avc_reader_stream.reset( new Gv::AvcReaderStream( *m_avcc.get() ) ) ;
		}
	}

	// if no fmtp then create a decoder reader stream without the avcc and rely on SPS and PPS NALUs
	if( m_avc_reader_stream.get() == nullptr )
	{
		G_DEBUG( "Gv::RtpServer::processAvcFrame: initialising avc decoder without fmtp" ) ;
		m_avc_reader_stream.reset( new Gv::AvcReaderStream ) ;
	}

	// decode
	G_DEBUG( "Gv::RtpServer::processAvcFrame: avc decode: " << G::hexdump<16>(payload.begin(),payload.end()) ) ;
	Gv::AvcReader reader( *m_avc_reader_stream.get() , &payload[0] , payload.size() ) ;
	if( reader.valid() )
	{
		m_output_buffer.clear() ;
		Gr::ImageType image_type = reader.fill( m_output_buffer , autoscale(m_scale,reader.dx()) , m_monochrome ) ;
		m_handler.onImage( m_output_buffer , image_type , reader.keyframe() ) ;
	}
	else
	{
		G_DEBUG( "Gv::RtpServer::processAvcFrame: avc decode failed" ) ;
	}
}

int Gv::RtpServer::autoscale( int s , int dx )
{
	if( s == -1 ) // "auto"
	{
		s = 1 ;
		dx = std::max( 0 , dx ) ;
		while( (dx/s) > 800 ) 
			s++ ;
	}
	return s ;
}

void Gv::RtpServer::onException( std::exception & e )
{
	G_DEBUG( "Gv::RtpServer::onException: " << e.what() ) ;
	throw ;
}

std::string Gv::RtpServer::readFmtpFile( const G::Path & fmtp_file )
{
	std::ifstream f ;
	{
		G::Root claim_root ;
		f.open( fmtp_file.str().c_str() ) ;
	}
	std::string line ;
	do
	{
		line.clear() ;
		std::getline( f , line ) ;
	} while( line.find("#") == 0U ) ;
	return line ;
}

// ==

Gv::RtpServerHandler::~RtpServerHandler()
{
}

/// \file gvrtpserver.cpp

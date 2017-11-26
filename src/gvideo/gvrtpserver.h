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
///
/// \file gvrtpserver.h
///

#ifndef GV_RTPSERVER__H
#define GV_RTPSERVER__H

#include "gdef.h"
#include "gnet.h"
#include "geventhandler.h"
#include "gtimer.h"
#include "gsocket.h"
#include "gaddress.h"
#include "gravc.h"
#include "gvrtppacket.h"
#include "gvrtpjpegpacket.h"
#include "gvrtpavcpacket.h"
#include "gvrtppacketstream.h"
#include "gvavcreader.h"
#include "grimagetype.h"
#include "grjpeg.h"
#include "gpath.h"
#include <vector>
#include <string>
#include <exception>

namespace Gv
{
	class RtpServer ;
	class RtpServerHandler ;
}

/// \class Gv::RtpServer
/// An RTP server class.
/// 
class Gv::RtpServer : public GNet::EventHandler
{
public:
	G_EXCEPTION( InvalidFmtp , "invalid fmtp" ) ;

	RtpServer( RtpServerHandler & , int scale , bool monochrome , GNet::Address bind_address , const std::string & group_address , 
		unsigned int packet_type , const G::Path & fmtp_file ,
		int jpeg_fudge_factor , const std::string & filter_spec , unsigned int source_stale_timeout ) ;
			///< Constructor. The received images are delivered to the given
			///< callback interface.
			///< 
			///< If a non-zero packet type is specified it should normally be
			///< 26 for "JPEG/90000" or some value in the dynamic range (96..127) 
			///< for "H264/90000". In principle the sdp attribute "rtpmap"
			///< should be used to map from "H264/90000" to the dynamic packet 
			///< type, but a fixed value of 96 is often adequate. If a zero 
			///< packet type is specified then the type of the first packet 
			///< received is used.

	virtual ~RtpServer() ;
		///< Destructor.

private:
	virtual void readEvent() override ;
	virtual void onException( std::exception & ) override ;
	void onData( const char * p , std::string::size_type n ) ;
	void processRtpData( const char * p , std::string::size_type n ) ;
	void processJpegPayload( const std::vector<char> & ) ;
	void processAvcPayload( const std::vector<char> & ) ;
	bool filter( const RtpAvcPacket & ) const ;
	bool filter( const RtpJpegPacket & ) const ;
	void onTimer() ;
	void join( const std::string & ) ;
	static std::string readFmtpFile( const G::Path & ) ;
	static int autoscale( int scale , int dx ) ;

private:
	RtpServerHandler & m_handler ;
	int m_scale ; // -1 for auto-scaling
	bool m_monochrome ;
	unsigned int m_packet_type ;
	unsigned long m_source_id ;
	time_t m_source_time ;
	unsigned int m_source_stale_timeout ;
	G::Path m_fmtp_file ;
	int m_jpeg_fudge_factor ;
	std::string m_filter_spec ;
	std::vector<unsigned int> m_filter_list ;
	unique_ptr<Gr::Avc::Configuration> m_avcc ;
	GNet::DatagramSocket m_socket ;
	std::vector<char> m_packet_buffer ;
	RtpPacketStream m_packet_stream ;
	std::vector<char> m_output_buffer ;
	unsigned int m_seq_old ;
	unique_ptr<Gv::AvcReaderStream> m_avc_reader_stream ;
	GNet::Timer<RtpServer> m_test_timer ;
	int m_test_index ;
	Gr::JpegReader m_jpeg_reader ;
	Gr::JpegWriter m_jpeg_writer ;
	Gr::ImageData m_jpeg_image_data ;
	std::vector<char> m_jpeg_buffer ;
} ;

/// \class Gv::RtpServerHandler
/// A interface that Gv::RtpServer uses to deliver image data.
/// 
class Gv::RtpServerHandler
{
public:
	virtual void onImage( const std::vector<char> & , const Gr::ImageType & , bool avc_key_frame ) = 0 ;
		///< Called on receipt of an image. This is either an un-compressed 
		///< JPEG image in JFIF format, or a fully-decoded AVC image.
		///< The recipient is free to modify the data buffer.

protected:
	virtual ~RtpServerHandler() ;
} ;

#endif

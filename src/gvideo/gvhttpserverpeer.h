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
/// \file gvhttpserverpeer.h
///

#ifndef GV_HTTPSERVERPEER__H
#define GV_HTTPSERVERPEER__H

#include "gdef.h"
#include "gvhttpserver.h"
#include "gvimageinput.h"
#include "grimagedata.h"
#include "gbufferedserverpeer.h"
#include "gurl.h"
#include <string>
#include <memory>
#include <vector>
#include <utility>

namespace Gv
{
	class HttpServerPeer ;
}

/// \class Gv::HttpServerPeer
/// A GNet::ServerPeer class for HTTP servers that serves up image streams. The 
/// URL sent in the peer's GET request is used to control the image format etc.
/// 
class Gv::HttpServerPeer : public GNet::BufferedServerPeer , public Gv::ImageInputHandler
{
public:
	typedef HttpServerConfig Config ;
	typedef HttpServerSource Source ;
	typedef HttpServerSources Sources ;
	typedef HttpServerResources Resources ;

	HttpServerPeer( GNet::Server::PeerInfo , Sources & , const Resources & , const Config & ) ;
		///< Constructor.

	virtual ~HttpServerPeer() ;
		///< Destructor.

private:
	virtual void onDelete( const std::string & ) override ;
	virtual void onSendComplete() override ;
	virtual bool onReceive( const std::string & ) override ;
	virtual void onSecure( const std::string & ) override ;
	virtual void onImageInput( ImageInputSource & , Gr::Image ) override ; // Gv::ImageInputHandler
	virtual void onNonImageInput( ImageInputSource & , Gr::Image , const std::string & ) override ; // Gv::ImageInputHandler
	virtual ImageInputConversion imageInputConversion( ImageInputSource & ) override ; // Gv::ImageInputHandler

private:
	struct Pdu /// Holds data as a head() string followed by a body shared-ptr, accessible as segments.
	{
		typedef std::pair<const char*,size_t> Segment ;
		typedef std::vector<Segment> Segments ;
		Pdu() ;
		explicit Pdu( const std::string & ) ;
		void clear() ;
		bool empty() const ;
		size_t size() const ;
		void append( const std::string & ) ; // appends to head()
		void assignBody( shared_ptr<const Gr::ImageBuffer> , size_t ) ;
		void operator=( const std::string & ) ;
		const Segments & segments() const ;
		std::string head() const ;
		void lock() const ;
		void release() ;
		//
		std::string m_head ;
		shared_ptr<const Gr::ImageBuffer> m_body_ptr ;
		size_t m_body_ptr_size ;
		mutable Segments m_segments ;
		mutable bool m_locked ;
	} ;

private:
	HttpServerPeer( const HttpServerPeer & ) ;
	void operator=( const HttpServerPeer & ) ;
	void selectSource( const std::string & path ) ;
	void onIdleTimeout() ;
	void onDataTimeout() ;
	void buildPduFirst() ;
	void buildPdu() ;
	void buildPduSingle() ;
	void buildPduFromFile() ;
	void buildPduFromStatus() ;
	bool sendPdu() ;
	std::string errorResponse( int e , const std::string & s , const std::string & header = std::string() ) const ;
	void doSendResponse( int e , const std::string & s , const std::string & header = std::string() ) ;
	bool doSend( const std::string & ) ;
	bool doSend( const Pdu & ) ;
	void doSendLogging( const Pdu & ) const ;
	void doInput( Gr::Image , const std::string & ) ;
	void startStreamingTimer() ;
	static std::string toPdu( std::string & , shared_ptr<char> , const std::string & , G::Url ) ;
	std::string pnmHeader( Gr::ImageType type ) const ;
	std::string pnmType( Gr::ImageType type ) const ;
	std::string fileHeader( size_t content_length , const std::string & type ) const ;
	std::string simpleHeader( size_t content_length , Gr::ImageType , const std::string & , const std::string & , const std::string & ) const ;
	std::string streamingHeader( size_t content_length , Gr::ImageType , const std::string & ) const ;
	std::string streamingSubHeader( size_t , Gr::ImageType , const std::string & ) const ;
	void doPost() ;
	void doGatewayMessage( const std::string & ) ;
	std::string doGatewayMessageImp( const std::string & ) ;
	bool fileSend() ;
	bool fileRequest() ;
	bool fileEnd() ;
	bool specialRequest() ;
	static unsigned int headerValue( const std::string & ) ;
	Gr::Image textToJpeg( const Gr::ImageBuffer & ) ;

private:
	Sources & m_sources ;
	Source m_source ;
	const Resources & m_resources ;
	Config m_config_default ;
	Config m_config ;
	G::Url m_url ;
	Pdu m_pdu ;
	GNet::Timer<HttpServerPeer> m_idle_timer ;
	GNet::Timer<HttpServerPeer> m_data_timer ;
	Gr::Image m_image ;
	size_t m_image_size ;
	Gr::ImageType m_image_type ;
	std::string m_image_type_str ;
	unsigned int m_sending ;
	unsigned int m_image_number ;
	enum { 
		s_init , // waiting for first get request
		s_idle , // sent single image or an error response, waiting for next get request
		s_got_get , // got 'get' line, waiting for end-of-headers
		s_got_get_headers , // got 'get' headers, waiting for request body
		s_got_post , // got 'post' line, waiting for end-of-headers
		s_got_post_headers , // got 'post' headers, waiting for request body
		s_waiting , // got request but no image, waiting for first image
		s_single_idle , // ready to send single-image pdu, waiting for timeout
		s_single_busy , // sending single-image pdu, waiting for send-complete
		s_streaming_first_idle , // ready to sending first pdu, waiting for timeout
		s_streaming_first_busy , // sending first pdu, waiting for send-complete
		s_streaming_idle , // waiting for next image
		s_streaming_busy , // sending pdu, waiting for send-complete
		s_file , // sending file
		s_file_busy // sending file, waiting for send-complete
	} ;
	int m_state ;
	unsigned int m_content_length ;
	Gr::Image m_text_raw_image ;
	Gr::Image m_text_jpeg_image ;
} ;

#endif

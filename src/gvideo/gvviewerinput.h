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
/// \file gvviewerinput.h
///

#ifndef GV_VIEWERINPUT__H
#define GV_VIEWERINPUT__H

#include "gdef.h"
#include "gnet.h"
#include "gdatetime.h"
#include "geventhandler.h"
#include "gtimer.h"
#include "gpublisher.h"
#include "gfatpipe.h"
#include "grimagetype.h"
#include "grimagedata.h"
#include "grimagedecoder.h"
#include <string>
#include <vector>
#include <iostream>

namespace Gv
{
	class ViewerInput ;
	class ViewerInputHandler ;
	struct ViewerInputConfig ;
}

/// \class Gv::ViewerInputHandler
/// A callback interface for Gv::ViewerInput.
/// 
class Gv::ViewerInputHandler
{
public:
	virtual void onInput( int dx , int dy , int channels , const char * p , size_t n ) = 0 ;
		///< Called to deliver a raw image.

	virtual ~ViewerInputHandler() ;
		///< Destructor.
} ;

/// \class Gv::ViewerInputConfig
/// A structure to hold configuration for a Gv::ViewerInput object.
/// 
struct Gv::ViewerInputConfig
{
	int m_scale ;
	bool m_static ; // one frame
	G::EpochTime m_rate_limit ; // inter-frame delay
	explicit ViewerInputConfig( int scale = 1 , bool static_ = false ) :
		m_scale(scale) ,
		m_static(static_) ,
		m_rate_limit(0)
	{
	}
} ;

/// \class Gv::ViewerInput
/// An input channel for images. Images are decoded as necessary.
/// 
class Gv::ViewerInput : private GNet::EventHandler
{
public:
	G_EXCEPTION( Closed , "viewer input channel closed" ) ;
	typedef ViewerInputHandler Handler ;

	ViewerInput( ViewerInputHandler & , const ViewerInputConfig & , 
		const std::string & channel , int shmem_fd , int pipe_fd ) ;
			///< Constructor.

	~ViewerInput() ;
		///< Destructor.

	int fd() const ;
		///< Returns the file descriptor that is being used for notification.

	const char * data() const ;
		///< Returns the data as a standard luma or rgb-interleaved buffer.

	size_t size() const ;
		///< Returns the data() size.

	int dx() const ;
		///< Returns the image width.

	int dy() const ;
		///< Returns the image height.

	int channels() const ;
		///< Returns the number of channels (one or three).

	Gr::ImageType type() const ;
		///< Returns the type for the decoded image.

private:
	ViewerInput( const ViewerInput & ) ;
	void operator=( const ViewerInput & ) ;
	virtual void readEvent() override ;
	virtual void onException( std::exception & ) override ;
	static Gr::ImageType subsample( std::vector<char> & , const Gr::ImageType & , int , bool , bool ) ;
	void onTimeout() ;

private:
	bool read( std::vector<char> & ) ;
	bool decode() ;

public:
	ViewerInputHandler & m_input_handler ;
	int m_scale ;
	bool m_static ;
	bool m_first ;
	G::EpochTime m_rate_limit ;
	int m_fd ;
	unique_ptr<G::FatPipe::Receiver> m_pipe ;
	unique_ptr<G::PublisherSubscription> m_channel ;
	std::vector<char> m_buffer ;
	std::vector<char> m_tmp ;
	Gr::ImageType m_type_in ;
	std::string m_type_in_str ;
	Gr::ImageType m_type_out ; // raw
	GNet::Timer<ViewerInput> m_timer ;
	Gr::ImageData m_decoder_tmp ;
	Gr::ImageDecoder m_decoder ;
	char * m_data_out ;
} ;

#endif

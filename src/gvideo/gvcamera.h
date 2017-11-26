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
/// \file gvcamera.h
///

#ifndef GV_CAMERA__H
#define GV_CAMERA__H

#include "gdef.h"
#include "gnet.h"
#include "gtimer.h"
#include "geventloop.h"
#include "gvimageinput.h"
#include "gvcapture.h"
#include "gvoverlay.h"
#include "gvcapturebuffer.h"
#include "geventhandler.h"
#include "gvtimezone.h"
#include "gtimer.h"
#include "glog.h"
#include "gassert.h"
#include <string>
#include <memory>

namespace Gv
{
	class Camera ;
}

/// \class Gv::Camera
/// A high-level webcam interface that hooks into the GNet event loop and acts 
/// as an Gv::ImageInputSource. Supports lazy device opening, transparent
/// re-opening, and slow reads.
/// 
class Gv::Camera : private GNet::EventHandler, public Gv::ImageInputSource
{
public:
	Camera( Gr::ImageConverter & , const std::string & dev_name , const std::string & dev_config , 
		bool one_shot , bool lazy_open , unsigned int lazy_open_timeout ,
		const std::string & caption , const Gv::Timezone & caption_tz = Gv::Timezone() , 
		const std::string & image_input_source_name = std::string() ) ;
			///< Constructor. Adds itself to the GNet event loop.
			///< 
			///< The device configuration string supports a "sleep" parameter for slow 
			///< reads; while sleeping any read events are ignored. The device 
			///< configuration is also passed to the Gv::Capture sub-object.
			///< 
			///< Images are received by objects implementing Gv::ImageInputHandler,
			///< and they need to register themselves by calling the base class 
			///< Gv::ImageInputSource::addImageInputHandler() method.

	virtual ~Camera() ;
		///< Destructor.

	bool isOpen() const ;
		///< Returns true if the webcam device is open.

private:
	Camera( const Camera & ) ;
	void operator=( const Camera & ) ;
	int fd() const ;
	Capture * create( const std::string & , const std::string & , bool ) ;
	void onOpenTimeout() ;
	void onReadTimeout() ;
	void onSendTimeout() ;
	void onOpen() ;
	void doRead() ;
	bool doReadImp() ;
	void blank() ;

private:
	virtual void readEvent() override ; // GNet::EventHandler
	virtual void onException( std::exception & ) override ; // GNet::EventHandler
	virtual void resend( ImageInputHandler & ) override ; // Gv::ImageInputSource

private:
	std::string m_dev_name ;
	std::string m_dev_config ;
	bool m_one_shot ;
	bool m_lazy_open ;
	unsigned int m_open_timeout ;
	std::string m_caption ;
	Gv::Timezone m_tz ;
	Gr::Image m_image ;
	Gr::ImageType m_image_type ;
	unique_ptr<Gv::Capture> m_capture ;
	int m_dx ;
	int m_dy ;
	GNet::Timer<Camera> m_open_timer ;
	GNet::Timer<Camera> m_read_timer ;
	GNet::Timer<Camera> m_send_timer ;
	unsigned int m_sleep_ms ;
} ;

#endif

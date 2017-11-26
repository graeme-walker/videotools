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
/// \file gvimageinput.h
///

#ifndef GV_IMAGEINPUT__H
#define GV_IMAGEINPUT__H

#include "gdef.h"
#include "grimagedata.h"
#include "grimagetype.h"
#include "grimageconverter.h"
#include "gpublisher.h"
#include "gtimer.h"
#include <string>
#include <vector>
#include <memory>

namespace Gv
{
	class ImageInput ;
	class ImageInputSource ;
	class ImageInputHandler ;
	class ImageInputConversion ;
	class ImageInputTask ;
}

/// \class Gv::ImageInputConversion
/// A structure that describes the preferred image type for the 
/// Gv::ImageInputHandler callback.
/// 
class Gv::ImageInputConversion
{
public:
	enum Type { none , to_jpeg , to_raw } ;

	ImageInputConversion() ;
		///< Default constructor for a 'none' conversion, with scale one.

	ImageInputConversion( Type type , int scale , bool monochrome ) ;
		///< Constructor.

	Type type ;
	int scale ;
	bool monochrome ;
} ;

/// \class Gv::ImageInputTask
/// A private implementation class used by Gv::ImageInputSource. Represents 
/// an image and its conversion.
/// 
class Gv::ImageInputTask
{
public:
	ImageInputTask() ;
		///< Default constructor.

	bool run( Gr::ImageConverter & , Gr::Image ) ;
		///< Runs the conversion. Returns false on error.

	ImageInputHandler * m_handler ;
	ImageInputConversion m_conversion ;
	Gr::Image m_image ;
} ;

/// \class Gv::ImageInputHandler
/// A callback interface for Gv::ImageInputSource.
/// 
class Gv::ImageInputHandler
{
public:
	virtual void onImageInput( ImageInputSource & , Gr::Image ) = 0 ;
		///< Called by Gv::ImageInputSource to deliver a new image. 
		///< 
		///< It is allowed to call removeImageInputHandler() from within
		///< the callback. 

	virtual void onNonImageInput( ImageInputSource & , Gr::Image , const std::string & type_str ) ;
		///< Called by Gv::ImageInputSource to deliver a new non-image. 
		///< This default implementation does nothing.

	virtual ImageInputConversion imageInputConversion( ImageInputSource & ) = 0 ;
		///< Returns the required image type conversion. This is called
		///< just before each image is delivered.

	virtual ~ImageInputHandler() ;
		///< Destructor.

private:
	void operator=( const ImageInputHandler & ) ;
} ;

/// \class Gv::ImageInputSource
/// A base class for distributing incoming images to multiple client objects,
/// supporting some simple image type conversions. Derived classes emit images
/// by calling sendImageInput(). Client objects register an interest by calling 
/// addImageInputHandler(), and they implement imageInputConversion() to indicate 
/// the image format they require. 
/// 
class Gv::ImageInputSource
{
public:
	explicit ImageInputSource( Gr::ImageConverter & , const std::string & source_name = std::string() ) ;
		///< Constructor.

	virtual ~ImageInputSource() ;
		///< Destructor.

	size_t handlers() const ;
		///< Returns the number of registered handlers.

	void addImageInputHandler( ImageInputHandler & ) ;
		///< Adds a handler for image callbacks.

	void removeImageInputHandler( ImageInputHandler & ) ;
		///< Removes a handler for image callbacks.

	std::string name() const ;
		///< Returns the source name, as passed to the constructor.

	virtual void resend( ImageInputHandler & ) = 0 ;
		///< Asks the source to resend asynchronously the lastest available image,
		///< if any, to the specified handler, even if it is old or seen before. 
		///< Some overrides may always do nothing, and others may do nothing
		///< only if no data is available.

protected:
	bool sendImageInput( Gr::Image , ImageInputHandler * one_handler_p = nullptr ) ;
		///< Sends a new image to all registered handlers, or optionally to just 
		///< one of them. Returns false if there were any image conversion errors.

	void sendNonImageInput( Gr::Image non_image , const std::string & type_str ,
		ImageInputHandler * one_handler_p = nullptr ) ;
			///< Sends non-image data to registered handlers (or one).

private:
	ImageInputSource( const ImageInputSource & ) ;
	void operator=( const ImageInputSource & ) ;
	void collectGarbage() ;

private:
	typedef std::vector<ImageInputTask> TaskList ;
	std::string m_name ;
	TaskList m_tasks ;
	Gr::ImageConverter & m_converter ;
} ;

/// \class Gv::ImageInput
/// A class for reading images from a publication channel and distributing 
/// them to multiple client objects.
/// 
class Gv::ImageInput : public Gv::ImageInputSource , private GNet::EventExceptionHandler
{
public:
	ImageInput( Gr::ImageConverter & , const std::string & channel_name , bool lazy_open ) ;
		///< Constructor.

	~ImageInput() ;
		///< Destructor.

	int fd() const ;
		///< Returns the publication channel file descriptor, or -1 
		///< if close()d.

	bool handleReadEvent() ;
		///< Called to process a read event on the file descriptor.
		///< All registered handlers are notified of any new images.
		///< Returns false if the publisher has disappeared, in
		///< which case the user should remove the fd() from the
		///< event loop and then call close().

	std::string info() const ;
		///< Returns the channel info.

	void close() ;
		///< Closes the input channel, releasing resources.

	bool open() ;
		///< Tries to re-open the input channel some time after
		///< handleReadEvent() has returned false. Returns true 
		///< on success.

private:
	ImageInput( const ImageInput & ) ;
	void operator=( const ImageInput & ) ;
	void onResendTimeout() ;
	bool receive( bool ) ;
	virtual void onException( std::exception & ) override ;
	virtual void resend( ImageInputHandler & ) override ;

private:
	G::PublisherSubscription m_channel ;
	Gr::Image m_image ;
	std::string m_type_str ;
	GNet::Timer<ImageInput> m_resend_timer ;
	ImageInputHandler * m_resend_to ;
} ;

#endif

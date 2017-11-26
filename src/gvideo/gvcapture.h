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
/// \file gvcapture.h
///

#ifndef GV_CAPTURE__H
#define GV_CAPTURE__H

#include "gdef.h"
#include "gvcapturebuffer.h"
#include "gexception.h"
#include <string>

namespace Gv
{
	class CaptureCallback ;
	class Capture ;
}

/// \class Gv::Capture
/// A video capture abstract interface, exposing a file descriptor
/// and a method to handle read events on it. The read method
/// delivers images to a callback interface in a "capture buffer".
/// 
class Gv::Capture
{
public:
	G_EXCEPTION( NoDevice , "no such device" ) ;
	G_EXCEPTION( Error , "video capture error" ) ;

	virtual ~Capture() = 0 ;
		///< Destructor.

	virtual void start() = 0 ;
		///< Restarts capturing after a stop(). Not normally needed.

	virtual void stop() = 0 ;
		///< Stops capturing.

	virtual bool simple() const = 0 ;
		///< Returns true if a complete RGB24 image can be read out
		///< of the capture device and straight into a contiguous
		///< image buffer using the appropriate read() overload.
		///< This is typically only true when using libv4l with the
		///< 'read' i/o method.

	virtual bool read( unsigned char * , size_t n ) = 0 ;
		///< Does a read of an RGB24 image into the user-supplied buffer, 
		///< on condition that the device is simple().

	virtual bool read( CaptureCallback & ) = 0 ;
		///< Delivers an image to the given callback fn. Returns true if an
		///< image was delivered to the callback interface, or false if 
		///< none was available.

	virtual int fd() const = 0 ;
		///< Returns the fd for select(). 
		///< 
		///< Note that when using streaming i/o there is no way to stop 
		///< events being delivered to this file descriptor. Using stop() 
		///< is not sufficient; also remove the file descriptor from the 
		///< event loop.

	virtual unsigned int dx() const = 0 ;
		///< Returns the width.

	virtual unsigned int dy() const = 0 ;
		///< Returns the height.

	virtual bool active() const = 0 ;
		///< Returns true after construction or start(), and false 
		///< after stop().

	virtual std::string info() const = 0 ;
		///< Returns information for debugging and logging purposes.

private:
	void operator=( const Capture & ) ;
} ;

/// \class Gv::CaptureCallback
/// A callback interface for the Gv::Capture class.
/// 
class Gv::CaptureCallback
{
public:
	virtual void operator()( const Gv::CaptureBuffer & ) = 0 ;
		///< Called to pass back an image buffer.

protected:
	virtual ~CaptureCallback() {}
} ;

#endif

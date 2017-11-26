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
/// \file gvcapture_test.h
///

#ifndef GV_CAPTURE_TEST__H
#define GV_CAPTURE_TEST__H

#include "gdef.h"
#include "gnet.h"
#include "gtimer.h"
#include "gvcapture.h"
#include "gvcapturebuffer.h"
#include "gvimagegenerator.h"

namespace Gv
{
	class CaptureTest ;
}

/// \class Gv::CaptureTest
/// A Gv::Capture implementation that serves up dummy frames created 
/// by a Gv::ImageGenerator.
/// 
class Gv::CaptureTest : public Gv::Capture , private GNet::EventExceptionHandler
{
public:
	CaptureTest( const std::string & config , ImageGenerator * ) ;
		///< Constructor. Ownership of the new()ed ImageGenerator
		///< is transferred.

	virtual ~CaptureTest() ;
		///< Destructor.

	virtual void start() override ;
		///< Override from Capture.

	virtual void stop() override ;
		///< Override from Capture.

	virtual bool simple() const override ;
		///< Override from Capture.

	virtual bool read( unsigned char * , size_t ) override ;
		///< Override from Capture.

	virtual bool read( CaptureCallback & ) override ;
		///< Override from Capture.

	virtual int fd() const override ;
		///< Override from Capture.

	virtual unsigned int dx() const override ;
		///< Override from Capture.

	virtual unsigned int dy() const override ;
		///< Override from Capture.

	virtual bool active() const override ;
		///< Override from Capture.

	virtual std::string info() const override ;
		///< Override from Capture.

	static int dx( int ) ;
		///< Static overload.

	static int dy( int ) ;
		///< Static overload.

private:
	bool createSockets() ;
	void raiseEvent() ;
	void flushEvents() ;
	void readSocket() ;
	void onTimeout() ;
	virtual void onException( std::exception & ) override ;

private:
	int m_dx ;
	int m_dy ;
	ImageGenerator * m_generator ;
	bool m_capturing ;
	int m_fd_read ;
	int m_fd_write ;
	CaptureBufferFormat m_format ;
	CaptureBufferScale m_scale ;
	unique_ptr<CaptureBuffer> m_buffer ;
	std::string m_config ;
	GNet::Timer<CaptureTest> m_timer ;
	unsigned int m_sleep_ms ;
} ;

#endif

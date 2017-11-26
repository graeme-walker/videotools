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
/// \file gvcapture_v4l.h
///

#ifndef GV_CAPTURE_V4L__H
#define GV_CAPTURE_V4L__H

#include "gdef.h"
#include "gvcapture.h"
#include "gvcapture_v4l.h"
#include "gvcapturebuffer.h"
#include <string>
#include <vector>

namespace Gv
{
	class CaptureV4l ;
	class CaptureRequeuer ;
}

/// \class Gv::CaptureV4l
/// A video capture implementation for video-for-linux (v4l).
/// 
class Gv::CaptureV4l : public Gv::Capture
{
public:
	enum io_method
	{
		IO_METHOD_READ,
		IO_METHOD_MMAP,
		IO_METHOD_USERPTR,
		///< TODO DMA method
	} ;

private:
	typedef std::vector<shared_ptr<Gv::CaptureBuffer> > CaptureBuffers ;
	typedef unsigned long size_type ; // __u32

public:
	CaptureV4l( const std::string & dev_name , const std::string & dev_config ) ;
		///< Constructor. Opens the video device, with optional device-specific
		///< configuration that can include "buffers=<n>", "nolibv4l", "mmap", 
		///< "read", "userptr", and "fmt=<n>". The number of buffers should
		///< be small to reduce latency, or larger for better throughput.

	~CaptureV4l() ;
		///< Destructor.

	virtual void start() override ;
		///< Override from Gv::Capture.

	virtual void stop() override ;
		///< Override from Gv::Capture.

	virtual bool simple() const override ;
		///< Override from Gv::Capture.

	virtual bool read( unsigned char * , size_t ) override ;
		///< Override from Gv::Capture.

	virtual bool read( CaptureCallback & ) override ;
		///< Override from Gv::Capture.

	virtual int fd() const override ;
		///< Override from Gv::Capture.

	virtual unsigned int dx() const override ;
		///< Override from Gv::Capture.

	virtual unsigned int dy() const override ;
		///< Override from Gv::Capture.

	virtual bool active() const override ;
		///< Override from Gv::Capture.

	virtual std::string info() const override ;
		///< Override from Gv::Capture.

private:
	friend class Gv::CaptureRequeuer ;
	CaptureV4l( const CaptureV4l & ) ;
	void operator=( const CaptureV4l & ) ;
	//
	static int open_device( const std::string & dev_name ) ;
	static io_method check_device( int fd , const std::string & dev_config ) ;
	CaptureBufferScale init_device( const std::string & dev_config ) ;
	void set_simple( const std::string & dev_config ) ;
	//
	void create_buffers( size_type sizeimage , const std::string & dev_config ) ;
	static void create_buffers_read( CaptureBuffers & buffers , size_type buffer_size , unsigned int nbuffers ) ;
	static void create_buffers_mmap( CaptureBuffers & buffers , int fd , size_t , unsigned int nbuffers ) ;
	static void create_buffers_userptr( CaptureBuffers & buffers , int fd , size_type buffer_size , unsigned int nbuffers ) ;
	//
	void add_buffers() ;
	static void add_buffers_read() ;
	static void add_buffers_mmap( CaptureBuffers & buffers , int fd ) ;
	static void add_buffers_userptr( CaptureBuffers & buffers , int fd ) ;
	//
	static void start_capturing_read() ;
	static void start_capturing_mmap( int fd ) ;
	static void start_capturing_userptr( int fd ) ;
	//
	static void stop_capturing_read() ;
	static void stop_capturing_mmap( int fd ) ;
	static void stop_capturing_userptr( int fd ) ;
	//
	bool read_frame_read( CaptureCallback & ) ;
	bool read_frame_read_simple( unsigned char * p , size_t n ) ;
	bool read_frame_mmap( CaptureCallback & ) ;
	bool read_frame_userptr( CaptureCallback & ) ;

private:
	int m_fd ;
	io_method m_io ;
	std::vector<shared_ptr<Gv::CaptureBuffer> > m_buffers ;
	unsigned int m_dx ;
	unsigned int m_dy ;
	bool m_active ;
	CaptureBufferFormat m_format ;
	CaptureBufferScale m_buffer_scale ;
	bool m_simple ;
} ;

#endif

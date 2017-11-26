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
// gvcapture_v4l.cpp
//

#include "gdef.h"
#include "gvcapturebuffer.h"
#include "gvcapture.h"
#include "gvcapture_v4l.h"
#include "gprocess.h"
#include "gcleanup.h"
#include "groot.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>
#include <limits>
#include <utility>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h> // ioctl()
#include <sys/stat.h>
#include <sys/mman.h> // PROT_READ
#include <linux/videodev2.h>
#include <unistd.h> // close()

#if GCONFIG_HAVE_LIBV4L
#include <libv4l2.h>
namespace { bool have_libv4l() { return true ; } }
#else
namespace
{
	bool have_libv4l() { return false ; }
	int v4l2_open( const char * p , int n , mode_t m ) { return ::open( p , n , m ) ; }
	void v4l2_close( int fd ) { ::close( fd ) ; }
	void * v4l2_mmap( void * p , size_t n , int prot , int flags , int fd , off_t o ) { return ::mmap( p , n , prot , flags , fd , o ) ; }
	int v4l2_munmap( void * p , size_t n ) { return ::munmap( p , n ) ; }
	int v4l2_read( int fd , void * p , size_t n ) { return ::read( fd , p , n ) ; }
	int v4l2_ioctl( int fd , int request , void * arg ) { return ::ioctl( fd , request , arg ) ; }
}
#endif

namespace
{
	struct ioctl_name { unsigned int n ; const char * p ; } ioctl_names[] = {
		{ VIDIOC_CROPCAP , "VIDIOC_CROPCAP" } ,
		{ VIDIOC_DQBUF , "VIDIOC_DQBUF" } ,
		{ VIDIOC_G_SELECTION , "VIDIOC_G_SELECTION" } ,
		{ VIDIOC_QBUF , "VIDIOC_QBUF" } ,
		{ VIDIOC_QUERYBUF , "VIDIOC_QUERYBUF" } ,
		{ VIDIOC_QUERYCAP , "VIDIOC_QUERYCAP" } ,
		{ VIDIOC_REQBUFS , "VIDIOC_REQBUFS" } ,
		{ VIDIOC_S_CROP , "VIDIOC_S_CROP" } ,
		{ VIDIOC_S_FMT , "VIDIOC_S_FMT" } ,
		{ VIDIOC_TRY_FMT , "VIDIOC_TRY_FMT" } ,
		{ VIDIOC_S_INPUT , "VIDIOC_S_INPUT" } ,
		{ VIDIOC_S_SELECTION , "VIDIOC_S_SELECTION" } ,
		{ VIDIOC_STREAMOFF , "VIDIOC_STREAMOFF" } ,
		{ VIDIOC_STREAMON , "VIDIOC_STREAMON" } ,
		{ VIDIOC_ENUM_FMT , "VIDIOC_ENUM_FMT" } ,
		{ 0 , nullptr }
	} ;

	std::string fourcc( g_uint32_t pf )
	{
		std::string result ;
		result.append( 1U , char((pf>>0)&0xff) ) ;
		result.append( 1U , char((pf>>8)&0xff) ) ;
		result.append( 1U , char((pf>>16)&0xff) ) ;
		result.append( 1U , char((pf>>24)&0xff) ) ;
		return G::Str::trimmed(G::Str::printable(result),G::Str::ws()) ;
	}
}

namespace Gv
{
	class CaptureImp ;
}

/// \class Gv::CaptureImp
/// A private implementation class used by Gv::CaptureV4l.
///
class Gv::CaptureImp
{
public:
	typedef std::pair<int,int> pair_t ;
	static bool set_format( v4l2_format * , int fd , unsigned int dx , unsigned int dy , g_uint32_t pf , 
		g_uint32_t f , const char * pfn , const char * fn ) ;
	static bool set_format( v4l2_format * , int fd , unsigned int dx , unsigned int dy , g_uint32_t pf , 
		const char * pfn ) ;
	static pair_t xioctl( int fd , int request , void * arg , bool v ) ;
	static bool xioctl_ok( int fd , int request , void * arg , bool v = false ) ;
	static void xioctl_warn( int fd , int request , void * arg , bool v = false ) ;
	static void xioctl_check( int fd , int request , void * arg , bool v = false ) ;
	static bool xioctl_eagain( int fd , int request , void * arg , bool v = false ) ;
	static void xioctl_einval( int fd , int request , void * arg , const std::string & more , bool v = false ) ;
	static const char * xioctl_name ( int request ) ;
	static void throw_errno( const char * , int e ) ;

	static bool m_use_libv4l ;
	static int xioctl_imp( int fd , int request , void * arg ) ;
	static int open( const char * , int , mode_t ) ;
	static void close( int ) ;
	static void * mmap( void * , size_t , int , int , int fd , off_t ) ;
	static int read( int , void * , size_t ) ;

private:
	struct Cleanup
	{
		typedef std::vector<int> List ;
		static List m_list_lib ;
		static List m_list_raw ;
		static void fn( G::SignalSafe , const char * ) ;
		static void add( int , bool ) ;
		static void remove( int fd ) ;
	} ;
} ;

/// \class Gv::CaptureRequeuer
/// A RAII class to do ioctl(VIDIOC_QBUF) on a v4l buffer.
///
class Gv::CaptureRequeuer
{
public:
	typedef struct v4l2_buffer buffer_type ;
	int m_fd ;
	buffer_type & m_buffer ;
	CaptureRequeuer( int fd , buffer_type & buffer ) ;
	~CaptureRequeuer() ;
} ;

// ==

Gv::CaptureV4l::CaptureV4l( const std::string & dev_name , const std::string & dev_config ) :
	m_fd(-1) ,
	m_io(IO_METHOD_MMAP) ,
	m_dx(0) ,
	m_dy(0) ,
	m_active(false)
{
	// libv4l is not built in by default, but if it is built in it is enabled by default, 
	// but it can still be disabled at run time by setting this config item
	CaptureImp::m_use_libv4l = have_libv4l() ;
	if( G::Str::splitMatch(dev_config,"nolibv4l",";") )
		CaptureImp::m_use_libv4l = false ;

	static bool first = true ;
	if( first )
	{
		first = false ;
		G_LOG( "Gv::CaptureV4l::ctor: libv4l is " << (have_libv4l()?"":"not ") << "built-in" 
			<< ((have_libv4l()&&!CaptureImp::m_use_libv4l)?" but disabled by 'nolibv4l' device option":"") ) ;
	}

	m_fd = open_device( dev_name ) ;
	m_io = check_device( m_fd , dev_config ) ;
	m_buffer_scale = init_device( dev_config ) ;
	create_buffers( m_buffer_scale.m_buffersize , dev_config ) ;
	add_buffers() ;
	start() ;
	set_simple( dev_config ) ;
	G_ASSERT( m_dx > 0 && m_dy > 0 ) ;
}

Gv::CaptureV4l::~CaptureV4l()
{
	m_buffers.clear() ; // stop libv4l whingeing
	CaptureImp::close( m_fd ) ;
}

int Gv::CaptureV4l::fd() const
{
	return m_fd ;
}

unsigned int Gv::CaptureV4l::dx() const
{
	return m_dx ;
}

unsigned int Gv::CaptureV4l::dy() const
{
	return m_dy ;
}

bool Gv::CaptureV4l::active() const
{
	return m_active ;
}

std::string Gv::CaptureV4l::info() const
{
	std::ostringstream ss ;
	ss 
		<< "io=" << (m_simple?"simple-":"") << (m_io==IO_METHOD_READ?"read":(m_io==IO_METHOD_MMAP?"mmap":"userptr")) << " "
		<< "dx=" << m_dx << " dy=" << m_dy << " "
		<< "format=[" << m_format.name() << "] "
		<< "fourcc=[" << fourcc(m_format.id()) << "]" ;
	return ss.str() ;
}

void Gv::CaptureV4l::set_simple( const std::string & dev_config )
{
	m_simple = 
		m_io == IO_METHOD_READ && 
		m_format.is_simple() && 
		m_buffer_scale.m_buffersize == size_t(m_dx)*size_t(m_dy)*size_t(3U) &&
		dev_config.find("nosimple") == std::string::npos ;
}

bool Gv::CaptureV4l::simple() const
{
	return m_simple ;
}

bool Gv::CaptureV4l::read( unsigned char * p , size_t n )
{
	G_ASSERT( m_io == IO_METHOD_READ ) ;
	if( m_io != IO_METHOD_READ ) return false ;
	return read_frame_read_simple( p , n ) ;
}

bool Gv::CaptureV4l::read( CaptureCallback & callback )
{
	if( m_io == IO_METHOD_READ )
	{
		if( !read_frame_read(callback) )
			return false ;
	}
	else if( m_io == IO_METHOD_MMAP )
	{
		if( !read_frame_mmap(callback) )
			return false ;
	}
	else
	{
		if( !read_frame_userptr(callback) )
			return false ;
	}
	return true ;
}

bool Gv::CaptureV4l::read_frame_read_simple( unsigned char * p , size_t n )
{
	G_ASSERT( p != nullptr && n > 0U ) ;
	ssize_t rc = CaptureImp::read( m_fd , p , n ) ;
	if( rc < 0 )
	{
		int e = errno ;
		if( e == EAGAIN ) return false ;
		CaptureImp::throw_errno( "read" , e ) ;
	}
	return true ;
}

bool Gv::CaptureV4l::read_frame_read( CaptureCallback & callback )
{
	G_ASSERT( m_buffers.size() <= 1U ) ;
	G_ASSERT( m_buffer_scale.m_buffersize > 0U ) ;
	if( m_buffers.empty() )
		m_buffers.push_back( shared_ptr<CaptureBuffer>(new CaptureBuffer(m_buffer_scale.m_buffersize)) ) ;

	ssize_t rc = CaptureImp::read( m_fd , m_buffers[0]->begin() , m_buffers[0]->size() ) ;
	if( rc < 0 )
	{
		int e = errno ;
		if( e == EAGAIN ) return false ;
		CaptureImp::throw_errno( "read" , e ) ;
	}
	Gv::CaptureBuffer & buffer = *m_buffers[0] ;
	buffer.setFormat( m_format , m_buffer_scale ) ;
	callback( buffer ) ;
	return true ;
}

bool Gv::CaptureV4l::read_frame_mmap( CaptureCallback & callback )
{
	// dequeue a buffer
	static struct v4l2_buffer zero_buf ;
	struct v4l2_buffer buf = zero_buf ;
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	buf.memory = V4L2_MEMORY_MMAP ;
	if( CaptureImp::xioctl_eagain( m_fd , VIDIOC_DQBUF , &buf ) )
		return false ;

	// requeue it on return
	CaptureRequeuer requeuer( m_fd , buf ) ;

	G_ASSERT( m_buffers.size() >= 1U ) ;
	G_ASSERT( buf.index < m_buffers.size() ) ;
	Gv::CaptureBuffer & buffer = *m_buffers[buf.index] ;
	buffer.setFormat( m_format , m_buffer_scale ) ;
	callback( buffer ) ;
	return true ;
}

bool Gv::CaptureV4l::read_frame_userptr( CaptureCallback & callback )
{
	// dequeue a buffer
	static struct v4l2_buffer zero_buf ;
	static struct v4l2_buffer buf = zero_buf ;
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	buf.memory = V4L2_MEMORY_USERPTR ;
	if( CaptureImp::xioctl_eagain( m_fd , VIDIOC_DQBUF, &buf ) )
		return false ;

	// requeue it on return
	CaptureRequeuer requeuer( m_fd , buf ) ;

	// find it in our list
	G_ASSERT( m_buffers.size() >= 1U ) ;
	unsigned int i = 0 ;
	for( ; i < m_buffers.size() ; ++i )
	{
		if( buf.m.userptr == reinterpret_cast<unsigned long>(m_buffers[i]->begin()) &&
			buf.length == m_buffers[i]->size() )
				break ;
	}
	if( i >= m_buffers.size() )
		throw Capture::Error( "buffer not found" ) ;

	Gv::CaptureBuffer & buffer = *m_buffers[i] ;
	buffer.setFormat( m_format , m_buffer_scale ) ;
	callback( buffer ) ;
	return true ;
}

void Gv::CaptureV4l::stop()
{
	if( m_active )
	{
		if( m_io == IO_METHOD_READ )
			stop_capturing_read() ;
		else if( m_io == IO_METHOD_MMAP )
			stop_capturing_mmap( m_fd ) ;
		else
			stop_capturing_userptr( m_fd ) ;
		m_active = false ;
	}
}

void Gv::CaptureV4l::stop_capturing_read()
{
	// no-op
}

void Gv::CaptureV4l::stop_capturing_mmap( int fd )
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	CaptureImp::xioctl_warn( fd , VIDIOC_STREAMOFF , &type ) ;
}

void Gv::CaptureV4l::stop_capturing_userptr( int fd )
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	CaptureImp::xioctl_warn( fd , VIDIOC_STREAMOFF , &type ) ;
}

void Gv::CaptureV4l::add_buffers()
{
	if( m_io == IO_METHOD_READ )
		add_buffers_read() ;
	else if( m_io == IO_METHOD_MMAP )
		add_buffers_mmap( m_buffers , m_fd ) ;
	else
		add_buffers_userptr( m_buffers , m_fd ) ;
}

void Gv::CaptureV4l::add_buffers_read()
{
	// no-op
}

void Gv::CaptureV4l::add_buffers_mmap( CaptureBuffers & buffers , int fd )
{
	G_ASSERT( buffers.size() > 0U ) ;
	for( unsigned int i = 0 ; i < buffers.size() ; ++i )
	{
		static struct v4l2_buffer zero_buf ;
		struct v4l2_buffer buf = zero_buf ;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
		buf.memory= V4L2_MEMORY_MMAP ;
		buf.index= i ;

		CaptureImp::xioctl_check( fd , VIDIOC_QBUF , &buf ) ;
	}
}

void Gv::CaptureV4l::add_buffers_userptr( CaptureBuffers & buffers , int fd )
{
	G_ASSERT( buffers.size() > 0U ) ;
	for( unsigned int i = 0 ; i < buffers.size() ; ++i )
	{
		static struct v4l2_buffer zero_buf ;
		struct v4l2_buffer buf = zero_buf ;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
		buf.memory= V4L2_MEMORY_USERPTR ;
		buf.index = i ;
		buf.m.userptr = reinterpret_cast<unsigned long>( buffers[i]->begin() ) ;
		buf.length = buffers[i]->size() ;

		CaptureImp::xioctl_check( fd , VIDIOC_QBUF , &buf ) ;
	}
}

void Gv::CaptureV4l::start()
{
	if( !m_active )
	{
		if( m_io == IO_METHOD_READ )
			start_capturing_read() ;
		else if( m_io == IO_METHOD_MMAP )
			start_capturing_mmap( m_fd ) ;
		else
			start_capturing_userptr( m_fd ) ;
		m_active = true ;
	}
}

void Gv::CaptureV4l::start_capturing_read()
{
	// no-op
}

void Gv::CaptureV4l::start_capturing_mmap( int fd )
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	CaptureImp::xioctl_check( fd , VIDIOC_STREAMON , &type ) ;
}

void Gv::CaptureV4l::start_capturing_userptr( int fd )
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	CaptureImp::xioctl_check( fd , VIDIOC_STREAMON , &type ) ;
}

void Gv::CaptureV4l::create_buffers( size_type buffersize , const std::string & dev_config )
{
	unsigned int nbuffers = std::min( G::Str::toUInt( G::Str::head(G::Str::tail(dev_config,"buffers="),";",false) , "0" ) , 30U ) ;

	if( m_io == IO_METHOD_READ )
		create_buffers_read( m_buffers , buffersize , nbuffers ) ;
	else if( m_io == IO_METHOD_MMAP )
		create_buffers_mmap( m_buffers , m_fd , buffersize , nbuffers == 0U ? 2U : nbuffers ) ;
	else
		create_buffers_userptr( m_buffers , m_fd , buffersize , nbuffers == 0U ? 4U : nbuffers ) ;
}

void Gv::CaptureV4l::create_buffers_read( CaptureBuffers & buffers , size_type buffer_size , unsigned int nbuffers )
{
	G_LOG( "Gv::CaptureV4l::create_buffers_read: read buffers: request=" << nbuffers << " count=1 size=" << buffer_size ) ;
	// don't do buffers.push_back(...) here -- do it lazily in case we can do simple() reads
}

void Gv::CaptureV4l::create_buffers_mmap( CaptureBuffers & buffers , int fd , size_t buffersize , unsigned int nbuffers )
{
	// have the device create the mmap buffers -- how many buffers is negotiated
	static struct v4l2_requestbuffers zero_req ;
	struct v4l2_requestbuffers req = zero_req ;
	req.count = nbuffers ; // negotiated up or down
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	req.memory = V4L2_MEMORY_MMAP ;
	CaptureImp::xioctl_einval( fd , VIDIOC_REQBUFS , &req , "device does not support memory mapped streaming" ) ;
	if( req.count < 1 )
		throw Capture::Error( "insufficient buffer memory" ) ;

	// map the buffers
	for( unsigned int i = 0 ; i < req.count ; ++i )
	{
		// query the length and offset
		static struct v4l2_buffer zero_buf ;
		struct v4l2_buffer buf = zero_buf ;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
		buf.memory = V4L2_MEMORY_MMAP ;
		buf.index = i ;
		CaptureImp::xioctl_check( fd , VIDIOC_QUERYBUF , &buf ) ;

		if( i == 0 )
			G_LOG( "Gv::CaptureV4l::create_buffers_mmap: mmap buffers: request=" << nbuffers << " count=" << req.count << " size=" << buf.length ) ;

		if( buf.length < buffersize )
			throw Capture::Error( "buffer size mis-match between VIDIOC_QUERYBUF and VIDIOC_S_FMT" ) ; 

		void * p = CaptureImp::mmap( nullptr , buf.length , PROT_READ | PROT_WRITE , MAP_SHARED , fd , buf.m.offset ) ;
		if( p == MAP_FAILED )
		{
			int e = errno ;
			CaptureImp::throw_errno( "mmap" , e ) ; // others are unmapped via ~CaptureBuffer()
		}

		buffers.push_back( shared_ptr<CaptureBuffer>( new CaptureBuffer(buf.length,p,::v4l2_munmap) ) ) ;
	}
	G_ASSERT( !buffers.empty() ) ;
}

void Gv::CaptureV4l::create_buffers_userptr( CaptureBuffers & buffers , int fd , size_type buffersize_in , unsigned int nbuffers )
{
	const int page_size = getpagesize() ; // unistd.h
	if( page_size <= 0 ) 
		throw Capture::Error( "assertion failure: invalid pagesize" ) ;

	unsigned int page = static_cast<unsigned int>(page_size) ;
	size_t buffersize = (buffersize_in + page - 1U) & ~(page - 1U) ; // round up
	G_ASSERT( buffersize >= buffersize_in ) ;

	static struct v4l2_requestbuffers zero_req ;
	struct v4l2_requestbuffers req = zero_req ;
	req.count = nbuffers ;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	req.memory = V4L2_MEMORY_USERPTR ;
	CaptureImp::xioctl_einval( fd , VIDIOC_REQBUFS, &req , "device does not support user pointer i/o" ) ;

	if( req.count < 1 )
		throw Capture::Error( "invalid negotiated buffer count" ) ;

	G_LOG( "Gv::CaptureV4l::create_buffers_userptr: userptr buffers: request=" << nbuffers << " count=" << req.count << " size=" << buffersize ) ;

	for( unsigned int i = 0U ; i < req.count ; ++i )
	{
		buffers.push_back( shared_ptr<CaptureBuffer>( new CaptureBuffer(page_size,buffersize) ) ) ;
	}
	G_ASSERT( !buffers.empty() ) ;
}

int Gv::CaptureV4l::open_device( const std::string & dev_name )
{
	struct stat st ;
	int rc = stat( dev_name.c_str() , &st ) ;
	if( rc < 0 )
	{
		std::string help( G::is_linux() ? " or do \"modprobe vivi\" as root" : "" ) ;
		throw Capture::NoDevice( "[" + dev_name + "]: try device \"test\"" + help ) ;
	}

	if( !S_ISCHR(st.st_mode) )
		throw Capture::Error( "not a device: " + dev_name ) ;

	int fd = CaptureImp::open( dev_name.c_str() , O_RDWR | O_NONBLOCK , 0 ) ;
	if( fd < 0 )
		throw Capture::Error( "cannot open [" + dev_name + "]: " + G::Process::strerror(errno) ) ;

	::fcntl( fd , F_SETFD , ::fcntl(fd,F_GETFD) | FD_CLOEXEC ) ;
	return fd ;
}

Gv::CaptureV4l::io_method Gv::CaptureV4l::check_device( int fd , const std::string & dev_config )
{
	const bool config_mmap = G::Str::splitMatch(dev_config,"mmap",";") ;
	const bool config_read = G::Str::splitMatch(dev_config,"read",";") ;
	const bool config_userptr = G::Str::splitMatch(dev_config,"userptr",";") ;

	struct v4l2_capability cap ;
	CaptureImp::xioctl_einval( fd , VIDIOC_QUERYCAP, &cap , "device is not a V4L2 device" ) ;
	if( !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) )
		throw Capture::Error( "device is not a video capture device" ) ;

	const bool can_read = !! (cap.capabilities & V4L2_CAP_READWRITE) ;
	const bool can_stream = !! (cap.capabilities & V4L2_CAP_STREAMING) ;
	G_LOG( "Gv::CaptureV4l::check_device: device capabilities: can-read=" << can_read << " can-stream=" << can_stream ) ;

	const char * error = nullptr ;
	if( !can_read && !can_stream )
		error = "device does not support any i/o method" ;

	if( config_read && !can_read )
		error = "device does not support read i/o" ;

	if( ( config_userptr || config_mmap ) && !can_stream )
		error = "device does not support streaming i/o" ;

	// normally prefer mmap so that the device driver can fill the buffer in
	// the kernel without doing a copy to userspace, and we convert the
	// (normally) YUV mmap buffer to an RGB image in CaptureBuffer::copyTo() -- 
	// however, when using libv4l it is better to use a read() of the libv4l RGB 
	// image straight into our RGB image buffer

	io_method method = 
		CaptureImp::m_use_libv4l ? 
			( can_read ? IO_METHOD_READ : IO_METHOD_MMAP ) :
			( can_stream ? IO_METHOD_MMAP : IO_METHOD_READ ) ;

	if( config_read ) method = IO_METHOD_READ ;
	if( config_mmap ) method = IO_METHOD_MMAP ;
	if( config_userptr ) method = IO_METHOD_USERPTR ;

	G_LOG( "Gv::CaptureV4l::check_device: device io-method: "
		<< "require-mmap=" << config_mmap << " "
		<< "require-read=" << config_read << " "
		<< "require-userptr=" << config_userptr << " "
		<< "can-read=" << can_read << " "
		<< "can-stream=" << can_stream << ": "
		<< (error?error:(method==IO_METHOD_READ?"read":(method==IO_METHOD_MMAP?"mmap":"userptr")))
	) ;

	if( error != nullptr )
		throw Capture::Error( error ) ;

	return method ;
}

namespace
{
	int format_type_gross( const Gv::CaptureBufferFormat & f )
	{
		return f.is_grey() ? 2 : 0 ; // grey is bad
	}
	int format_type_fine( const Gv::CaptureBufferFormat & f )
	{
		if( f.is_rgb() ) return 0 ; // rgb is better
		if( f.is_yuv() ) return 1 ;
		return 2 ;
	}
	int format_depth( const Gv::CaptureBufferFormat & f )
	{
		unsigned int d = std::max( f.component(0).depth() , std::max(f.component(1).depth(),f.component(2).depth()) ) ;
		return d == 8 ? -1000 : -d ; // eight is best, otherwise bigger is better
	}
	int format_compression( const Gv::CaptureBufferFormat & f )
	{
		return f.is_yuv() ? (f.component(1).xshift() * f.component(1).yshift()) : 0 ;
	}
	bool format_less( const Gv::CaptureBufferFormat & a , const Gv::CaptureBufferFormat & b )
	{
		if( format_type_gross(a) != format_type_gross(b) ) return format_type_gross(a) < format_type_gross(b) ;
		if( format_depth(a) != format_depth(b) ) return format_depth(a) < format_depth(b) ;
		if( format_type_fine(a) != format_type_fine(b) ) return format_type_fine(a) < format_type_fine(b) ;
		if( format_compression(a) != format_compression(b) ) return format_compression(a) < format_compression(b) ;
		return false ;
	}
}

Gv::CaptureBufferScale Gv::CaptureV4l::init_device( const std::string & dev_config )
{
	// select the first input
	{
		int index = 0 ;
		CaptureImp::xioctl_warn( m_fd , VIDIOC_S_INPUT , &index , true ) ;
	}

	// cropping parameters persist across close/open (see 1.13.3) so start by 
	// resetting the cropping using S_SELECTION or S_CROP
	//
	unsigned int dx = 0 ;
	unsigned int dy = 0 ;
	{
		bool done = false ;
		{
			static struct v4l2_cropcap zero_cropcap ;
			struct v4l2_cropcap cropcap = zero_cropcap ;
			cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
			struct v4l2_crop crop ;
			if( CaptureImp::xioctl_ok( m_fd , VIDIOC_CROPCAP , &cropcap , true ) )
			{
				dx = cropcap.defrect.width ;
				dy = cropcap.defrect.height ;

				crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
				crop.c = cropcap.defrect; // reset to default
				done = CaptureImp::xioctl_ok( m_fd , VIDIOC_S_CROP , &crop , true ) ;
			}
		}
		if( !done ) // try using the newer selection API -- see 1.14.4 and 1.14.5
		{
			static struct v4l2_selection zero_selection ;
			struct v4l2_selection selection = zero_selection ;
			selection.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
			selection.target = V4L2_SEL_TGT_CROP_DEFAULT ;
			if( CaptureImp::xioctl_ok( m_fd , VIDIOC_G_SELECTION , &selection , true ) )
			{
				dx = selection.r.width ;
				dy = selection.r.height ;

				selection.target = V4L2_SEL_TGT_CROP ;
				done = CaptureImp::xioctl_ok( m_fd , VIDIOC_S_SELECTION , &selection , true ) ;
			}
		}
	}

	// show a list of the device's supported formats (not a well-supported xioctl)
	{
		const int sanity = 1000 ;
		for( int i = 0 ; i < sanity ; i++ )
		{
			static struct v4l2_fmtdesc zero_fmtdesc ;
			struct v4l2_fmtdesc fmtdesc = zero_fmtdesc ;
			fmtdesc.index = i ;

			if( ! CaptureImp::xioctl_ok( m_fd , VIDIOC_ENUM_FMT , &fmtdesc , true ) )
				break ;

			if( fmtdesc.type != V4L2_BUF_TYPE_VIDEO_CAPTURE )
				continue ; 

			const char * description = reinterpret_cast<char *>(fmtdesc.description) ;
			fmtdesc.description[sizeof(fmtdesc.description)-1] = '\0' ;

			G_LOG( "Gv::CaptureV4l::init_device: device format " << i << ": "
				<< "pixelformat=" << fmtdesc.pixelformat << " "
				<< "fourcc=[" << fourcc(fmtdesc.pixelformat) << "] "
				<< "description=[" << G::Str::printable(description) << "] "
				<< "flags=" << fmtdesc.flags ) ;
		}
	}

	// prepare a list of our supported formats -- device drivers are not supposed to do
	// conversions in kernel space, so we have to make an effort to support a reasonable 
	// number in case we do not have libv4l built in -- the format descriptions are 
	// interpreted by Gv::CaptureBuffer in-line methods
	//
	std::vector<CaptureBufferFormat> format ;
	{
		typedef CaptureBufferFormat Format ;
		typedef CaptureBufferFormat::Type Type ;
		typedef CaptureBufferComponent Component ;
		const CaptureBufferComponent::Type c_byte = CaptureBufferComponent::c_byte ;
		const CaptureBufferComponent::Type c_word_le = CaptureBufferComponent::c_word_le ;
		const CaptureBufferComponent::Type c_word_be = CaptureBufferComponent::c_word_be ;
		const size_t XY = Gv::CaptureBufferComponent::XY ;
		const size_t XY2 = Gv::CaptureBufferComponent::XY2 ;
		const size_t XY4 = Gv::CaptureBufferComponent::XY4 ;
		const size_t XY8 = Gv::CaptureBufferComponent::XY8 ;

		static const Format ff[] = {
			Format( V4L2_PIX_FMT_RGB332 , "rgb332" , Format::rgb ,
				// offset, x_shift, y_shift, step, depth, shift, wordsize
				Component( 0 , 0 , 0 , 1 , 3 , 5 , c_byte ) ,
				Component( 0 , 0 , 0 , 1 , 3 , 2 , c_byte ) ,
				Component( 0 , 0 , 0 , 1 , 2 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_RGB565X , "rgb565x" , Format::rgb ,
				Component( 0 , 0 , 0 , 2 , 5 , 11 , c_word_be ) ,
				Component( 0 , 0 , 0 , 2 , 6 , 5 , c_word_be ) ,
				Component( 0 , 0 , 0 , 2 , 5 , 0 , c_word_be ) ) ,
			Format( V4L2_PIX_FMT_RGB24 , "rgb24" , Format::rgb ,
				Component( 0 , 0 , 0 , 3 , 8 , 0 , c_byte ) ,
				Component( 1 , 0 , 0 , 3 , 8 , 0 , c_byte ) ,
				Component( 2 , 0 , 0 , 3 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_BGR24 , "bgr24" , Format::rgb ,
				Component( 2 , 0 , 0 , 3 , 8 , 0 , c_byte ) ,
				Component( 1 , 0 , 0 , 3 , 8 , 0 , c_byte ) ,
				Component( 0 , 0 , 0 , 3 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_RGB444 , "rgb444" , Format::rgb ,
				Component( 0 , 0 , 0 , 2 , 4 , 8 , c_word_le ) ,
				Component( 0 , 0 , 0 , 2 , 4 , 4 , c_word_le ) ,
				Component( 0 , 0 , 0 , 2 , 4 , 0 , c_word_le ) ) ,
			Format( V4L2_PIX_FMT_RGB555 , "rgb555" , Format::rgb ,
				Component( 0 , 0 , 0 , 2 , 5 , 10 , c_word_le ) ,
				Component( 0 , 0 , 0 , 2 , 5 , 5 , c_word_le ) ,
				Component( 0 , 0 , 0 , 2 , 5 , 0 , c_word_le ) ) ,
			Format( V4L2_PIX_FMT_RGB555X , "rgb555x" , Format::rgb ,
				Component( 0 , 0 , 0 , 2 , 5 , 10 , c_word_be ) ,
				Component( 0 , 0 , 0 , 2 , 5 , 5 , c_word_be ) ,
				Component( 0 , 0 , 0 , 2 , 5 , 0 , c_word_be ) ) ,
			Format( V4L2_PIX_FMT_BGR32 , "bgr32" , Format::rgb ,
				Component( 2 , 0 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 1 , 0 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 0 , 0 , 0 , 4 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_RGB32 , "rgb32" , Format::rgb ,
				Component( 1 , 0 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 2 , 0 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 3 , 0 , 0 , 4 , 8 , 0 , c_byte ) ) ,
			//
			Format( V4L2_PIX_FMT_GREY , "grey" , Format::grey ,
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_byte ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_Y10 , "y10" , Format::grey ,
				Component( 0 , 0 , 0 , 1 , 10 , 0 , c_word_le ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_word_le ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_word_le ) ) ,
			Format( V4L2_PIX_FMT_Y12 , "y12" , Format::grey ,
				Component( 0 , 0 , 0 , 1 , 12 , 0 , c_word_le ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_word_le ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_word_le ) ) ,
			Format( V4L2_PIX_FMT_Y16 , "y16" , Format::grey ,
				Component( 0 , 0 , 0 , 1 , 16 , 0 , c_word_le ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_word_le ) ,
				Component( 0 , 0 , 0 , 1 , 0 , 0 , c_word_le ) ) ,
			//
			Format( V4L2_PIX_FMT_YUYV , "yuyv" , Format::yuv ,
				Component( 0 , 0 , 0 , 2 , 8 , 0 , c_byte ) ,
				Component( 1 , 1 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 3 , 1 , 0 , 4 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_YVYU , "yvyu" , Format::yuv ,
				Component( 0 , 0 , 0 , 2 , 8 , 0 , c_byte ) ,
				Component( 3 , 1 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 1 , 1 , 0 , 4 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_UYVY , "uyvy" , Format::yuv ,
				Component( 1 , 0 , 0 , 2 , 8 , 0 , c_byte ) ,
				Component( 0 , 1 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 2 , 1 , 0 , 4 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_VYUY , "vyuy" , Format::yuv ,
				Component( 1 , 0 , 0 , 2 , 8 , 0 , c_byte ) ,
				Component( 2 , 1 , 0 , 4 , 8 , 0 , c_byte ) ,
				Component( 0 , 1 , 0 , 4 , 8 , 0 , c_byte ) ) ,
			Format( V4L2_PIX_FMT_YVU420 , "yvu420" , Format::yuv , // YUV 4:2:0
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY+XY4 , 1 , 1 , 1 , 8 , 0 , c_byte , 1 ) ,
				Component( XY , 1 , 1 , 1 , 8 , 0 , c_byte , 1 ) ) ,
			Format( V4L2_PIX_FMT_YUV420 , "yuv420" , Format::yuv ,
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 1 , 1 , 1 , 8 , 0 , c_byte , 1 ) ,
				Component( XY+XY4 , 1 , 1 , 1 , 8 , 0 , c_byte , 1 ) ) ,
			Format( V4L2_PIX_FMT_YVU410 , "yvu410" , Format::yuv , // YUV 4:1:0
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY+XY8 , 2 , 2 , 1 , 8 , 0 , c_byte , 2 ) ,
				Component( XY , 2 , 2 , 1 , 8 , 0 , c_byte , 2 ) ) ,
			Format( V4L2_PIX_FMT_YUV410 , "yuv410" , Format::yuv ,
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 2 , 2 , 1 , 8 , 0 , c_byte , 2 ) ,
				Component( XY+XY8 , 2 , 2 , 1 , 8 , 0 , c_byte , 2 ) ) ,
			Format( V4L2_PIX_FMT_YUV422P , "yuv422p" , Format::yuv , // YUV 4:2:2
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 1 , 0 , 1 , 8 , 0 , c_byte , 1 ) ,
				Component( XY+XY2 , 1 , 0 , 1 , 8 , 0 , c_byte , 1 ) ) ,
			Format( V4L2_PIX_FMT_YUV411P , "yuv411p" , Format::yuv , // YUV 4:1:1
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 2 , 0 , 1 , 8 , 0 , c_byte , 2 ) ,
				Component( XY+XY4 , 2 , 0 , 1 , 8 , 0 , c_byte , 2 ) ) ,
			Format( V4L2_PIX_FMT_NV12 , "nv12" , Format::yuv , // YUV 4:2:0
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 1 , 1 , 2 , 8 , 0 , c_byte , 0 ) ,
				Component( XY+1 , 1 , 1 , 2 , 8 , 0 , c_byte , 0 ) ) ,
			Format( V4L2_PIX_FMT_NV21 , "nv21" , Format::yuv ,
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY+1 , 1 , 1 , 2 , 8 , 0 , c_byte , 0 ) ,
				Component( XY , 1 , 1 , 2 , 8 , 0 , c_byte , 0 ) ) ,
			Format( V4L2_PIX_FMT_NV16 , "nv16" , Format::yuv , // YUV 4:2:2
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 1 , 0 , 2 , 8 , 0 , c_byte , 0 ) ,
				Component( XY+1 , 1 , 0 , 2 , 8 , 0 , c_byte , 0 ) ) ,
			Format( V4L2_PIX_FMT_NV61 , "nv61" , Format::yuv ,
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY+1 , 1 , 0 , 2 , 8 , 0 , c_byte , 0 ) ,
				Component( XY , 1 , 0 , 2 , 8 , 0 , c_byte , 0 ) ) ,
			Format( V4L2_PIX_FMT_NV24 , "nv24" , Format::yuv , // YUV 4:4:4
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY , 0 , 0 , 2 , 8 , 0 , c_byte , -1 ) ,
				Component( XY+1 , 0 , 0 , 2 , 8 , 0 , c_byte , -1 ) ) ,
			Format( V4L2_PIX_FMT_NV42 , "nv42" , Format::yuv ,
				Component( 0 , 0 , 0 , 1 , 8 , 0 , c_byte ) ,
				Component( XY+1 , 0 , 0 , 2 , 8 , 0 , c_byte , -1 ) ,
				Component( XY , 0 , 0 , 2 , 8 , 0 , c_byte , -1 ) )
		} ;

		// build the vector of supported formats
		size_t n = sizeof(ff)/sizeof(ff[0]) ;
		format.reserve( n ) ;
		for( unsigned int f = 0 ; f < n ; f++ )
			format.push_back( ff[f] ) ;

		// set a priority order
		std::stable_sort( format.begin() , format.end() , format_less ) ;

		// log the formats in priority order
		for( unsigned int f = 0 ; f < format.size() ; f++ )
		{
			G_LOG( "Gv::CaptureV4l::init_device: supported format: "
				<< "index=" << f << " "
				<< "name=[" << format[f].name() << "] "
				<< "pixelformat=" << format[f].id() << " "
				<< "fourcc=[" << fourcc(format[f].id()) << "] "
				<< "type=" << int(format[f].type()) ) ;
		}
	}

	// parse out a preferred format from the device config
	unsigned int first_format = G::Str::toUInt( G::Str::head(G::Str::tail(dev_config,"fmt="),";",false) , "0" ) ;

	// negotiate a format -- go through our list in perferred order and
	// do 'try' and 'set' ioctls until something works
	//
	CaptureBufferScale scale ;
	for( unsigned int f = 0 ; f < format.size() ; f++ )
	{
		int i = (f+first_format) % format.size() ;
		v4l2_format fmt ;
		if( CaptureImp::set_format( &fmt , m_fd , dx , dy , format[i].id() , format[i].name() ) )
		{
			G_LOG( "Gv::CaptureV4l::init_device: format accepted by device: "
				<< "index=" << i << " "
				<< "name=[" << format[i].name() << "] "
				<< "pixelformat=" << format[i].id() << " " 
				<< "fourcc=[" << fourcc(format[i].id()) << "] "
				<< "sizeimage=" << fmt.fmt.pix.sizeimage << " "
				<< "bytesperline=" << fmt.fmt.pix.bytesperline << " "
				<< "(" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << ")"
			) ;

			m_dx = fmt.fmt.pix.width ;
			m_dy = fmt.fmt.pix.height ;
			m_format = format[i] ;
			scale = CaptureBufferScale( fmt.fmt.pix.sizeimage , fmt.fmt.pix.bytesperline , fmt.fmt.pix.width , fmt.fmt.pix.height ) ;
			break ;
		}
		else
		{
			G_LOG( "Gv::CaptureV4l::init_device: format rejected by device: "
				<< "index=" << i << " "
				<< "name=[" << format[i].name() << "] "
				<< "pixelformat=" << format[i].id() << " " 
				<< "fourcc=[" << fourcc(format[i].id()) << "]: rejected" ) ;
		}
	}
	if( scale.m_buffersize == 0U )
		throw Capture::Error( "failed to negotiate a pixel format" ) ;
	return scale ; // buffersize, linesize, dx, dy
}

bool Gv::CaptureImp::set_format( v4l2_format * out_p , int fd , unsigned int dx , unsigned int dy , 
	g_uint32_t pixelformat , const char * name )
{
	return
		set_format( out_p , fd , dx , dy , pixelformat , V4L2_FIELD_NONE , name , "none" ) ||
		set_format( out_p , fd , dx , dy , pixelformat , V4L2_FIELD_INTERLACED , name , "interlaced" ) ||
		set_format( out_p , fd , dx , dy , pixelformat , V4L2_FIELD_SEQ_TB , name , "top-bottom" ) ||
		set_format( out_p , fd , dx , dy , pixelformat , V4L2_FIELD_SEQ_BT , name , "bottom-top" ) ||
		set_format( out_p , fd , dx , dy , pixelformat , V4L2_FIELD_INTERLACED_TB , name , "interlaced-top-bottom" ) ||
		set_format( out_p , fd , dx , dy , pixelformat , V4L2_FIELD_INTERLACED_BT , name , "interlaced-bottom-top" ) ||
		false ;
}

bool Gv::CaptureImp::set_format( v4l2_format * out_p , int fd , unsigned int dx , unsigned int dy , 
	g_uint32_t pixelformat , g_uint32_t field , const char * format_name , const char * field_name )
{
	static struct v4l2_format zero_fmt ;
	struct v4l2_format fmt = zero_fmt ;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
	fmt.fmt.pix.width = dx ? dx : 640 ;
	fmt.fmt.pix.height = dy ? dy : 480 ;
	fmt.fmt.pix.pixelformat = pixelformat ;
	fmt.fmt.pix.field = field ;

	bool try_ok = CaptureImp::xioctl_ok( fd , VIDIOC_TRY_FMT , &fmt , true ) ;
	bool ok =
		fmt.fmt.pix.width > 8 && fmt.fmt.pix.height > 8 &&
		fmt.fmt.pix.pixelformat == pixelformat && fmt.fmt.pix.field == field ;

	if( try_ok && !ok )
	{
		G_DEBUG( "Gv::CaptureImp::set_format: ioctl try_fmt: " << try_ok << ": " 
			<< pixelformat << "->" << fmt.fmt.pix.pixelformat << " " 
			<< field << "->" << fmt.fmt.pix.field ) ;
	}
	else
	{
		CaptureImp::xioctl_check( fd , VIDIOC_S_FMT , &fmt , true ) ;
		ok =
			fmt.fmt.pix.width > 8 && fmt.fmt.pix.height > 8 &&
			fmt.fmt.pix.pixelformat == pixelformat && fmt.fmt.pix.field == field ;
	}

	G_DEBUG( "Gv::CaptureV4l::init_device: format negiotiation: "
		<< "[" << format_name << "," << field_name << "]: " << (ok?"accepted":"rejected") ) ;

	if( ok )
	{
		*out_p = fmt ;
		return true ;
	}
	else
	{
		return false ;
	}
}

// ==

std::pair<int,int> Gv::CaptureImp::xioctl( int fd , int request , void * arg , bool v )
{
	int rc ;
	do
	{
		rc = xioctl_imp( fd , request , arg ) ;
	} while( rc == -1 && errno == EINTR ) ;
	int e = rc >= 0 ? 0 : errno ;

	if( v ) 
	{
		G_DEBUG( "Gv::CaptureImp::xioctl: "
			<< "fd=" << fd << " "
			<< "request=" << xioctl_name(request) << "(" << static_cast<unsigned int>(request) << ") "
			<< "rc=" << rc << " "
			<< "errno=" << e ) ;
	}
	return std::make_pair( rc , e ) ;
}

bool Gv::CaptureImp::xioctl_ok( int fd , int request , void * arg , bool v )
{
	pair_t p = xioctl( fd , request , arg , v ) ;
	return p.first >= 0 ;
}

void Gv::CaptureImp::xioctl_warn( int fd , int request , void * arg , bool v )
{
	pair_t p = xioctl( fd , request , arg , v ) ;
	if( p.first < 0 )
		G_WARNING( "CaptureImp::xioctl: ioctl " << xioctl_name(request) << ": " << G::Process::strerror(p.second) ) ;
}

void Gv::CaptureImp::xioctl_check( int fd , int request , void * arg , bool v )
{
	pair_t p = xioctl( fd , request , arg , v ) ;
	if( p.first < 0 )
		throw_errno( xioctl_name(request) , p.second ) ;
}

bool Gv::CaptureImp::xioctl_eagain( int fd , int request , void * arg , bool v )
{
	pair_t p = xioctl( fd , request , arg , v ) ;
	if( p.first == -1 )
	{
		if( p.second == EAGAIN ) return true ;
		throw_errno( xioctl_name(request) , p.second ) ;
	}
	return false ;
}

void Gv::CaptureImp::xioctl_einval( int fd , int request , void * arg , const std::string & more , bool v )
{
	pair_t p = xioctl( fd , request , arg , v ) ;
	if( p.first == -1 )
	{
		if( p.second == EINVAL )
			throw Capture::Error( more ) ;
		throw_errno( xioctl_name(request) , p.second ) ;
	}
}

const char * Gv::CaptureImp::xioctl_name( int request )
{
	for( const ioctl_name * np = ioctl_names ; np->p ; np++ )
	{
		if( static_cast<int>(np->n) == request )
			return np->p ;
	}
	return "xioctl" ;
}

void Gv::CaptureImp::throw_errno( const char * op , int e )
{
	std::ostringstream ss ;
	ss << op << " error " << e << ", " << G::Process::strerror(e) ;
	if( e == ENOSPC )
		ss << " (not enough spare usb bandwidth)" ;
	if( e == ENODEV )
		throw Capture::NoDevice( ss.str() ) ;
	else
		throw Capture::Error( ss.str() ) ;
}

bool Gv::CaptureImp::m_use_libv4l = true ;

int Gv::CaptureImp::xioctl_imp( int fd , int request , void * arg )
{
	return m_use_libv4l ? v4l2_ioctl( fd , request , arg ) : ::ioctl( fd , request , arg ) ;
}

int Gv::CaptureImp::open( const char * p , int n , mode_t mode )
{
	int fd = -1 ;
	{
		G::Root claim_root ;
		fd = m_use_libv4l ? v4l2_open( p , n , mode ) : ::open( p , n , mode ) ;
	}
	Cleanup::add( fd , m_use_libv4l ) ;
	return fd ;
}

void Gv::CaptureImp::close( int fd )
{
	if( m_use_libv4l ) v4l2_close( fd ) ; else ::close( fd ) ;
	Cleanup::remove( fd ) ;
}

void * Gv::CaptureImp::mmap( void * p , size_t n , int prot , int flags , int fd , off_t offset )
{
	G::Root claim_root ;
	return m_use_libv4l ? v4l2_mmap( p , n , prot , flags , fd , offset ) : ::mmap( p , n , prot , flags , fd , offset ) ;
}

int Gv::CaptureImp::read( int fd , void * buffer , size_t n )
{
	return m_use_libv4l ? v4l2_read( fd , buffer , n ) : ::read( fd , buffer , n ) ;
}

// ==

Gv::CaptureRequeuer::CaptureRequeuer( int fd , buffer_type & buffer ) :
	m_fd(fd),
	m_buffer(buffer)
{
}

Gv::CaptureRequeuer::~CaptureRequeuer()
{
	CaptureImp::xioctl_warn( m_fd , VIDIOC_QBUF , &m_buffer ) ;
}

// ==

// work-round for linux usb bug "not enough bandwidth for altsetting" ...

Gv::CaptureImp::Cleanup::List Gv::CaptureImp::Cleanup::m_list_lib ;
Gv::CaptureImp::Cleanup::List Gv::CaptureImp::Cleanup::m_list_raw ;

void Gv::CaptureImp::Cleanup::fn( G::SignalSafe , const char * )
{
	for( List::iterator p = m_list_lib.begin() ; p != m_list_lib.end() ; ++p )
		v4l2_close( *p ) ;
	for( List::iterator p = m_list_raw.begin() ; p != m_list_raw.end() ; ++p )
		::close( *p ) ;
}

void Gv::CaptureImp::Cleanup::add( int fd , bool lib )
{
	static bool first = true ;
	if( first )
		G::Cleanup::add( CaptureImp::Cleanup::fn , nullptr ) ;
	first = false ;

	remove( fd ) ;
	List & list = lib ? m_list_lib : m_list_raw ;
	list.push_back( fd ) ;
}

void Gv::CaptureImp::Cleanup::remove( int fd )
{
	m_list_lib.erase( std::remove(m_list_lib.begin(),m_list_lib.end(),fd) , m_list_lib.end() ) ;
	m_list_raw.erase( std::remove(m_list_raw.begin(),m_list_raw.end(),fd) , m_list_raw.end() ) ;
}


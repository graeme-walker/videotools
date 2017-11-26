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
// gfatpipe.cpp
// 

#include "gdef.h"
#include "gfatpipe.h"
#include "gmsg.h"
#include "gcleanup.h"
#include "gprocess.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"
#include <stdexcept>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

std::string G::FatPipe::name()
{
	std::ostringstream ss ;
	ss << "fatpipe." << getpid() ;
	return ss.str() ;
}

std::string G::FatPipe::name( size_t size )
{
	std::ostringstream ss ;
	ss << "fatpipe." << getpid() << "." << size ;
	return ss.str() ;
}

size_t G::FatPipe::sensible( size_t size )
{
	return std::max( size_t(1024U)*16U , size ) ;
}

G::FatPipe::FatPipe() :
	m_child(false) ,
	m_parent(false) ,
	m_shmem(name(),sizeof(ControlMemory),SharedMemory::Control()) ,
	m_pipe_fd(-1) ,
	m_new_data_fd(-1)
{
	if( ::socketpair( AF_UNIX , SOCK_DGRAM , 0 , m_pipe_fds ) < 0 )
		throw Error( "socketpair pipe creation failed" ) ;

	m_shmem.inherit() ; // inherit fd across exec()
	m_shmem.unlink() ;

	ControlMemory * mem = static_cast<ControlMemory*>(m_shmem.ptr()) ;
	mem->magic = ControlMemory::MAGIC ;
	new (&mem->mutex) G::Semaphore ; // placement new
}

G::FatPipe::~FatPipe()
{
	if( m_parent && m_pipe_fd != -1 ) 
		G::Msg::send( m_pipe_fd , "x" , 1 , MSG_DONTWAIT ) ;

	close_( m_pipe_fds[0] ) ;
	close_( m_pipe_fds[1] ) ;
	close_( m_pipe_fd ) ;

	//ControlMemory * mem = static_cast<ControlMemory*>(m_shmem.ptr()) ;
	//Semaphore * sem = G::Semaphore::at(&mem->mutex) ;
	//sem->~Semaphore() ; // placement delete
}

void G::FatPipe::cleanup( G::SignalSafe , const char * fd_p )
{
	char * end = const_cast<char*>( fd_p + std::strlen(fd_p) ) ;
	long pipe_fd = ::strtol( fd_p , &end , 10 ) ; 
	if( pipe_fd >= 0L )
		G::Msg::send( static_cast<int>(pipe_fd) , "x" , 1 , MSG_DONTWAIT ) ;
}

void G::FatPipe::close_( int fd )
{
	if( fd != -1 )
		::close( fd ) ;
}

void G::FatPipe::doChild()
{
	try
	{
		m_child = true ;
		::close( m_pipe_fds[1] ) ; // close write end
		m_pipe_fd = m_pipe_fds[0] ; // read end
		m_pipe_fds[0] = m_pipe_fds[1] = -1 ;

		// no close on exec
		::fcntl( m_pipe_fd , F_SETFD , ::fcntl(m_pipe_fd,F_GETFD) & ~FD_CLOEXEC ) ; 

		m_arg_shmemfd = tostring(m_shmem.fd()) ;
		m_arg_pipefd = tostring(m_pipe_fd) ;
	}
	catch(...)
	{
	}
}

std::string G::FatPipe::tostring( int n )
{
	std::ostringstream ss ; 
	ss << n ;
	return ss.str() ;
}

const char * G::FatPipe::pipefd() const
{
	return m_arg_pipefd.c_str() ;
}

const char * G::FatPipe::shmemfd() const
{
	return m_arg_shmemfd.c_str() ;
}

void G::FatPipe::doParent( bool auto_cleanup )
{
	m_parent = true ;
	::close( m_pipe_fds[0] ) ; // close read end
	m_pipe_fd = m_pipe_fds[1] ; // write end
	m_pipe_fds[0] = m_pipe_fds[1] = -1 ;

	if( auto_cleanup )
	{
		static std::list<std::string> arg_list ;
		arg_list.push_back( G::Str::fromInt(m_pipe_fd) ) ;
		G::Cleanup::add( cleanup , arg_list.back().c_str() ) ;
	}
}

void G::FatPipe::send( const char * data_p , size_t data_n , const char * type )
{
	if( data_p == nullptr || data_n == 0U ) return ;
	m_data_p.resize( 1U ) ;
	m_data_n.resize( 1U ) ;
	m_data_p[0] = data_p ;
	m_data_n[0] = data_n ;
	send( data_n , 1U , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::FatPipe::send( const std::vector<char> & data , const char * type )
{
	if( data.empty() ) return ;
	m_data_p.resize( 1U ) ;
	m_data_n.resize( 1U ) ;
	m_data_p[0] = &data[0] ;
	m_data_n[0] = data.size() ;
	send( data.size() , 1U , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::FatPipe::send( const std::vector<std::pair<const char *,size_t> > & data , const char * type )
{
	m_data_p.resize( data.size() ) ;
	m_data_n.resize( data.size() ) ;
	size_t data_total = 0U ;
	for( size_t i = 0U ; i < data.size() ; i++ )
	{
		m_data_p[i] = data[i].first ;
		m_data_n[i] = data[i].second ;
		data_total += data[i].second ;
	}
	if( data_total != 0U )
		send( data_total , data.size() , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::FatPipe::send( const std::vector<std::vector<char> > & data , const char * type )
{
	m_data_p.resize( data.size() ) ;
	m_data_n.resize( data.size() ) ;
	size_t data_total = 0U ;
	size_t i_out = 0U ;
	for( size_t i_in = 0U ; i_in < data.size() ; i_in++ )
	{
		if( data[i_in].empty() ) continue ;
		m_data_p[i_out] = &(data[i_in])[0] ;
		m_data_n[i_out++] = data[i_in].size() ;
		data_total += data[i_in].size() ;
	}
	if( data_total != 0U )
		send( data_total , i_out , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::FatPipe::send( size_t data_total , size_t data_count , const char ** data_p_p , size_t * data_n_p , const char * type )
{
	// create the data segment on first use, or resize it if it's too small
	//
	if( m_shmem_data.get() == nullptr || (sizeof(DataMemory)+data_total) > m_shmem_data->size() )
	{
		size_t new_size = sizeof(DataMemory) + data_total + data_total/2U ;
		G_DEBUG( "G::FatPipe::send: new data segment: size " << new_size ) ;
		m_shmem_data.reset( new SharedMemory( name(new_size) , new_size ) ) ;
		m_new_data_fd = m_shmem_data->fd() ;
		G_DEBUG( "G::FatPipe::send: new data segment: fd " << m_new_data_fd ) ;
		m_shmem_data->unlink() ;
		DataMemory * dmem = static_cast<DataMemory*>(m_shmem_data->ptr()) ;
		dmem->magic = DataMemory::MAGIC ;
		dmem->type[0] = '\0' ;
		dmem->data_size = 0U ;
		dmem->time_s = 0 ;
		dmem->time_us = 0U ;
	}

	// lock the control segment and copy the payload into the data segment
	//
	ControlMemory * mem = static_cast<ControlMemory*>(m_shmem.ptr()) ;
	DataMemory * dmem = static_cast<DataMemory*>(m_shmem_data->ptr()) ;
	{
		G::EpochTime time = G::DateTime::now() ;
		Lock lock( G::Semaphore::at(&mem->mutex) ) ;
		G_ASSERT( mem->magic == ControlMemory::MAGIC ) ;
		G_ASSERT( dmem->magic == DataMemory::MAGIC ) ;
		::memset( dmem->type , 0 , sizeof(dmem->type) ) ;
		if( type != nullptr ) ::strncpy( dmem->type , type , sizeof(dmem->type)-1U ) ;
		for( char * out = dmem->data ; data_count ; out += *data_n_p , data_count-- , data_p_p++ , data_n_p++ )
			::memcpy( out , *data_p_p , *data_n_p ) ;
		dmem->data_size = data_total ;
		dmem->time_s = time.s ;
		dmem->time_us = time.us ;
	}

	// poke the child, sending it the new data segment fd if necessary
	//
	if( m_pipe_fd != -1 )
	{
		bool wait = m_new_data_fd != -1 ;
		int flags = wait ? 0 : MSG_DONTWAIT ;
		ssize_t rc = G::Msg::send( m_pipe_fd , "." , 1 , flags , m_new_data_fd ) ;
		if( rc == 1 )
		{
			m_new_data_fd = -1 ; // transferred ok
		}
		else
		{
			int e = errno ;
			const bool temporary = e == ENOBUFS ; // not used on linux
			if( !temporary )
				throw Error( "pipe write failed" , G::Process::strerror(e) ) ;
		}
	}
}

bool G::FatPipe::ping()
{
	if( m_pipe_fd == -1 ) return false ;
	ssize_t rc = G::Msg::send( m_pipe_fd , "p" , 1 , MSG_DONTWAIT ) ;
	return rc == 1 ;
}

// ==

G::FatPipeReceiver::FatPipeReceiver( int shmem_fd , int pipe_fd ) :
	m_pipe_fd(pipe_fd) ,
	m_shmem(shmem_fd)
{
}

int G::FatPipeReceiver::pipefd() const
{
	return m_pipe_fd ;
}

G::FatPipeReceiver::Info G::FatPipeReceiver::flush( int pipe_fd )
{
	// read whatever's in the pipe -- see if there's a "." 
	// payload and/or a new data segment fd
	//
	Info result ;
	result.got_data = false ;
	result.got_eof = false ;
	result.data_fd = -1 ;
	for(;;)
	{
		char c = '\0' ;
		int fd = -1 ;
		ssize_t rc = G::Msg::recv( pipe_fd , &c , 1 , MSG_DONTWAIT , &fd ) ;
		G_DEBUG( "G::FatPipeReceiver::flush: recv() rc=" << rc << " c=" << int(c) << " fd=" << fd ) ;
		if( fd != -1 )
			result.data_fd = fd ;
		if( rc == 1 && c == '.' )
			result.got_data = true ;
		else if( rc == 1 && c == 'x' )
			result.got_eof = true ;
		else if( rc == 1 && c == 'p' )
			; // ping
		else
			break ;
	}
	return result ;
}

bool G::FatPipeReceiver::receive( std::vector<char> & buffer , std::string * type_p , G::EpochTime * time_p )
{
	// clear the outputs
	buffer.clear() ;
	if( type_p != nullptr )
		type_p->clear() ;

	// read everything in the pipe
	Info pipe_info = flush( m_pipe_fd ) ; 
	G_DEBUG( "G::FatPipeReceiver::receive: got-data=" << pipe_info.got_data << " data-fd=" << pipe_info.data_fd ) ;
	if( pipe_info.got_eof )
		return false ;

	// maintain the data segment
	if( m_shmem_data.get() == nullptr && pipe_info.got_data && pipe_info.data_fd == -1 )
	{
		G_WARNING( "G::FatPipeReceiver::receive: data segment file descriptor was not received" ) ;
		return false ;
	}
	if( pipe_info.data_fd != -1 )
	{
		G_DEBUG( "G::FatPipeReceiver::receive: new data segment fd: " << pipe_info.data_fd ) ;
		m_shmem_data.reset( new SharedMemory(pipe_info.data_fd) ) ;
	}

	// return an empty buffer if no data
	if( !pipe_info.got_data )
		return true ;

	// read the data
	G_ASSERT( m_shmem_data.get() != nullptr ) ;
	copy( m_shmem , *m_shmem_data.get() , buffer , type_p , time_p ) ;

	// trim padding from the type string
	if( type_p && type_p->find('\0') != std::string::npos )
		type_p->resize( type_p->find('\0') ) ;

	return true ;
}

void G::FatPipeReceiver::copy( SharedMemory & shmem , SharedMemory & shmem_data , 
	std::vector<char> & buffer , std::string * type_p , G::EpochTime * time_p )
{
	ControlMemory * mem = static_cast<ControlMemory*>( shmem.ptr() ) ;
	if( mem->magic != ControlMemory::MAGIC ) // no-mutex read of constant data
		throw Error( "magic number mismatch in shared memory" ) ;

	DataMemory * dmem = static_cast<DataMemory*>( shmem_data.ptr() ) ;
	G::Semaphore * mutex = G::Semaphore::at( &mem->mutex ) ;
	{
		Lock lock( mutex ) ;
		G_ASSERT( dmem->magic == DataMemory::MAGIC ) ;
		buffer.resize( dmem->data_size ) ;
		::memcpy( &buffer[0] , dmem->data , buffer.size() ) ;
		if( dmem->data_size && type_p != nullptr )
			type_p->assign( dmem->type , sizeof(dmem->type) ) ; // inc. padding
		if( time_p != nullptr )
			time_p->s = dmem->time_s , time_p->us = dmem->time_us ;
		dmem->data_size = 0U ;
	}
}

void G::FatPipeReceiver::wait( int fd )
{
	fd_set read_fds ;
	FD_ZERO( &read_fds ) ;
	FD_SET( fd , &read_fds ) ;
	for(;;)
	{
		int rc = ::select( fd+1 , &read_fds , nullptr , nullptr , nullptr ) ;
		if( rc < 0 && errno == EINTR ) continue ;
		if( rc != 1 )
			throw Error( "cannot wait on the pipe" ) ;
		break ;
	}
}

// ==

G::FatPipe::Lock::Lock( Semaphore * s ) : 
	m_s(s) 
{ 
	s->decrement() ; 
}

G::FatPipe::Lock::~Lock() 
{ 
	m_s->increment() ; 
}

/// \file gfatpipe.cpp

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
/// \file gfatpipe.h
///

#ifndef G_FAT_PIPE__H
#define G_FAT_PIPE__H

#include "gdef.h"
#include "gsharedmemory.h"
#include "gsemaphore.h"
#include "gdatetime.h"
#include "gsignalsafe.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <utility>

namespace G
{
	class FatPipe ;
	class FatPipeReceiver ;
}

/// \class G::FatPipe
/// A one-way, unreliable-datagram communication channel from a parent process 
/// to a child process, using shared memory. Update events are sent from the
/// parent process via an inherited non-blocking file descriptor. The
/// shared memory file descriptor and the event file descriptor are typically
/// passed to the child process via its command-line.
/// \code
///
/// // writer...
/// FatPipe fat_pipe ;
/// if( fork() == 0 )
/// {
/// 	fat_pipe.doChild() ;
///		execl( "/bin/child" , "/bin/child" , 
/// 		fat_pipe.shmemfd() , fat_pipe.pipefd() , nullptr ) ;
/// 	_exit( 1 ) ;
/// }
/// fat_pipe.doParent()
/// fat_pipe.send( p , n ) ;
///
/// // reader...
/// FatPipe::Receiver rx( shmem_fd , pipe_fd ) ;
/// vector<char> buffer ;
/// while( FatPipe::wait(rx.fd()) )
/// 	rx.receive( buffer ) ;
/// \endcode
/// 
/// The implementation actuall uses two shared memory segments; a fixed size 
/// control segment containing a mutex, and a data segment that is resized as 
/// necessary. They are unlinked from the file system as soon as they are 
/// created, so they are almost never visible. 
/// 
/// A socketpair() pipe is used for the data-available event signalling.
/// 
/// The child process typically obtains the control segment's file descriptor 
/// from its command-line, whereas the data segment's file descriptor is 
/// passed over the event pipe.
/// 
class G::FatPipe
{
public:
	G_EXCEPTION( Error , "fat pipe error" ) ;
	typedef FatPipeReceiver Receiver ;

	FatPipe() ;
		///< Constructor.

	~FatPipe() ;
		///< Destructor.

	void doParent( bool auto_cleanup = true ) ;
		///< To be called from the parent process after fork().

	void doChild() ;
		///< To be called from the child process after fork().

	void send( const char * data , size_t size , const char * type = nullptr ) ;
		///< Sends a chunk of data to the child process. This will block in sendmsg() on
		///< first use or if the shared memory is grown in order to transfer the data fd.

	void send( const std::vector<char> & data , const char * type = nullptr ) ;
		///< Sends a chunk of data to the child process. This will block in sendmsg() on
		///< first use or if the shared memory is grown in order to transfer the data fd.

	void send( const std::vector<std::pair<const char *,size_t> > & data , const char * type = nullptr ) ;
		///< Sends a segmented data to the child process. This will block in sendmsg() on
		///< first use or if the shared memory is grown in order to transfer the data fd.

	void send( const std::vector<std::vector<char> > & data , const char * type = nullptr ) ;
		///< Sends chunked data to the child process. This will block in sendmsg() on
		///< first use or if the shared memory is grown in order to transfer the data fd.

	bool ping() ;
		///< Returns true if the receiver seems to be there.

	const char * shmemfd() const ;
		///< Returns the shared memory file descriptor as a string pointer (suitable 
		///< for exec()).

	const char * pipefd() const ;
		///< Returns the pipe file descriptor as a string pointer (suitable for exec()).

public:
	struct DataMemory /// Shared memory structure for G::FatPipe.
	{
		enum { MAGIC = 0xbeef } ;
		int magic ;
		char type[60] ;
		time_t time_s ;
		g_uint32_t time_us ;
		size_t data_size ;
		char data[1] ;
	} ;
	struct ControlMemory /// Shared memory structure for G::FatPipe.
	{
		enum { MAGIC = 0xdead } ; 
		int magic ;
		G::Semaphore::storage_type mutex ;
	} ;
	struct Lock /// RAII class to lock the G::FatPipe control segment.
	{
		explicit Lock( G::Semaphore * s ) ;
		~Lock() ;
		G::Semaphore * m_s ;
	} ;

private:
	FatPipe( const FatPipe & ) ;
	void operator=( const FatPipe & ) ;
	static size_t sensible( size_t ) ;
	static std::string name() ;
	static std::string name( size_t ) ;
	static std::string tostring( int ) ;
	static void close_( int ) ;
	static void cleanup( G::SignalSafe , const char * ) ;
	void send( size_t , size_t , const char ** , size_t * , const char * type ) ;

private:
	bool m_child ;
	bool m_parent ;
	G::SharedMemory m_shmem ;
	unique_ptr<G::SharedMemory> m_shmem_data ;
	int m_pipe_fds[2] ;
	int m_pipe_fd ;
	std::string m_arg_shmemfd ;
	std::string m_arg_pipefd ;
	int m_new_data_fd ;
	std::vector<const char*> m_data_p ;
	std::vector<size_t> m_data_n ;
} ;

/// \class G::FatPipeReceiver
/// A class to read a fat pipe in the child process.
/// 
class G::FatPipeReceiver
{
public:
	FatPipeReceiver( int shmem_fd , int pipe_fd ) ;
		///< Constructor.

	bool receive( std::vector<char> & buffer , std::string * type_p = nullptr , G::EpochTime * = nullptr ) ;
		///< Reads a message from the fat pipe's shared memory into the
		///< supplied buffer. 
		///< 
		///< This is used in the child process, with the file descriptors 
		///< inherited from the parent. The pipe fd must be non-blocking
		///< and it must have a pending read event (see wait()).
		///< 
		///< If the read event should be ignored then an empty buffer is 
		///< returned. Throws on error, but returns false if the pipe breaks. 

	int pipefd() const ;
		///< Returns the pipe fd.

	static void wait( int pipe_fd ) ;
		///< A convenience function that sets the pipe fd to be non-blocking
		///< and does a non-multiplexed wait for a read event.

private:
	typedef FatPipe::Error Error ;
	typedef FatPipe::DataMemory DataMemory ;
	typedef FatPipe::ControlMemory ControlMemory ;
	typedef FatPipe::Lock Lock ;
	FatPipeReceiver( const FatPipeReceiver & ) ;
	void operator=( const FatPipeReceiver & ) ;
	static void copy( SharedMemory & , SharedMemory & , std::vector<char> & , std::string * , G::EpochTime * ) ;
	struct Info { bool got_data ; bool got_eof ; int data_fd ; } ;
	static Info flush( int pipe_fd ) ;

private:
	int m_pipe_fd ;
	SharedMemory m_shmem ;
	unique_ptr<SharedMemory> m_shmem_data ;
} ;

#endif

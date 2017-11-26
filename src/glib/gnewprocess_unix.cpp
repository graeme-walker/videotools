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
// gnewprocess_unix.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gnewprocess.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gassert.h"
#include "gstr.h"
#include "glog.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> // open()
#include <unistd.h> // setuid() etc
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <iostream>
#include <signal.h> // kill()

/// \class G::Pipe
/// A private implementation class used by G::NewProcess that wraps
/// a unix pipe.
///
class G::Pipe
{
public:
	Pipe() ;
	~Pipe() ;
	void inChild() ; // writer
	void inParent() ; // reader
	int fd() const ;
	void dupTo( int stdxxx ) ; // onto stdout/stderr
	std::string read() ; // size-limited
	void write( const std::string & ) ;

private:
	int m_fds[2] ;
	int m_fd ;
} ;

/// \class G::NewProcessImp
/// A pimple-pattern implementation class used by G::NewProcess.
///
class G::NewProcessImp
{
public:
	NewProcessImp( const Path & exe , const StringArray & args , 
		int stdxxx , bool clean , bool strict_path ,
		Identity run_as_id , bool strict_id ,
		int exec_error_exit , const std::string & exec_error_format ,
		std::string (*exec_error_format_fn)(std::string,int) ) ;

	static std::pair<bool,pid_t> fork() ;
	std::string read() ;
	NewProcessWaitFuture & wait() ;
	int run( const G::Path & , const StringArray & , bool clean , bool strict ) ;
	void kill() ;
	static void printError( int , const std::string & s ) ;
	std::string execErrorFormat( const std::string & format , int errno_ ) ;

private:
	Pipe m_pipe ;
	NewProcessWaitFuture m_wait_future ;
	pid_t m_child_pid ;

private:
	NewProcessImp( const NewProcessImp & ) ;
	void operator=( const NewProcessImp & ) ;
} ;

// ==

G::NewProcess::NewProcess( const Path & exe , const StringArray & args , 
	int stdxxx , bool clean , bool strict_path ,
	Identity run_as_id , bool strict_id ,
	int exec_error_exit , const std::string & exec_error_format ,
	std::string (*exec_error_format_fn)(std::string,int) ) :
		m_imp(new NewProcessImp(exe,args,stdxxx,clean,strict_path,
			run_as_id,strict_id,exec_error_exit,exec_error_format,exec_error_format_fn) )
{
}

G::NewProcess::~NewProcess()
{
	delete m_imp ;
}

G::NewProcessWaitFuture & G::NewProcess::wait()
{
	return m_imp->wait() ;
}

std::string G::NewProcess::read()
{
	return m_imp->read() ;
}

std::pair<bool,pid_t> G::NewProcess::fork()
{
	return NewProcessImp::fork() ;
}

void G::NewProcess::kill()
{
	m_imp->kill() ;
}

// ==

G::NewProcessImp::NewProcessImp( const Path & exe , const StringArray & args , 
	int stdxxx , bool clean , bool strict_path ,
	Identity run_as_id , bool strict_id ,
	int exec_error_exit , const std::string & exec_error_format ,
	std::string (*exec_error_format_fn)(std::string,int) ) :
		m_wait_future(0) ,
		m_child_pid(-1)
{
	// impose sanity
	G_ASSERT( stdxxx == -1 || stdxxx == 1 || stdxxx == 2 ) ;
	if( stdxxx == 1 ) stdxxx = STDOUT_FILENO ;
	else if( stdxxx == 2 ) stdxxx = STDERR_FILENO ;

	// safety checks
	if( strict_path && exe.isRelative() ) 
		throw NewProcess::InvalidPath( exe.str() ) ;
	if( strict_id && run_as_id != Identity::invalid() &&
		( Identity::effective().isRoot() || run_as_id.isRoot() ) )
			throw NewProcess::Insecure() ;

	// fork
	std::pair<bool,pid_t> f = fork() ;
	bool in_child = f.first ;
	m_child_pid = f.second ;

	if( in_child )
	{
		try
		{
			// set real id
			if( run_as_id != Identity::invalid() )
				Process::beOrdinaryForExec( run_as_id ) ;

			// dup() so writing to stdxxx goes down the pipe
			m_pipe.inChild() ; 
			Process::closeFilesExcept( m_pipe.fd() ) ;
			m_pipe.dupTo( stdxxx ) ;

			// exec -- doesnt normally return from run()
			int e = run( exe , args , clean , strict_path ) ;

			// execve() failed -- write an error message to stdxxx
			if( exec_error_format_fn != 0 )
				printError( stdxxx , (*exec_error_format_fn)(exec_error_format,e) ) ;
			else if( !exec_error_format.empty() )
				printError( stdxxx , execErrorFormat(exec_error_format,e) ) ;
		}
		catch(...)
		{
		}
		::_exit( exec_error_exit ) ;
	}
	else
	{
		m_pipe.inParent() ;
		m_wait_future = NewProcessWaitFuture( m_child_pid ) ;
	}
}

std::pair<bool,pid_t> G::NewProcessImp::fork()
{
	std::cout << std::flush ;
	std::cerr << std::flush ;
	pid_t rc = ::fork() ;
	const bool ok = rc != -1 ;
	if( !ok ) throw NewProcess::CannotFork() ;
	bool in_child = rc == 0 ;
	pid_t child_pid = static_cast<pid_t>(rc) ;
	return std::make_pair( in_child , child_pid ) ;
}

void G::NewProcessImp::printError( int stdxxx , const std::string & s )
{
	// write an exec-failure message back down the pipe 
	G_IGNORE_RETURN( int , ::write( stdxxx , s.c_str() , s.length() ) ) ;
}

int G::NewProcessImp::run( const G::Path & exe , const StringArray & args , bool clean , bool strict )
{
	char * env[3U] ;
	std::string path( "PATH=/usr/bin:/bin" ) ; // no "."
	std::string ifs( "IFS= \t\n" ) ;
	env[0U] = const_cast<char*>( path.c_str() ) ;
	env[1U] = const_cast<char*>( ifs.c_str() ) ;
	env[2U] = nullptr ;

	char ** argv = new char* [ args.size() + 2U ] ;
	std::string str_exe = exe.str() ;
	argv[0U] = const_cast<char*>( str_exe.c_str() ) ;
	unsigned int argc = 1U ;
	for( StringArray::const_iterator arg_p = args.begin() ; arg_p != args.end() ; ++arg_p , argc++ )
		argv[argc] = const_cast<char*>(arg_p->c_str()) ;
	argv[argc] = nullptr ;

	if( clean && strict )
		::execve( exe.str().c_str() , argv , env ) ;
	else if( clean )
		::execvpe( exe.str().c_str() , argv , env ) ;
	else if( strict )
		::execv( exe.str().c_str() , argv ) ;
	else
		::execvp( exe.str().c_str() , argv ) ;
	int e = Process::errno_() ;

	delete [] argv ;
	G_DEBUG( "G::NewProcess::run: execve() returned: errno=" << e << ": " << exe ) ;
	return e ;
}

G::NewProcessWaitFuture & G::NewProcessImp::wait()
{
	return m_wait_future ;
}

std::string G::NewProcessImp::read()
{
	return m_pipe.read() ;
}

void G::NewProcessImp::kill()
{
	if( m_child_pid != -1 )
	{
		::kill( m_child_pid , SIGTERM ) ;
		m_child_pid = -1 ;
	}
}

std::string G::NewProcessImp::execErrorFormat( const std::string & format , int errno_ )
{
	std::string result = format ;
	G::Str::replaceAll( result , "__errno__" , G::Str::fromInt(errno_) ) ;
	G::Str::replaceAll( result , "__strerror__" , G::Process::strerror(errno_) ) ;
	return result ;
}

// ==

G::Pipe::Pipe() : 
	m_fd(-1) 
{ 
	m_fds[0] = m_fds[1] = -1 ;
	if( ::socketpair( AF_UNIX , SOCK_STREAM , 0 , m_fds ) < 0 ) // must be a stream to dup() onto stdout
		throw NewProcess::PipeError() ; 
	G_DEBUG( "G::Pipe::ctor: " << m_fds[0] << " " << m_fds[1] ) ;
}

G::Pipe::~Pipe()
{
	if( m_fds[0] >= 0 ) ::close( m_fds[0] ) ;
	if( m_fds[1] >= 0 ) ::close( m_fds[1] ) ;
}

void G::Pipe::inChild() 
{ 
	::close( m_fds[0] ) ; // close read end
	m_fds[0] = -1 ;
	m_fd = m_fds[1] ; // writer
}

void G::Pipe::inParent() 
{ 
	::close( m_fds[1] ) ; // close write end
	m_fds[1] = -1 ;
	m_fd = m_fds[0] ; // reader
}

int G::Pipe::fd() const
{ 
	return m_fd ; 
}

void G::Pipe::dupTo( int stdxxx )
{ 
	if( m_fd != -1 && stdxxx != -1 && m_fd != stdxxx )
	{
		if( ::dup2(m_fd,stdxxx) != stdxxx )
			throw NewProcess::PipeError() ;
		::close( m_fd ) ;
		m_fd = -1 ;
		m_fds[1] = -1 ;

		int flags = ::fcntl( stdxxx , F_GETFD ) ;
		flags &= ~FD_CLOEXEC ;
		::fcntl( stdxxx , F_SETFD , flags ) ; // no close on exec
	}
}

std::string G::Pipe::read()
{
	// (this is called from the parent process, so no dup()ing shenanigans)
	char buffer[limits::pipe_buffer] ;
	ssize_t rc = m_fd == -1 ? 0 : ::read( m_fd , buffer , sizeof(buffer) ) ;
	if( rc < 0 ) throw NewProcess::PipeError("read") ;
    const size_t buffer_size = static_cast<size_t>(rc) ;
    return std::string(buffer,buffer_size) ;
}

// ==

G::NewProcessWaitFuture::NewProcessWaitFuture() :
	m_hprocess(0) ,
	m_pid(0) ,
	m_rc(0) ,
	m_status(0) ,
	m_error(0)
{
}

G::NewProcessWaitFuture::NewProcessWaitFuture( pid_t pid ) :
	m_hprocess(0) ,
	m_pid(pid) ,
	m_rc(0) ,
	m_status(0) ,
	m_error(0)
{
}

G::NewProcessWaitFuture & G::NewProcessWaitFuture::run()
{
	// (worker thread - keep it simple)
	while( m_pid != 0 )
	{
		errno = 0 ;
		m_rc = ::waitpid( m_pid , &m_status , 0 ) ;
		m_error = errno ;
		if( m_rc == -1 && m_error == EINTR )
			; // signal in parent -- keep waiting
		else 
			break ;
	}
	return *this ;
}

int G::NewProcessWaitFuture::get()
{
	if( m_pid != 0 )
	{
		if( m_error )
		{
			std::ostringstream ss ;
			ss << "errno=" << m_error ;
			throw NewProcess::WaitError( ss.str() ) ;
		}
		if( ! WIFEXITED(m_status) )
		{
			// uncaught signal or stopped
			std::ostringstream ss ;
			ss << "status=" << m_status ;
			throw NewProcess::ChildError( ss.str() ) ;
		}
		return WEXITSTATUS(m_status) ;
	}
	else
	{
		return 0 ;
	}
}


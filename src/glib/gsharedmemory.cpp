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
// gsharedmemory.cpp
//

#include "gdef.h"
#include "gsharedmemory.h"
#include "gprocess.h"
#include "groot.h"
#include "gstr.h"
#include "gdirectory.h"
#include "gassert.h"
#include <algorithm>
#include <vector>
#include <sys/mman.h> // shm_open()
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

namespace
{
	static std::string s_prefix ; // prefix for o/s-name, eg. "foo-" -- set by prefix(...)
	static std::string s_help ; // eg. "try deleting %S" -- set by help(...)
	static std::string s_filetext ; // eg. "# this is a shared memory name placeholder file" -- set by filetext(...)
}

namespace
{
	std::string get_help( const std::string & name , const std::string & os_name )
	{
		static const char * help_linux = ": try deleting /dev/shm/%S*" ;
		static const char * help_bsd = ": try this (ayor)... "
			"perl -e 'syscall($ARGV[0],$ARGV[1])' `awk '/shm_unlink/{print $NF}' /usr/include/sys/syscall.h` /%S" ;

		std::string help = s_help ;
		if( help.empty() )
			help = std::string( G::is_linux() ? help_linux : help_bsd )  ;

		G::Str::replaceAll( help , "%s" , name ) ;
		G::Str::replaceAll( help , "%S" , os_name ) ;
		return help ;
	}
}

G::SharedMemory::Imp::Imp() :
	m_fd(-1) ,
	m_p(nullptr) ,
	m_size(0U)
{
}

G::SharedMemory::Imp::~Imp()
{
	if( m_p != nullptr )
		::munmap( m_p , m_size ) ;

	if( m_fd != -1 )
		::close( m_fd ) ;

	unlink() ;
}

void G::SharedMemory::Imp::swap( Imp & other )
{
	using std::swap ;
	swap( m_fd , other.m_fd ) ;
	swap( m_unlink_name , other.m_unlink_name ) ;
	swap( m_p , other.m_p ) ;
	swap( m_size , other.m_size ) ;
}

int G::SharedMemory::Imp::fd() const
{
	return m_fd ;
}

void G::SharedMemory::Imp::unlink()
{
	if( !m_unlink_name.empty() )
	{
		G::Root claim_root ;
		::shm_unlink( m_unlink_name.c_str() ) ;
		m_unlink_name.clear() ;
	}
}

// ==

G::SharedMemory::SharedMemory( size_t size )
{
	G_ASSERT( size != 0U ) ;
	init( std::string() , -1 , size , O_RDWR , false ) ;
}

G::SharedMemory::SharedMemory( int fd )
{
	G_ASSERT( fd >= 0 ) ;
	init( std::string() , fd , 0U , O_RDWR , false ) ;
}

G::SharedMemory::SharedMemory( const std::string & name )
{
	G_ASSERT( !name.empty() ) ;
	init( name , -1 , 0U , O_RDWR , false ) ;
}

G::SharedMemory::SharedMemory( const std::string & name , size_t size )
{
	G_ASSERT( !name.empty() ) ;
	G_ASSERT( size != 0U ) ;
	bool exclusive = true ;
	init( name , -1 , size , O_RDWR | O_CREAT | ( exclusive ? O_EXCL : O_TRUNC ) , false ) ;
}

G::SharedMemory::SharedMemory( size_t size , Control )
{
	G_ASSERT( size != 0U ) ;
	init( std::string() , -1 , size , O_RDWR , true ) ;
}

G::SharedMemory::SharedMemory( int fd , Control )
{
	G_ASSERT( fd >= 0 ) ;
	init( std::string() , fd , 0U , O_RDWR , true ) ;
}

G::SharedMemory::SharedMemory( const std::string & name , Control )
{
	G_ASSERT( !name.empty() ) ;
	init( name , -1 , 0U , O_RDWR , true ) ;
}

G::SharedMemory::SharedMemory( const std::string & name , size_t size , Control )
{
	G_ASSERT( !name.empty() ) ;
	G_ASSERT( size != 0U ) ;
	bool exclusive = true ;
	init( name , -1 , size , O_RDWR | O_CREAT | ( exclusive ? O_EXCL : O_TRUNC ) , true ) ;
}

G::SharedMemory::~SharedMemory()
{
}

void G::SharedMemory::init( const std::string & name_in , int fd , size_t size , int open_flags , bool control_segment )
{
	// see "man shm_overview" and "man sem_overview"

	int mmap_flags = 0 ;
	std::string os_name = osName( name_in ) ;

	if( os_name.empty() && fd == -1 ) // new anonymous segment
	{
		G_ASSERT( size != 0U ) ;
		G::Root claim_root ;
		Imp imp ;
		imp.m_size = size ;
		imp.m_p = ::mmap( nullptr , imp.m_size , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS | mmap_flags , -1 , 0 ) ;
		if( imp.m_p == nullptr )
			throw Error( "mmap error" ) ;
		m_imp.swap( imp ) ;
	}
	else
	{
		Imp imp ;
		if( os_name.empty() ) // inherited anonymous segment
		{
			imp.m_fd = fd ;
		}
		else // named segment, new or pre-existing
		{
			int e = 0 ;
			{
				G::Root claim_root ;
				imp.m_fd = ::shm_open( os_name.c_str() , open_flags , 0777 ) ;
				e = errno ;
			}
			if( imp.fd() == -1 )
			{
				std::stringstream ss ;
				ss << "shm_open() failed for [" << os_name << "]" ;
				ss << ": " << G::Process::strerror(e) ;
				if( (open_flags & O_CREAT) && e == EEXIST )
					ss << get_help(name_in,os_name) ;
				throw Error( ss.str() ) ;
			}
			if( open_flags & O_CREAT )
			{
				imp.m_unlink_name = os_name ;
				if( control_segment )
					createMarker( os_name ) ;
			}
		}

		if( size == 0U ) // pre-existing named segment - determine its size from the file system
		{
			G::Root claim_root ;
			struct ::stat statbuf ;
			if( 0 != fstat( imp.fd() , &statbuf ) )
				throw Error( os_name , "fstat error" ) ;
			if( statbuf.st_size == 0 )
				throw Error( os_name , "empty shared memory file" ) ; // never gets here?
			imp.m_size = statbuf.st_size ;
		}
		else // new named segment - set its size
		{
			G::Root claim_root ;
			if( 0 != ::ftruncate( imp.fd() , size ) ) 
				throw Error( os_name , "ftruncate error" ) ;
			imp.m_size = size ;
		}

		{
			G::Root claim_root ;
			imp.m_p = ::mmap( nullptr , imp.m_size , PROT_READ | PROT_WRITE , MAP_SHARED | mmap_flags , imp.m_fd , 0 ) ;
			if( imp.m_p == nullptr )
				throw Error( os_name , "mmap error" ) ;
		}

		m_imp.swap( imp ) ;
	}
}

bool G::SharedMemory::remap( size_t new_size , bool may_move , bool no_throw )
{
	G::Root claim_root ;
	if( m_imp.m_fd != -1 && 0 != ::ftruncate( m_imp.fd() , new_size ) )
		throw Error( "ftruncate error" ) ;

	// mremap is linux only - stubbed out in gdef.h otherwise
	void * new_p = ::mremap( m_imp.m_p , m_imp.m_size , new_size , may_move ? MREMAP_MAYMOVE : 0 ) ;
	if( new_p == MAP_FAILED ) // -1, not nullptr
	{
		if( !no_throw )
			throw Error( "cannot resize the shared memory segment" ) ;
		return false ;
	}
	m_imp.m_p = new_p ;
	m_imp.m_size = new_size ;
	return true ;
}

std::string G::SharedMemory::osName( const std::string & name_in )
{
	if( name_in.empty() )
		return std::string() ;
	else if( name_in.at(0U) != '/' && !G::is_linux() )
		return "/" + s_prefix + name_in ;
	else
		return s_prefix + name_in ;
}

void G::SharedMemory::cleanup( SignalSafe signal_safe , const char * os_name , void (*fn)(SignalSafe,void*) )
{
	G::Identity identity = G::Root::start( signal_safe ) ;
	int fd = ::shm_open( os_name , O_RDWR , 0 ) ;
	::shm_unlink( os_name ) ;
	if( fd >= 0 )
	{
		struct ::stat statbuf ;
		if( fstat(fd,&statbuf) == 0 && statbuf.st_size > 0 )
		{
			void * p = ::mmap( nullptr , statbuf.st_size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 ) ;
			if( fn && p ) fn( signal_safe , p ) ;
			::munmap( p , statbuf.st_size ) ;
		}
		::close( fd ) ;
	}
	G::Root::stop( signal_safe , identity ) ;
	deleteMarker( signal_safe , os_name ) ;
}

void G::SharedMemory::trap( SignalSafe signal_safe , const char * os_name , void (*fn)(SignalSafe,void*) )
{
	G::Identity identity = G::Root::start( signal_safe ) ;
	int fd = ::shm_open( os_name , O_RDWR , 0 ) ;
	if( fd >= 0 )
	{
		struct ::stat statbuf ;
		if( fstat(fd,&statbuf) == 0 && statbuf.st_size > 0 )
		{
			void * p = ::mmap( nullptr , statbuf.st_size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 ) ;
			if( fn && p ) fn( signal_safe , p ) ;
		}
		::close( fd ) ;
	}
	G::Root::stop( signal_safe , identity ) ;
	deleteMarker( signal_safe , os_name ) ;
}

void G::SharedMemory::inherit()
{
	::fcntl( m_imp.fd() , F_SETFD , ::fcntl(m_imp.fd(),F_GETFD) & ~FD_CLOEXEC ) ; // no close on exec
}

void * G::SharedMemory::ptr() const
{
	return m_imp.m_p ;
}

int G::SharedMemory::fd() const
{
	return m_imp.m_fd ;
}

size_t G::SharedMemory::size() const
{
	return m_imp.m_size ;
}

void G::SharedMemory::unlink()
{
	m_imp.unlink() ;
}

void G::SharedMemory::createMarker( const std::string & os_name )
{
	std::string path = "/tmp/" + os_name + ".x" ;
	int fd = -1 ;
	{
		G::Root claim_root ;
		G_IGNORE_RETURN( int , ::unlink( path.c_str() ) ) ; // delete so that ownership gets updated
		fd = ::open( path.c_str() , O_WRONLY | O_CREAT , 0666 ) ;
	}
	if( fd != -1 )
	{
		if( !s_filetext.empty() )
		{
			G_IGNORE_RETURN( int , ::write( fd , s_filetext.data() , s_filetext.length() ) ) ;
		}
		::close( fd ) ;
	}
}

void G::SharedMemory::deleteMarker( const std::string & os_name )
{
	std::string path = "/tmp/" + os_name + ".x" ;
	{
		G::Root claim_root ;
		G_IGNORE_RETURN( int , ::unlink( path.c_str() ) ) ;
	}
}

void G::SharedMemory::deleteMarker( SignalSafe signal_safe , const char * os_name )
{
	if( os_name )
	{
		std::string path = std::string("/tmp/") + os_name + ".x" ;
		{
			G::Identity identity = G::Root::start( signal_safe ) ;
			G_IGNORE_RETURN( int , ::unlink( path.c_str() ) ) ;
			G::Root::stop( signal_safe , identity ) ;
		}
	}
}

std::vector<std::string> G::SharedMemory::list( bool os_names )
{
	std::vector<std::string> result ;
	listImp( result , "/tmp" , "x" , os_names ) ; // createMarker()
	listImp( result , "/run/shm" , "" , os_names ) ; // linux
	std::sort( result.begin() , result.end() ) ;
	result.erase( std::unique(result.begin(),result.end()) , result.end() ) ;
	return result ;
}

void G::SharedMemory::listImp( std::vector<std::string> & out , const std::string & dir , const std::string & x , bool os_names )
{
	for( G::DirectoryIterator p( (G::Directory(dir)) ) ; p.more() ; )
	{
		if( !p.isDir() && p.filePath().extension() == x &&
			( prefix().empty() || p.fileName().find(prefix()) == 0U ) )
		{
			std::string basename = p.filePath().withoutExtension().basename() ;
			std::string sname = prefix().empty() ? basename : basename.substr(prefix().length()) ;
			out.push_back( os_names ? osName(sname) : sname ) ;
		}
	}
}

std::string G::SharedMemory::delete_( const std::string & name , bool control )
{
	int e , rc = 0 ;
	{
		G::Root claim_root ;
		rc = ::shm_unlink( osName(name).c_str() ) ;
		e = errno ;
	}
	if( rc == 0 || e == ENOENT )
	{
		if( control )
			deleteMarker( osName(name) ) ;
		return std::string() ;
	}
	else
	{
		return G::Process::strerror(e) ;
	}
}

void G::SharedMemory::help( const std::string & s )
{
	s_help = s ;
}

void G::SharedMemory::prefix( const std::string & s )
{
	G_ASSERT( s.find('/') == std::string::npos ) ;
	if( s.find("__") != 0U )
		s_prefix = s ;
}

std::string G::SharedMemory::prefix()
{
	return s_prefix ;
}

void G::SharedMemory::filetext( const std::string & s )
{
	s_filetext = s ;
}

/// \file gsharedmemory.cpp

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
// gvcache.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "gfile.h"
#include "groot.h"
#include "gprocess.h"
#include "gdatetime.h"
#include "gvcache.h"
#include "gassert.h"
#include "glog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // ::open()
#include <stdio.h> // ::rename()
#include <errno.h>
#include <unistd.h>

Gv::Cache::Cache( const G::Path & base_dir , const std::string & name , size_t size ) :
	m_base_dir(base_dir) ,
	m_prefix(name.empty()?std::string():(name+".")) ,
	m_cache_dir(base_dir+".cache") ,
	m_list(size) ,
	m_p(m_list.begin()) ,
	m_ok(true)
{
	G_ASSERT( name.find('/') == std::string::npos ) ;
	G::Str::replaceAll( m_prefix , "/" , "" ) ;

	G_LOG( "Gv::Cache::ctor: cache dir=[" << m_cache_dir << "] size=" << m_list.size() ) ;
	if( m_list.size() )
	{
		G::Root claim_root ;
		G::File::mkdirs( m_cache_dir ) ;
	}

	unsigned int i = 0U ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p , i++ )
		(*p).cache_path = G::Path(m_cache_dir+(m_prefix+G::Str::fromUInt(i))).str() ;
}

Gv::Cache::~Cache()
{
	G::Root claim_root ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( (*p).fd != -1 )
			::close( (*p).fd ) ;
		::unlink( (*p).cache_path.c_str() ) ;
	}
}

std::string Gv::Cache::base() const
{
	return m_base_dir.str() ;
}

void Gv::Cache::fail( const char * where )
{
	G_DEBUG( "Gv::Cache::fail: failed: " << where ) ;
	G_WARNING_ONCE( "Gv::Cache::fail: one or more cache failures" ) ;
	m_ok = false ;
}

void Gv::Cache::store( const Gr::ImageBuffer & image_buffer , const std::string & commit_path , 
	const std::string & commit_path_other , const std::string & same_as_path )
{
	if( m_list.empty() ) return ;
	if( ++m_p == m_list.end() )
		m_p = m_list.begin() ;

	Entry & e = *m_p ;
	e.commit_path = commit_path ;
	e.commit_path_other = commit_path_other ;
	e.same_as_path = same_as_path ;
	open( e ) ;
	write( e , image_buffer ) ;
}

void Gv::Cache::open( Entry & e )
{
	if( e.fd == -1 ) 
	{
		{
			G::Root claim_root ;
			e.fd = ::open( e.cache_path.c_str() , O_CREAT | O_TRUNC | O_WRONLY , 
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ) ;
		}
		if( e.fd == -1 ) 
			fail( "open" ) ;
	}
}

void Gv::Cache::write( Entry & e , const Gr::ImageBuffer & image_buffer )
{
	if( ::lseek( e.fd , 0L , SEEK_SET ) < 0 ) 
		fail( "lseek" ) ;

	typedef Gr::traits::imagebuffer<Gr::ImageBuffer>::const_row_iterator row_iterator ;
	for( row_iterator part_p = Gr::imagebuffer::row_begin(image_buffer) ; part_p != Gr::imagebuffer::row_end(image_buffer) ; ++part_p )
	{
		const char * p = Gr::imagebuffer::row_ptr( part_p ) ;
		size_t n = Gr::imagebuffer::row_size( part_p ) ;
		if( n != 0U && ::write( e.fd , p , n ) != static_cast<ssize_t>(n) ) 
		{
			fail( "write" ) ;
			break ;
		}
	}
}

void Gv::Cache::commit( bool other )
{
	size_t n = 0U ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( (*p).fd >= 0 )
		{
			commit( *p , other ) ;
			n++ ;
		}
	}
	G_DEBUG( "Gv::Cache::commit: commited " << n << "/" << m_list.size() ) ;
}

void Gv::Cache::commit( Entry & e , bool other )
{
	close( e ) ;
	const std::string & commit_path = other && !e.commit_path_other.empty() ? e.commit_path_other : e.commit_path ;
	if( !e.same_as_path.empty() && move(e.same_as_path,commit_path) ) // avoid duplication
	{
		G::Root claim_root ;
		G::File::remove( e.cache_path , G::File::NoThrow() ) ;
	}
	else
	{
		move( e.cache_path , commit_path ) ;
	}
}

void Gv::Cache::close( Entry & e )
{
	int fd = e.fd ;
	e.fd = -1 ;
	if( ::close( fd ) < 0 )
		fail( "close" ) ;
}

bool Gv::Cache::move( const std::string & src , const std::string & dst )
{
	int rc = -1 ;
	{
		G::Root claim_root ;
		rc = ::rename( src.c_str() , dst.c_str() ) ;
	}
	if( rc < 0 )
	{
		int err = errno ;
		if( err == ENOENT )
		{
			mkdirs( dst ) ;
			G::Root claim_root ;
			rc = ::rename( src.c_str() , dst.c_str() ) ;
		}
	}
	if( rc < 0 )
		fail( "rename" ) ;
	else
		G_DEBUG( "Gv::Cache::move: [" << src << "] -> [" << dst << "]" ) ;
	return rc >= 0 ;
}

void Gv::Cache::mkdirs( const G::Path & path )
{
	bool ok = false ;
	{
		G::Path dir = path.dirname() ;
		G::Root claim_root ;
		ok = G::File::mkdirs( dir , G::File::NoThrow() ) ;
	}
	if( !ok )
		fail( "mkdirs" ) ;
}

/// \file gvcache.cpp

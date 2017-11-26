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
/// \file gvcache.h
///

#ifndef GV_CACHE__H
#define GV_CACHE__H

#include "gdef.h"
#include "grimagebuffer.h"
#include "gexception.h"
#include "gpath.h"
#include <vector>
#include <string>

namespace Gv
{
	class Cache ;
}

/// \class Gv::Cache
/// A queue of recent images that can be flushed to the filesystem on demand.
/// 
/// The implementation keeps a large set of cache files open and re-writes them 
/// on a least-recently used basis. When a commit is requested the files are 
/// closed and moved into their proper place (see ImageOutput). By keeping the 
/// files open the images live in the kernel's buffer cache and do not need to 
/// hit the filesystem at all.
/// 
class Gv::Cache
{
public:
	G_EXCEPTION( Error , "cache error" ) ;

	explicit Cache( const G::Path & base_dir , const std::string & name , size_t size ) ;
		///< Constructor. Paths are like "<base-dir>/.cache/<name>.123".

	~Cache() ;
		///< Destructor.

	void store( const Gr::ImageBuffer & , const std::string & commit_path , 
		const std::string & commit_path_other = std::string() ,
		const std::string & same_as = std::string() ) ;
			///< Stores an image in the cache.
			///< 
			///< An alternate path for the committed file can be supplied,
			///< with the choice dictated by a parameter passed to commit().
			///< 
			///< The "same-as" path indicates where the image has already 
			///< been saved to the filesystem, so a commit() just moves
			///< the already-saved file into place and discards the
			///< cached version. (In practice this avoids duplicates
			///< when recording in slow mode and then switching to fast
			///< mode with an associated flush of the cache.)

	void commit( bool other = false ) ;
		///< Commits all cached images to their non-cache location.

	std::string base() const ;
		///< Returns the base directory, as passed to the constructor.

private:
	struct Entry 
	{ 
		Entry( int fd_ = -1 , const std::string & cache_path_ = std::string() , const std::string & commit_path_ = std::string() ) : 
			fd(fd_) , 
			cache_path(cache_path_) , 
			commit_path(commit_path_)
		{
		}
		int fd ; 
		std::string cache_path ; 
		std::string commit_path ; 
		std::string commit_path_other ; 
		std::string same_as_path ;
	} ;

private:
	Cache( const Cache & ) ;
	void operator=( const Cache & ) ;
	std::string commitPath( const std::string & , G::EpochTime ) const ;
	void fail( const char * where ) ;
	void open( Entry & e ) ;
	void write( Entry & e , const Gr::ImageBuffer & ) ;
	void commit( Entry & e , bool ) ;
	void close( Entry & e ) ;
	bool move( const std::string & src , const std::string & dst ) ;
	void mkdirs( const G::Path & path ) ;

private:
	typedef std::vector<Entry> List ;
	G::Path m_base_dir ;
	std::string m_prefix ;
	G::Path m_cache_dir ;
	List m_list ;
	List::iterator m_p ;
	bool m_ok ;
} ;

#endif

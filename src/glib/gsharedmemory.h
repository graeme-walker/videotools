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
/// \file gsharedmemory.h
///

#ifndef G_SHARED_MEMORY__H
#define G_SHARED_MEMORY__H

#include "gdef.h"
#include "gsignalsafe.h"
#include "gexception.h"
#include <string>
#include <vector>

namespace G
{
	class SharedMemory ;
}

/// \class G::SharedMemory
/// A POSIX shared memory class. Anonymous shared memory can be inherited across 
/// a fork(); shared memory associated with an open file descriptor can be 
/// inherited across a fork() and exec(); and shared memory associated with 
/// a filesystem path can be shared freely by name.
/// 
class G::SharedMemory
{
public:
	G_EXCEPTION( Error , "shared memory error" ) ;
	struct Control /// Overload discriminator for G::SharedMemory
		{} ;

	explicit SharedMemory( size_t size ) ;
		///< Constructor for a new anonymous segment, typically used across
		///< a fork().

	explicit SharedMemory( int fd ) ;
		///< Constructor for a segment inherited via a file descriptor, 
		///< typically across fork()/exec(). The file is closed by the 
		///< destructor. See also: inherit(), fd()

	explicit SharedMemory( const std::string & name ) ;
		///< Constructor for an existing named segment. Throws if 
		///< it does not exist.

	SharedMemory( const std::string & name , size_t size ) ;
		///< Constructor for a new named segment. Throws if it already
		///< exists. The destructor will call unlink().

	SharedMemory( size_t size , Control ) ;
		///< Constructor overload for control segments that might
		///< contain semaphores.

	SharedMemory( int fd , Control ) ;
		///< Constructor overload for control segments that might
		///< contain semaphores.

	SharedMemory( const std::string & name , Control ) ;
		///< Constructor overload for control segments that might
		///< contain semaphores.

	SharedMemory( const std::string & name , size_t size , Control ) ;
		///< Constructor overload for control segments that might
		///< contain semaphores.

	~SharedMemory() ;
		///< Destructor. The memory is unmapped and, where relevant, 
		///< the newly-created named segment is unlink()ed.

	size_t size() const ;
		///< Returns the segment size. This is affected by remap().

	void unlink() ;
		///< Unlinks the segment from the filesystem. Typically 
		///< used for early cleanup when doing fork()/exec(). 
		///< Does not throw.

	static std::string delete_( const std::string & name , bool control_segment ) ;
		///< Unlinks the segment from the filesystem. Typically
		///< used administratively, especially on BSD. Returns a
		///< failure reason, or the empty string on success.

	static void cleanup( SignalSafe , const char * os_name , void (*fn)(SignalSafe,void*) ) ;
		///< An exit handler that unlinks the named shared memory segment 
		///< and calls the user's function for the memory contents. Use 
		///< osName() for the segment name.

	static void trap( SignalSafe , const char * os_name , void (*fn)(SignalSafe,void*) ) ;
		///< An exit handler that calls the user's function on the shared
		///< memory contents. Use osName() for the segment name.

	static std::string osName( const std::string & name ) ;
		///< Converts the segment name to an internal form that can be
		///< passed to cleanup() or trap().

	int fd() const ;
		///< Returns the file descriptor. Returns -1 for anonymous segments.

	void inherit() ;
		///< Marks fd() as inherited across exec() ie. no-close-on-exec.

	void * ptr() const ;
		///< Returns the mapped address.

	bool remap( size_t , bool may_move , bool no_throw = false ) ;
		///< Resizes and remaps the shared memory. Addresses may change if 'may_move' 
		///< is true, but when false resizing will be less likely to succeed. 
		///< Throws or returns false on error depending on 'no_throw'.
		///< Do not use this if there are semaphores inside the shared 
		///< memory.

	static void help( const std::string & ) ;
		///< Sets some error-message help text for the case when named shared memory
		///< cannot be created because it already exists.

	static void prefix( const std::string & ) ;
		///< Sets a prefix for shared memory names, used by osName(). This method
		///< has to be called early and consistently (if called at all) in all 
		///< cooperating programs. Does nothing if the prefix starts with a 
		///< double-underscore.

	static std::string prefix() ;
		///< Returns the prefix, as set by by a call to the other overload.

	static void filetext( const std::string & ) ;
		///< Sets some help text that gets written into the placeholder files.

	static std::vector<std::string> list( bool os_names = false ) ;
		///< Returns a list of possible control segment names.

private:
	struct Imp 
	{ 
		Imp() ;
		~Imp() ;
		int fd() const ;
		void swap( Imp & ) ;
		void unlink() ;
		//
		std::string m_unlink_name ;
		int m_fd ;
		void * m_p ;
		size_t m_size ;
		//
		private: Imp( const Imp & ) ;
		private: void operator=( const Imp & ) ;
	} ;

private:
	SharedMemory( const SharedMemory & ) ;
	void operator=( const SharedMemory & ) ;
	void init( const std::string & , int , size_t , int , bool ) ;
	static void createMarker( const std::string & ) ;
	static void deleteMarker( const std::string & ) ;
	static void deleteMarker( SignalSafe , const char * ) ;
	static void listImp( std::vector<std::string> & , const std::string & , const std::string & , bool ) ;

private:
	Imp m_imp ;
} ;

#endif

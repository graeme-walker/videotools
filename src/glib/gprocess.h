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
/// \file gprocess.h
///

#ifndef G_PROCESS_H
#define G_PROCESS_H

#include "gdef.h"
#include "gexception.h"
#include "gidentity.h"
#include "gpath.h"
#include "gstrings.h"
#include "gsignalsafe.h"
#include <iostream>
#include <sys/types.h>
#include <string>

namespace G
{
	class Process ;
	class NewProcess ;
}

/// \class G::Process
/// A static interface for doing things with processes.
/// \see G::Identity
/// 
class G::Process : private G::IdentityUser
{
public:
	G_EXCEPTION( CannotChangeDirectory , "cannot cd()" ) ;
	G_EXCEPTION( InvalidId , "invalid process-id string" ) ;

	class IdImp ;
	class Id /// Process-id class.
	{
		public: Id() ;
		public: explicit Id( std::istream & ) ;
		public: Id( SignalSafe , const char * pid_file_path ) ; // (ctor for signal-handler)
		public: std::string str() const ;
		public: bool operator==( const Id & ) const ;
		private: pid_t m_pid ;
		friend class NewProcess ;
		friend class Process ;
	} ;
	class Umask /// Used to temporarily modify the process umask.
	{
		public: enum Mode { Readable , Tighter , Tightest , GroupOpen } ;
		public: explicit Umask( Mode ) ;
		public: ~Umask() ;
		public: static void set( Mode ) ;
		public: static void tighten() ; // no "other" access, user and group unchanged
		private: Umask( const Umask & ) ; // not implemented
		private: void operator=( const Umask & ) ; // not implemented
		private: class UmaskImp ;
		private: UmaskImp * m_imp ;
	} ;
	class NoThrow /// An overload discriminator for Process.
		{} ;

	static void closeFiles( bool keep_stderr = false ) ;
		///< Closes all open file descriptors, but optionally not stderr.
		///< Stdin, stdout, and possibly stderr are reopened to the 
		///< null device.

	static void closeFilesExcept( int fd_1 , int fd_2 = -1 ) ;
		///< Closes all open file descriptors except the given ones.
		///< If stdin, stdout, or stderr are closed then they are
		///< reopened to the null device.

	static void closeStderr() ;
		///< Closes stderr and reopens it to the null device.

	static void cd( const Path & dir ) ;
		///< Changes directory.

	static bool cd( const Path & dir , NoThrow ) ;
		///< Changes directory. Returns false on error.

	static int errno_( const SignalSafe & = G::SignalSafe() ) ;
		///< Returns the process's current 'errno' value.

	static int errno_( const SignalSafe & , int e_new ) ;
		///< Sets the process's 'errno' value. Returns the old
		///< value. Used in signal handlers.

	static std::string strerror( int errno_ ) ;
		///< Translates an 'errno' value into a meaningful diagnostic string.
		///< The returned string is non-empty, even for a zero errno.

	static void revokeExtraGroups() ;
		///< Revokes secondary group memberships if really root
		///< or if suid.

	static Identity beOrdinary( Identity ordinary_id , bool change_group = true ) ;
		///< Revokes special privileges (root or suid).
		///< 
		///< If the real-id is root then the effective id is changed to whatever 
		///< is passed in. Otherwise the effective id is changed to the real id, 
		///< and the parameter is ignored. 
		///< 
		///< Returns the old effective-id, which can be passed to beSpecial().
		///< 
		///< See also class G::Root.

	static Identity beSpecial( Identity special_id , bool change_group = true ) ;
		///< Re-acquires special privileges (either root or suid). The 
		///< parameter must have come from a previous call to beOrdinary().
		///< 
		///< Returns the old effective-id (which is normally ignored).
		///< 
		///< See also class G::Root.

	static Identity beOrdinary( SignalSafe , Identity ordinary_id , bool change_group = true ) ;
		///< A signal-safe overload.

	static Identity beSpecial( SignalSafe , Identity special_id , bool change_group = true ) ;
		///< A signal-safe overload.

	static void beOrdinaryForExec( Identity run_as_id ) ;
		///< Sets the real and effective user and group ids to those
		///< given. Errors are ignored. This should only be used in
		///< a forked child process prior to doing an exec().

	static std::string cwd( bool no_throw = false ) ;
		///< Returns the current working directory. Throws on error
		///< by default or returns the empty string.

	static std::string exe() ;
		///< Returns the absolute path of the current executable, 
		///< independent of the argv array passed to main(). Returns 
		///< the empty string if unknown.

private:
	Process() ;
} ;

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const G::Process::Id & id )
	{
		return stream << id.str() ;
	}

	inline
	std::istream & operator>>( std::istream & stream , G::Process::Id & id )
	{
		id = G::Process::Id( stream ) ;
		return stream ;
	}
}

#endif

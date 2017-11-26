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
/// \file gsemaphore.h
///

#ifndef G_SEMAPHORE__H
#define G_SEMAPHORE__H

#include "gdef.h"
#include "gexception.h"

namespace G
{
	class Semaphore ;
	class SemaphoreImp ;
}

/// \class G::Semaphore
/// A semaphore class with a posix or sysv implementation chosen at build-time.
/// Eg.
/// \code
/// struct Memory { Semaphore::storage_type sem_storage ; int value ; } ;
/// Memory * mem = static_cast<Memory*>( mem_p ) ;
/// Semaphore * s = new (&mem->sem_storage) Semaphore ; // creator - placement new
/// Semaphore * s = Semaphore::at(&mem->sem_storage) ; // user
/// s->decrement() ; // lock - may block
/// mem->value = 999 ;
/// s->increment() ; // unlock
/// s->~Semaphore() ; // creator - placement delete
/// \endcode
/// 
class G::Semaphore
{
public:
	G_EXCEPTION( Error , "semaphore error" ) ;
	struct storage_type /// semaphore storage
		{ unsigned long filler[4] ; } ; // adjust the size wrt. sem_t as necessary

	explicit Semaphore( unsigned int initial_value = 1U ) ;
		///< Constructor for a new anonymous semaphore, typically located
		///< inside a shared memory segment using "placement new".
		///< The initial value should be one for a mutex.

	~Semaphore() ;
		///< Destroys the semaphore. Results are undefined if there
		///< are waiters.

	static Semaphore * at( storage_type * ) ;
		///< Syntactic sugar to return an object pointer corresponding 
		///< to the given storage pointer.

	void increment() ;
		///< Increment operator. Used for mutex unlocking.

	void decrement() ;
		///< Decrement-but-block-if-zero operator.
		///< Used for mutex locking.

	bool decrement( int timeout_s ) ;
		///< Overload with a timeout. Returns true if decremented.

private:
	Semaphore( const Semaphore & ) ;
	void operator=( const Semaphore & ) ;

private:
	friend class G::SemaphoreImp ;
	storage_type m_storage ; 
} ;

#endif

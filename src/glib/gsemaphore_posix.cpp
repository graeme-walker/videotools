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
// gsemaphore_posix.cpp
//

#include "gdef.h"
#include "gsemaphore.h"
#include "gprocess.h"
#include "gassert.h"
#include <semaphore.h>
#include <errno.h>

typedef char static_assert_sem_t_fits_in_storage_type[sizeof(sem_t)<=sizeof(G::Semaphore::storage_type)?1:-1] ; // if this fails to compile then increase the size of the Semaphore::storage_type::filler array

static sem_t * ptr( G::Semaphore * s )
{
	return reinterpret_cast<sem_t*>(s) ;
}

// ==

G::Semaphore * G::Semaphore::at( storage_type * p )
{
	// SignalSafe
	return reinterpret_cast<G::Semaphore*>(p) ;
}

G::Semaphore::Semaphore( unsigned int initial_value )
{
	G_ASSERT( reinterpret_cast<void*>(this) == reinterpret_cast<void*>(&m_storage) ) ;
	int shared = 1 ;
	if( 0 != ::sem_init( ptr(this) , shared , initial_value ) )
	{
		int e = errno ;
		throw Error( "sem_init" , G::Process::strerror(e) ) ;
	}
}

G::Semaphore::~Semaphore()
{
	::sem_destroy( ptr(this) ) ;
}

void G::Semaphore::increment()
{
	// SignalSafe
	::sem_post( ptr(this) ) ;
}

#ifdef G_UNIX_LINUX
bool G::Semaphore::decrement( int timeout )
{
	struct timespec ts = { ::time(nullptr)+timeout , 0 } ;
	int rc = ::sem_timedwait( ptr(this) , &ts ) ;
	return rc == 0 ;
}
#endif

void G::Semaphore::decrement()
{
	::sem_wait( ptr(this) ) ;
}

/// \file gsemaphore_posix.cpp

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
// gsemaphore_sysv.cpp
//

#include "gdef.h"
#include "gsemaphore.h"
#include "gprocess.h"
#include <stdexcept>
#include <string>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

G::Semaphore * G::Semaphore::at( storage_type * p )
{
	// SignalSafe
	return reinterpret_cast<G::Semaphore*>(p) ;
}

G::Semaphore::Semaphore( unsigned int initial_value )
{
	int id = ::semget( IPC_PRIVATE , 1 , IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR ) ;
	if( id < 0 )
		throw std::runtime_error( "semget failed" ) ;

	m_storage.filler[0] = static_cast<unsigned int>(id) ;

	union { int val ; void * p ; } u ; u.val = static_cast<int>(initial_value) ;
	int rc = ::semctl( id , 0 , SETVAL , u ) ;
	if( rc < 0 )
	{
		int e = errno ;
		throw std::runtime_error( std::string("semctl setval failed: ") + G::Process::strerror(e) ) ;
	}
}

G::Semaphore::~Semaphore()
{
	int id = static_cast<int>( m_storage.filler[0] ) ;
	m_storage.filler[0] = -1 ;
	::semctl( id , 0 , IPC_RMID ) ;
}

void G::Semaphore::increment()
{
	// SignalSafe
	int id = static_cast<int>( m_storage.filler[0] ) ;
	struct ::sembuf op_zero ;
	struct ::sembuf op = op_zero ;
	op.sem_num = 0 ; // sem_num'th in the set
	op.sem_op = 1 ;
	op.sem_flg = 0 ;
	::semop( id , &op , 1 ) ;
}

#ifdef G_UNIX_LINUX
bool G::Semaphore::decrement( int timeout )
{
	int id = static_cast<int>( m_storage.filler[0] ) ;
	struct ::sembuf op_zero ;
	struct ::sembuf op = op_zero ;
	op.sem_num = 0 ; // sem_num'th in the set
	op.sem_op = -1 ;
	op.sem_flg = 0 ;
	struct timespec ts = { timeout , 0 } ;
	int rc = ::semtimedop( id , &op , 1 , &ts ) ; // not in freebsd
	if( rc < 0 && errno != EAGAIN )
		throw std::runtime_error( "semtimedop decrement failed" ) ;
	return rc >= 0 ;
}
#endif

void G::Semaphore::decrement()
{
	int id = static_cast<int>( m_storage.filler[0] ) ;
	struct ::sembuf op_zero ;
	struct ::sembuf op = op_zero ;
	op.sem_num = 0 ; // sem_num'th in the set
	op.sem_op = -1 ;
	op.sem_flg = 0 ;
	int rc = ::semop( id , &op , 1 ) ;
	if( rc < 0 )
		throw std::runtime_error( "semop decrement failed" ) ;
}

/// \file gsemaphore_sysv.cpp

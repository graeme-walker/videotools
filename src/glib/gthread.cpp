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
// gthread.cpp
//

#include "gdef.h"

namespace 
{
	void test_fn( void ) {}
}

bool G::threading::works()
{
	if( using_std_thread ) 
	{
		try
		{
			threading::thread_type t( test_fn ) ;
			t.join() ;
			threading::mutex_type mutex ;
			threading::lock_type lock( mutex ) ;
			return true ;
		}
		catch(...)
		{
			// eg. gcc std::thread builds okay with -std=c++11 but throws 
			// at run-time if not also built with "-pthread" - also, linking 
			// with -lGL suppresses linking with libpthread.so and breaks
			// threading at run-time
			return false ;
		}
	}
	else
	{
		return false ;
	}
}

/// \file gthread.cpp

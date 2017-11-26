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
/// \file gvdatabase.h
///

#ifndef GV_DATABASE__H
#define GV_DATABASE__H

#include "gdef.h"
#include "gdatetime.h"
#include <string>

namespace Gv
{
	class Database ;
}

/// \class Gv::Database
/// An abstract interface for a timeseries database that holds video analysis 
/// statistics.
/// 
class Gv::Database
{
public:
	static Database * create( const std::string & path ) ;
		///< Factory function.

	virtual void add( time_t t , unsigned int n , unsigned int dx , unsigned int dy , unsigned int squelch , unsigned int apathy ) = 0 ;
		///< Adds a data point for an analysis value 'n' at time 't', together with
		///< the analysis parameters (image size, 'squelch' and 'apathy').

	virtual std::string graph( std::string & , time_t start_time , time_t end_time ) = 0 ;
		///< Creates a graph image in the supplied string buffer and returns
		///< the image type. Returns the empty string if not valid.

	virtual ~Database() ;
		///< Destructor.

private:
	void operator=( const Database & ) ;
} ;

inline
Gv::Database::~Database()
{
}

#endif

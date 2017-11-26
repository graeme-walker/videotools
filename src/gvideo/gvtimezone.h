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
/// \file gvtimezone.h
///

#ifndef GV_TIMEZONE__H
#define GV_TIMEZONE__H

#include "gdef.h"
#include <string>

namespace Gv
{
	class Timezone ;
}

/// \class Gv::Timezone
/// A representation of a timezone.
/// 
class Gv::Timezone
{
public:
	explicit Timezone( int hours = 0 ) ;
		///< Constructor.

	explicit Timezone( const std::string & hours ) ;
		///< Constructor with support for for half hours (eg. "-8.5").
		///< Throws on error.

	bool zero() const ;
		///< Returns true for utc.

	std::string str() const ;
		///< Returns a five-character "+/-hhmm" representation.

	std::time_t seconds() const ;
		///< Returns the offset as a signed number of seconds.

private:
	int m_minutes ;
} ;

#endif

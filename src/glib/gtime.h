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
/// \file gtime.h
///

#ifndef G_TIME_H
#define G_TIME_H

#include "gdef.h"
#include "gexception.h"
#include "gdatetime.h"
#include <ctime>

namespace G
{
	class Time ;
}

/// \class G::Time
/// A simple time-of-day (hh/mm/ss) class.
/// \see G::Date, G::DateTime
/// 
class G::Time
{
public:
	class LocalTime /// An overload discriminator class for Time constructors.
		{} ;

	Time() ;
		///< Constructor for the current time, using UTC.

	explicit Time( const G::DateTime::BrokenDownTime & tm ) ;
		///< Constructor for the given broken-down time.

	explicit Time( G::EpochTime t ) ;
		///< Constructor for the given epoch time, using UTC.

	Time( G::EpochTime t , const LocalTime & ) ;
		///< Constructor for the given epoch time, using the local timezone.

	explicit Time( const LocalTime & ) ;
		///< Constructor for the current time, using the local timezone.

	int hours() const ;
		///< Returns the hours (0 <= h < 24).

	int minutes() const ;
		///< Returns the minutes (0 <= m < 60).

	int seconds() const ;
		///< Returns the seconds (0 <= s <= 61).

	std::string hhmmss( const char * sep = nullptr ) const ;
		///< Returns the hhmmss string.

	std::string hhmm( const char * sep = nullptr ) const ;
		///< Returns the hhmm string.

	std::string ss() const ;
		///< Returns the seconds as a two-digit decimal string.

private:
	int m_hh ;
	int m_mm ;
	int m_ss ;
} ;

#endif

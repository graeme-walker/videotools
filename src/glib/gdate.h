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
/// \file gdate.h
///

#ifndef G_DATE_H
#define G_DATE_H

#include "gdef.h"
#include "gdatetime.h"
#include "gdebug.h"
#include <ctime>
#include <string>

namespace G
{
	class Date ;
}

/// \class G::Date
/// A date (dd/mm/yyyy) class.
/// \see G::Time, G::DateTime
/// 
class G::Date
{
public:
	class LocalTime /// An overload discriminator class for Date constructors.
		{} ;

	enum Weekday 
		{ sunday, monday, tuesday, wednesday, thursday, friday, saturday } ;

	enum Month 
		{ january = 1 , february , march , april , may , june , july , 
		august , september , october , november , december } ;

	enum Format
		{ yyyy_mm_dd_slash , yyyy_mm_dd , mm_dd } ;

	static int yearUpperLimit() ;
		///< Returns the largest supported year value.

	static int yearLowerLimit() ;
		///< Returns the smallest supported year value.

	Date() ;
		///< Default constructor for the current date
		///< in the UTC timezone.

	explicit Date( const LocalTime & ) ;
		///< Constructor for the current date
		///< in the local timezone.

	Date( const G::DateTime::BrokenDownTime & tm ) ;
		///< Constructor for the specified date.

	explicit Date( G::EpochTime t ) ;
		///< Constructor for the date in the UTC
		///< timezone as at the given epoch time.

	Date( G::EpochTime t , const LocalTime & ) ;
		///< Constructor for the date in the local
		///< timezone as at the given epoch time.

	Date( int year , Month month , int day_of_month ) ;
		///< Constructor for the specified date.

	std::string string( Format format = yyyy_mm_dd_slash ) const ;
		///< Returns a string representation of the date.

	Weekday weekday() const ;
		///< Returns the day of the week.

	std::string weekdayName( bool brief = false ) const ;
		///< Returns an english string representation of 
		///< the day of the week.
		///< (Was weekdayString().)

	int monthday() const ;
		///< Returns the day of the month.

	std::string dd() const ;
		///< Returns the day of the month as a two-digit decimal string.
		///< (Was monthdayString().)

	Month month() const ;
		///< Returns the month.

	std::string monthName( bool brief = false ) const ;
		///< Returns the month as a string (in english).

	std::string mm() const ;
		///< Returns the month as a two-digit decimal string.

	int year() const ;
		///< Returns the year.

	std::string yyyy() const ;
		///< Returns the year as a four-digit decimal string.
		///< (Was yearString().)

	Date & operator++() ;
		///< Increments the date by one day.

	Date & operator--() ;
		///< Decrements the date by one day.

	bool operator==( const Date & rhs ) const ;
		///< Comparison operator.

	bool operator!=( const Date & rhs ) const ;
		///< Comparison operator.

private:
	void init( const G::DateTime::BrokenDownTime & ) ;
	static int lastDay( int month , int year ) ;
	static bool isLeapYear( int y ) ;

private:
	int m_day ;
	int m_month ;
	int m_year ;
	bool m_weekday_set ;
	Weekday m_weekday ;
} ;

#endif

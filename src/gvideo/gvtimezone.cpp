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
// gvtimezone.cpp
//

#include "gdef.h"
#include "gvtimezone.h"
#include "gdatetime.h"
#include "gdate.h"
#include "gtime.h"
#include "gassert.h"
#include "gstr.h"
#include <stdexcept>

Gv::Timezone::Timezone( int hours ) :
	m_minutes(hours*60)
{
	if( hours < -11 || hours > 12 )
		throw std::runtime_error( "timezone must be between -11 and 12" ) ;
}

Gv::Timezone::Timezone( const std::string & hours_in )
{
	std::string hours( hours_in ) ;
	bool negative = !hours.empty() && hours.at(0U) == '-' ;
	bool positive = !hours.empty() && hours.at(0U) == '+' ;
	if( negative || positive ) hours.erase(0U,1U) ;
	std::string::size_type pos = hours.find(".") ;
	std::string head = G::Str::head( hours , pos , hours ) ;
	std::string tail = G::Str::tail( hours , pos , "0" ) ;

	if( hours.empty() || 
		head.empty() || !G::Str::isUInt(head) || tail.empty() || !G::Str::isUInt(tail) ||
		G::Str::toUInt(head) > (negative?11U:12U) || 
		(G::Str::toUInt(tail) != 0U && G::Str::toUInt(tail)!=5U) )
			throw std::runtime_error( "invalid timezone: use a signed integral number of hours" ) ;

	unsigned int minutes = G::Str::toUInt(head)*60U + (G::Str::toUInt(tail)==5U?30U:0U) ;
	m_minutes = static_cast<int>(minutes) ;
	if( negative ) m_minutes = -m_minutes ;
}

bool Gv::Timezone::zero() const
{
	return m_minutes == 0 ;
}

std::time_t Gv::Timezone::seconds() const
{
	G_ASSERT( std::time_t(-1) < std::time_t(0) ) ;
	return std::time_t(m_minutes) * 60 ;
}

std::string Gv::Timezone::str() const
{
	unsigned int uminutes = m_minutes < 0 ? 
		static_cast<unsigned int>(-m_minutes) : 
		static_cast<unsigned int>(m_minutes) ;

	unsigned int hh = uminutes / 60U ;
	unsigned int mm = uminutes % 60U ;

	std::ostringstream ss ;
	ss << (m_minutes<0?'-':'+') << (hh/10U) << (hh%10U) << (mm/10U) << (mm%10U) ;
	return ss.str() ;
}

/// \file gvtimezone.cpp

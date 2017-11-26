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
// gvribbon.cpp
//

#include "gdef.h"
#include "gvribbon.h"
#include "gstr.h"
#include "gdatetime.h"
#include "gdate.h"
#include "gtime.h"
#include "gfiletree.h"
#include "gassert.h"
#include <algorithm>
#include <string>
#include <ctime>

int Gv::Ribbon::m_height = 12 ;

Gv::Ribbon::Ribbon()
{
}

Gv::Ribbon::Ribbon( size_t size , const G::Path & scan_base , const std::string & name , const Gv::Timezone & tz ) :
	m_scan_base(scan_base) ,
	m_tz(tz) ,
	m_list(size)
{
	const std::string prefix = name.empty() ? std::string() : ( name + "." ) ;
	m_match1 = "/####/##/##/##/##/##/" + prefix + "##" ; // /yyyy/mm/dd/hh/mm/ss/[<name>.]##[.<ext>]
	m_match2 = "/####/##/##/##/##/##/###/" + prefix + "##" ; // /yyyy/mm/dd/hh/mm/ss/msm/[<name>.]##[.<ext>]
}

size_t Gv::Ribbon::timepos( const G::Path & path , const std::string & name )
{
	const std::string prefix = name.empty() ? std::string() : ( name + "." ) ;
	std::string match1 = "/####/##/##/##/##/##/" + prefix + "##" ;
	std::string match2 = "/####/##/##/##/##/##/###/" + prefix + "##" ;
	return timeposImp( path , match1 , match2 ) ;
}

size_t Gv::Ribbon::timepos( const G::Path & path ) const
{
	return timeposImp( path , m_match1 , m_match2 ) ;
}

size_t Gv::Ribbon::timeposImp( const G::Path & path , const std::string & match1 , const std::string & match2 )
{
	G_ASSERT( path != G::Path() ) ;
	if( path == G::Path() ) 
		return 0U ;

	std::string path_str = path.withoutExtension().str() ;
	if( path_str.length() < match1.length() )
		return 0U ;

	std::string::iterator const end = path_str.end() ;
	for( std::string::iterator p = path_str.begin() ; p != end ; ++p )
	{
		if( *p == '#' ) *p = '_' ;
		if( *p >= '0' && *p <= '9' ) *p = '#' ;
	}

	size_t pos = path_str.find( match1 ) ;
	if( pos != std::string::npos )
		return pos+1U ;

	pos = path_str.find( match2 ) ;
	return pos == std::string::npos ? 0U : (pos+1U) ;
}

void Gv::Ribbon::clear()
{
	m_list.assign( m_list.size() , Item() ) ;
}

void Gv::Ribbon::scan( const G::Path & trigger_path , size_t tpos )
{
	G_ASSERT( tpos != 0U ) ;
	scan( RibbonRange(trigger_path,tpos,m_tz) ) ;
}

void Gv::Ribbon::scan( const Gv::RibbonRange & range )
{
	if( !m_list.empty() )
	{
		G::FileTree file_tree ;
		if( scanStart( file_tree , range ) )
			scanSome( file_tree , G::EpochTime(0) ) ;
	}
}

bool Gv::Ribbon::scanStart( G::FileTree & file_tree , const G::Path & trigger_path , size_t tpos )
{
	G_ASSERT( tpos != 0U ) ;
	return scanStart( file_tree , RibbonRange(trigger_path,tpos,m_tz) ) ;
}

bool Gv::Ribbon::scanStart( G::FileTree & file_tree , const Gv::RibbonRange & range )
{
	if( m_list.empty() ) 
	{
		return false ;
	}
	else
	{
		clear() ;
		m_range = range ;
		G::Path start_path = m_range.startpath() ;
		G_LOG( "Gv::Ribbon::scan: ribbon: starting scan: base=[" << m_scan_base << "] start=[" << start_path << "]" ) ;
		file_tree.reroot( m_scan_base ) ;
		return file_tree.reposition( start_path ) ;
	}
}

bool Gv::Ribbon::scanSome( G::FileTree & file_tree , G::EpochTime interval )
{
	bool limited = interval != G::EpochTime(0) ;
	G::EpochTime limit = limited ? ( G::DateTime::now() + interval ) : G::EpochTime(0) ;

	unsigned int count = 0U ;
	for( G::Path path = file_tree.current() ; path != G::Path() ; path = file_tree.next() , count++ )
	{
		unsigned int ts = m_range.timestamp( path ) ;
		if( ts == 0U ) continue ;
		if( ts >= m_range.end() ) break ;
		m_list.at(bucket(ts)).update( path , ts ) ;
		if( limited && G::DateTime::now() > limit )
		{
			G_LOG( "Gv::Ribbon::scan: ribbon: partial scan: count=" << (count+1U) ) ;
			file_tree.next() ;
			return false ;
		}
	}
	G_LOG( "Gv::Ribbon::scan: ribbon: scan complete: count=" << count ) ;
	return true ;
}

bool Gv::Ribbon::apply( const G::Path & path )
{
	if( !m_list.empty() )
	{
		unsigned int ts = m_range.timestamp( path ) ;
		if( ts == 0U ) 
			return false ;

		bool outside_range = ts < m_range.start() || ts >= m_range.end() ;
		if( outside_range )
		{
			clear() ;
			m_range = m_range.around( path ) ;
		}

		size_t index = bucket( ts ) ;
		Item & item = m_list.at( index ) ;
		item.update( path , ts ) ;
		if( !item.mark ) unmark() ;
		item.mark = true ;

		return outside_range ;
	}
	else
	{
		return false ;
	}
}

void Gv::Ribbon::unmark()
{
	const List::iterator end = m_list.end() ;
	for( List::iterator p = m_list.begin() ; p != end ; ++p )
		(*p).mark = false ;
}

size_t Gv::Ribbon::bucket( unsigned int ts ) const
{
	unsigned int range = m_range.end() - m_range.start() ;
	size_t index = (ts-m_range.start()) * m_list.size() ; 
	index /= range ;
	return std::min( index , m_list.size()-1U ) ;
}

G::Path Gv::Ribbon::first() const
{
	List::const_iterator p = m_list.begin() ;
	while( p != m_list.end() && (*p).first == G::Path() ) 
		++p ;
	return p == m_list.end() ? G::Path() : (*p).first ;
}

G::Path Gv::Ribbon::last() const
{
	if( m_list.empty() ) return G::Path() ;
	List::const_iterator p = m_list.begin() + (m_list.size()-1U) ;
	while( p != m_list.begin() && (*p).first == G::Path() ) 
		--p ;
	return (*p).last ;
}

G::Path Gv::Ribbon::find( size_t bucket , bool before ) const
{
	G_ASSERT( bucket < m_list.size() ) ;
	List::const_iterator p = m_list.begin() + bucket ; 
	if( before )
	{
		while( p != m_list.begin() && (*p).first == G::Path() ) 
			--p ;
		return (*p).first == G::Path() ? first() : (*p).last ;
	}
	else
	{
		while( p != m_list.end() && (*p).first == G::Path() ) 
			++p ;
		return p == m_list.end() ? last() : (*p).first ;
	}
}

size_t Gv::Ribbon::size() const
{
	return m_list.size() ;
}

int Gv::Ribbon::height()
{
	return m_height ;
}

bool Gv::Ribbon::timestamped( const G::Path & path ) const
{
	return timepos( path ) != 0U ;
}

Gv::Ribbon::List::const_iterator Gv::Ribbon::begin() const
{
	return m_list.begin() ;
}

Gv::Ribbon::List::const_iterator Gv::Ribbon::end() const
{
	return m_list.end() ;
}

const Gv::RibbonRange & Gv::Ribbon::range() const
{
	return m_range ;
}

Gv::RibbonRange Gv::Ribbon::range( int offset ) const
{
	return m_range(offset) ;
}

// ==

Gv::RibbonRange::RibbonRange() :
	m_start(0) ,
	m_end(24U*3600U) ,
	m_startpath("/2000/01/01/00/00") ,
	m_endpath("/2000/01/02/00/00")
{
}

Gv::RibbonRange::RibbonRange( const G::Path & trigger_path , size_t tpos , const Gv::Timezone & tz ) :
	m_tpos(tpos) ,
	m_tz(tz) ,
	m_start(0) ,
	m_end(0)
{
	const unsigned int day_seconds = 24U * 3600U ;
	m_start = daystamp( bdt(trigger_path.str(),tpos) , tz ) ;
	m_end = m_start ? (m_start+day_seconds) : 0U ;

	if( m_start != 0 )
	{
		m_startpath = makePath( trigger_path , tpos , G::EpochTime(m_start) ) ;
		m_endpath = makePath( trigger_path , tpos , G::EpochTime(m_end) ) ;
	}

	G_LOG( "Gv::RibbonRange::ctor: ribbon: trigger-path=[" << trigger_path << "] " << "startpath=[" << m_startpath << "]" ) ;
}

Gv::RibbonRange Gv::RibbonRange::around( const G::Path & path ) const
{
	return RibbonRange( path , m_tpos , m_tz ) ;
}

Gv::RibbonRange Gv::RibbonRange::operator()( int offset ) const
{
	const int day_seconds = 3600*24 ;

	RibbonRange result ;
	result.m_start = m_start + ( offset * day_seconds ) ;
	result.m_end = m_end + ( offset * day_seconds ) ;
	result.m_startpath = makePath( m_startpath , m_tpos , G::EpochTime(result.m_start) ) ;
	result.m_endpath = makePath( m_startpath/*sic*/ , m_tpos , G::EpochTime(result.m_end) ) ;
	return result ;
}

G::Path Gv::RibbonRange::makePath( const G::Path & trigger_path , size_t tpos , G::EpochTime t )
{
	Tm tm = G::DateTime::utc( t ) ;
	G::Date date( tm ) ;
	G::Time time( tm ) ;
	return trigger_path.str().substr(0U,tpos) + "/" + date.string() + "/" + time.hhmm("/") ;
}

unsigned int Gv::RibbonRange::start() const
{
	return m_start < epoch_base ? 0U : static_cast<unsigned int>(m_start-epoch_base) ;
}

unsigned int Gv::RibbonRange::end() const
{
	return m_end < epoch_base ? 0U : static_cast<unsigned int>(m_end-epoch_base) ;
}

G::Path Gv::RibbonRange::startpath() const
{
	return m_startpath ;
}

G::Path Gv::RibbonRange::endpath() const
{
	return m_endpath ;
}

unsigned int Gv::RibbonRange::timestamp( const G::Path & path ) const
{
	return timestamp( path , m_tpos ) ;
}

unsigned int Gv::RibbonRange::timestamp( const G::Path & path , size_t tpos )
{
	if( tpos == 0U ) 
		return 0U ;

	G::DateTime::BrokenDownTime tm = bdt( path.str() , tpos ) ;
	if( tm.tm_year == 0 ) 
		return 0U ;

	std::time_t t = G::DateTime::epochTime(tm).s ;
	if( t == std::time_t(-1) || t < epoch_base ) 
		return 0U ;

	return static_cast<unsigned int>(t-epoch_base) ;
}

std::time_t Gv::RibbonRange::daystamp( G::DateTime::BrokenDownTime tm , const Gv::Timezone & tz )
{
	const std::time_t day_seconds = 3600*24 ;
	if( tm.tm_year == 0 ) return 0 ;
	G_ASSERT( tm.tm_hour >= 0 && tm.tm_min >= 0 ) ;

	// round down
	std::time_t day_offset = std::time_t(tm.tm_hour)*3600 + std::time_t(tm.tm_min)*60 + tm.tm_sec ;
	tm.tm_hour = tm.tm_min = tm.tm_sec = 0 ; // reset to start-of-day
	std::time_t start_of_day = G::DateTime::epochTime(tm).s ;
	if( start_of_day == std::time_t(-1) ) return 0 ;

	// adjust
	start_of_day += tz.seconds() ;

	// carry
	if( tz.seconds() > 0 && day_offset < tz.seconds() )
		start_of_day -= day_seconds ;
	else if( tz.seconds() < 0 && day_offset > (day_seconds+/*sic*/tz.seconds()) )
		start_of_day += day_seconds ;

	return start_of_day ;
}

G::DateTime::BrokenDownTime Gv::RibbonRange::bdt( const std::string & path , size_t tpos )
{
	static Tm tm_zero ;
	if( tpos == 0U )
		return tm_zero ;

	G_ASSERT( tpos < path.length() ) ;
	G_ASSERT( path.at(tpos+4U) == '/' && path.at(tpos+7U) == '/' ) ;
	G_ASSERT( path.at(tpos+10U) == '/' && path.at(tpos+13U) == '/' ) ;
	G_ASSERT( path.at(tpos+16U) == '/' ) ;

	const char * p = path.data() + tpos ;

	unsigned int yy = to_int( p[2] , p[3] ) ;
	unsigned int mm = to_int( p[5] , p[6] ) ;
	unsigned int dd = to_int( p[8] , p[9] ) ;
	unsigned int HH = to_int( p[11], p[12] ) ;
	unsigned int MM = to_int( p[14], p[15] ) ;

	const bool valid = 
		yy <= 99U && 
		mm >= 1U && mm <= 12U &&
		dd >= 1U && mm <= 31U &&
		HH <= 23U &&
		MM <= 59U ;

	Tm tm = tm_zero ;
	if( valid )
	{
		tm.tm_sec = 0 ;
		tm.tm_min = MM ;
		tm.tm_hour = HH ;
		tm.tm_mday = dd ;
		tm.tm_mon = mm - 1U ;
		tm.tm_year = yy + 100U ;
		tm.tm_wday = 0 ;
		tm.tm_yday = 0 ;
		tm.tm_isdst = 0 ;
	}
	return tm ;
}

unsigned int Gv::RibbonRange::to_int( char a , char b , unsigned int error )
{
	if( a < '0' || a > '9' || b < '0' || b > '9' ) return error ;
	unsigned int hi = static_cast<unsigned int>(a-'0') ;
	unsigned int lo = static_cast<unsigned int>(b-'0') ;
	return hi * 10U + lo ;
}

// ==

Gv::Ribbon::Item::Item() :
	first_ts(0U) ,
	last_ts(0U) ,
	mark(false)
{
}

void Gv::Ribbon::Item::update( const G::Path & path , unsigned int ts )
{
	G_ASSERT( path != G::Path() && ts != 0U && (first_ts==0U) == (first==G::Path()) ) ;
	if( first_ts == 0U )
	{
		first = last = path ;
		first_ts = last_ts = ts ;
	}
	else if( ts < first_ts || ( ts == first_ts && path.str() <= first.str() ) )
	{
		first = path ;
		first_ts = ts ;
	}
	else if( ts > last_ts || ( ts == last_ts && path.str() > last.str() ) )
	{
		last = path ;
		last_ts = ts ;
	}
}

bool Gv::Ribbon::Item::set() const
{
	return first_ts != 0U ;
}

bool Gv::Ribbon::Item::includes( const G::Path & path ) const
{
	return set() && path.str() >= first.str() && path.str() <= last.str() ;
}

bool Gv::Ribbon::Item::marked() const
{
	return mark ;
}
/// \file gvribbon.cpp

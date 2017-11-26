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
/// \file gvribbon.h
///

#ifndef GV_RIBBON__H
#define GV_RIBBON__H

#include "gdef.h"
#include "gpath.h"
#include "gfiletree.h"
#include "gvtimezone.h"
#include "gdatetime.h"
#include <vector>

namespace Gv
{
	class Ribbon ;
	class RibbonRange ;
}

/// \class Gv::RibbonRange
/// Represents a day span within the file store, used by Gv::Ribbon.
/// 
class Gv::RibbonRange
{
public:
	RibbonRange( const G::Path & trigger_path , size_t tpos , const Gv::Timezone & tz ) ;
		///< Constructor for the day span that includes the given
		///< timestamped trigger path. The root of the file
		///< store is given by the first tpos characters
		///< of the trigger path.

	RibbonRange() ;
		///< Default constructor for an unusable object.

	unsigned int start() const ;
		///< Returns the timestamp() value for the start of the day.

	unsigned int end() const ;
		///< Returns the timestamp() value for the start of the next day.

	G::Path startpath() const ;
		///< Returns the directory path that is the start of the day
		///< at the minutes level (eg. "/data/2000/01/01/00/00"),
		///< taking the timezone into account.

	G::Path endpath() const ;
		///< Returns the directory path that is the start of the 
		///< next day at the minutes level.

	RibbonRange around( const G::Path & ) const ;
		///< Returns a range includes the given timestamped trigger path.
		///< The root directory and timezone carry over.

	RibbonRange operator()( int offset ) const ;
		///< Returns a range that is offset from the current range.

	unsigned int timestamp( const G::Path & ) const ;
		///< Returns the time value for the given path, as seconds 
		///< within some arbitrary epoch.

private:
	typedef G::DateTime::BrokenDownTime Tm ;
	static constexpr std::time_t epoch_base = 946684800 ; // 2000
	static unsigned int to_int( char a , char b , unsigned int error = 100U ) ;
	static Tm bdt( const std::string & , size_t ) ;
	static std::time_t daystamp( Tm , const Gv::Timezone & tz ) ;
	static G::Path makePath( const G::Path & trigger_path , size_t pos , G::EpochTime t ) ;
	static unsigned int timestamp( const G::Path & , size_t tpos ) ;

private:
	size_t m_tpos ;
	Gv::Timezone m_tz ;
	std::time_t m_start ;
	std::time_t m_end ;
	G::Path m_startpath ;
	G::Path m_endpath ;
} ;

/// \class Gv::Ribbon
/// Holds ribbon data that can be used to add a ribbon widget to a video stream display. The
/// ribbon is populated by searching the filesystem for files with a suitable 'timestampy' 
/// path, ie. like '/base/yyyy/mm/dd/hh/mm/ss.ext' or '/base/yyyy/mm/dd/hh/mm/ss/xxx.ext'.
/// If the timestamp is in the relevant range the file is allocated into one of a number 
/// of time buckets.
/// 
class Gv::Ribbon
{
public:
	struct Item /// A time bucket within a Gv::Ribbon.
	{
		G::Path first ;
		G::Path last ;
		unsigned int first_ts ;
		unsigned int last_ts ;
		bool mark ;
		Item() ;
		bool set() const ;
		bool includes( const G::Path & ) const ;
		void update( const G::Path & , unsigned int ts ) ;
		bool marked() const ;
	} ;
	typedef std::vector<Item> List ;

	Ribbon() ;
		///< Default constructor for a zero-size() ribbon.

	Ribbon( size_t size , const G::Path & base , const std::string & name , const Gv::Timezone & tz ) ;
		///< Constructor. The size parameter is the number of time buckets, which 
		///< is typically the number of horizontal pixels. The base directory
		///< and timezone parameters are used in scan()ning.
		///< Use scan() to populate the ribbon.

	void scan( const G::Path & path , size_t tpos ) ;
		///< Does a complete scan for the day that encompases the given timestamped() file.
		///< Precondition: timestamped(path)

	void scan( const RibbonRange & range ) ;
		///< Does a complete scan for the given day range.

	bool scanStart( G::FileTree & , const RibbonRange & range ) ;
		///< Prepares for an iterative scan, with subsequent calls to scanSome(). 
		///< The default-constructed G::FileTree object contains the scan
		///< state. Returns true if there is scanSome() work to do.

	bool scanStart( G::FileTree & , const G::Path & path , size_t tpos ) ;
		///< Prepares for an iterative scan, with subsequent calls to scanSome(). 
		///< The default-constructed G::FileTree object contains the scan
		///< state. Returns true if there is scanSome() work to do.
		///< Precondition: timestamped(path)

	bool scanSome( G::FileTree & , G::EpochTime interval ) ;
		///< Does a partial scan, limited by the given time interval.
		///< Returns true if the scan is complete.

	G::Path find( size_t offset , bool before = false ) const ;
		///< Finds the first scanned path at-or-after the given bucket position, or 
		///< returns the last() path if there are no paths at-or-after the given 
		///< position. Alternatively, finds the first scanned path at-or-before the 
		///< given position, or returns the first() path if there are no paths 
		///< at-or-before the given position.
		///< Precondition: offset < size()

	G::Path last() const ;
		///< Returns the last scanned path. Returns the empty path if all the buckets
		///< are empty.

	G::Path first() const ;
		///< Returns the first scanned path. Returns the empty path if all the buckets
		///< are empty.

	bool apply( const G::Path & ) ;
		///< Called when a possibly-new timestamped file is encountered, allowing the
		///< ribbon to update itself for that file. Returns true if the apply()d file 
		///< is outside the existing day range, implying that the ribbon needs 
		///< to be rescanned.

	static int height() ;
		///< Returns a suggested height of the ribbon, in pixels.

	size_t size() const ;
		///< Returns the size of the list, as passed in to the constructor.

	static size_t timepos( const G::Path & path , const std::string & name ) ;
		///< Returns the timestamp position within the given path, or zero if none.

	size_t timepos( const G::Path & path ) const ;
		///< Returns the timestamp position within the given path, or zero if none.

	bool timestamped( const G::Path & path ) const ;
		///< Returns true if the given absolute path has a non-zero timepos().

	List::const_iterator begin() const ;
		///< Returns the item list's begin iterator.

	List::const_iterator end() const ;
		///< Returns the item list's end iterator.

	const RibbonRange & range() const ;
		///< Returns the current day range.

	RibbonRange range( int day_offset ) const ;
		///< Returns a day range that is offset from the current day.

	void mark( const G::Path & ) ;
		///< Marks the bucket that contains the given path. Marks on other buckets are
		///< cleared. See Gv::Ribbon::Item::marked().

private:
	void clear() ;
	size_t bucket( unsigned int ts ) const ;
	static size_t timeposImp( const G::Path & path , const std::string & match1 , const std::string & match2 ) ;
	void unmark() ;

private:
	G::Path m_scan_base ;
	Gv::Timezone m_tz ;
	Gv::RibbonRange m_range ;
	List m_list ;
	static int m_height ;
	size_t m_current ;
	std::string m_match1 ;
	std::string m_match2 ;
} ;

#endif

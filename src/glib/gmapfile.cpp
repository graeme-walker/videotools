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
// gmapfile.cpp
//

#include "gdef.h"
#include "gmapfile.h"
#include "gstr.h"
#include "gconvert.h"
#include "gpath.h"
#include "gprocess.h"
#include "gdatetime.h"
#include "gdate.h"
#include "gtime.h"
#include "gfile.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm> // std::find
#include <iterator>
#include <stdexcept>

G::MapFile::MapFile() :
	m_logging(true)
{
}

G::MapFile::MapFile( const G::Path & path , bool utf8 ) :
	m_logging(true)
{
	if( path != Path() )
		readFrom( path , utf8 ) ;
}

G::MapFile::MapFile( std::istream & stream , bool utf8 ) :
	m_logging(true)
{
	G_LOG( "MapFile::read: start" ) ;
	readFrom( stream , utf8 ) ;
}

G::MapFile::MapFile( const G::StringMap & map ) :
	m_logging(true) ,
	m_map(map)
{
	m_keys.reserve( m_map.size() ) ;
	for( StringMap::iterator p = m_map.begin() ; p != m_map.end() ; ++p )
		m_keys.push_back( (*p).first ) ;
}

G::MapFile::MapFile( const OptionMap & map , const std::string & yes ) :
	m_logging(true)
{
	for( OptionMap::const_iterator p = map.begin() ; p != map.end() ; )
	{
		std::string key = (*p).first ;
		if( map.contains(key) ) // ie. ignore "--foo=off"
		{
			std::string value = (*p).second.valued() ? map.value(key) : yes ;
			log( key , value ) ;
			add( key , value ) ;
		}
		while( p != map.end() && (*p).first == key ) // since we used OptionMap::value()
			++p ;
	}
}

void G::MapFile::readFrom( const G::Path & path , bool utf8 )
{
	std::ifstream stream( path.str().c_str() ) ;
	if( !stream.good() )
		throw std::runtime_error( std::string() + "cannot open \"" + path.str() + "\"" ) ;
	G_LOG( "MapFile::read: reading [" << path.str() << "]" ) ;
	readFrom( stream , utf8 ) ;
}

void G::MapFile::readFrom( std::istream & ss , bool utf8 )
{
	std::string line ;
	while( ss.good() )
	{
		Str::readLineFrom( ss , "\n" , line ) ;
		if( line.empty() )
			continue ;
		if( !ss )
			break ;

		std::string::size_type pos_interesting = line.find_first_not_of(" \t\r#") ;
		if( pos_interesting == std::string::npos )
			continue ;

		std::string::size_type pos_hash = line.find("#") ;
		if( pos_hash != std::string::npos && pos_hash < pos_interesting )
			continue ;

		StringArray part ;
		Str::splitIntoTokens( line , part , " =\t" ) ;
		if( part.size() == 0U )
			continue ;

		std::string value = part.size() == 1U ? std::string() : line.substr(part[0].length()+1U) ;
		value = Str::trimmed( value , Str::ws() ) ;
		if( value.length() >= 2U && value.at(0U) == '"' && value.at(value.length()-1U) == '"' )
			value = value.substr(1U,value.length()-2U) ;

		std::string key = part[0] ;
		log( key , value ) ;

		value = fromUtf( value , utf8 ) ;

		add( key , value ) ;
	}
}

void G::MapFile::check( const G::Path & path , bool utf8 )
{
	MapFile tmp ;
	tmp.m_logging = false ;
	tmp.readFrom( path , utf8 ) ;
}

void G::MapFile::log() const
{
	for( StringArray::const_iterator p = m_keys.begin() ; p != m_keys.end() ; ++p )
		log( (*(m_map.find(*p))).first , (*(m_map.find(*p))).second ) ;
}

void G::MapFile::log( const std::string & key , const std::string & value ) const
{
	log( m_logging , key , value ) ;
}

void G::MapFile::log( bool logging , const std::string & key , const std::string & value )
{
	if( logging )
		G_LOG( "MapFile::item: " << key << "=[" << 
			( key.find("password") == std::string::npos ?
				Str::printable(value) :
				std::string("<not-logged>")
		) << "]" ) ;
}

void G::MapFile::writeItem( std::ostream & stream , const std::string & key , bool utf8 ) const
{
	std::string value = m_map.find(key) == m_map.end() ? std::string() : (*m_map.find(key)).second ;
	writeItem( stream , key , value , utf8 ) ;
}

void G::MapFile::writeItem( std::ostream & stream , const std::string & key , const std::string & value , bool utf8 ) 
{
	log( true , key , value ) ;
	const char * qq = value.find(' ') == std::string::npos ? "" : "\"" ;
	stream << key << "=" << qq << toUtf(value,utf8) << qq << "\n" ;
}

std::string G::MapFile::quote( const std::string & s )
{
	return s.find_first_of(" \t") == std::string::npos ? s : (std::string()+"\""+s+"\"") ;
}

void G::MapFile::editInto( const G::Path & path , bool make_backup , bool allow_read_error , bool allow_write_error , bool utf8 ) const
{
	typedef std::list<std::string> List ;
	List lines = read( path , allow_read_error ) ;
	commentOut( lines ) ;
	replace( lines , utf8 ) ;
	if( make_backup ) backup( path ) ;
	save( path , lines , allow_write_error ) ;
}

G::MapFile::List G::MapFile::read( const G::Path & path , bool allow_read_error ) const
{
	List line_list ;
	std::ifstream file_in( path.str().c_str() ) ;
	if( !file_in.good() && !allow_read_error )
		throw std::runtime_error( std::string() + "cannot read \"" + path.str() + "\"" ) ;
	while( file_in.good() )
	{
		std::string line = Str::readLineFrom( file_in , "\n" ) ;
		if( !file_in ) break ;
		line_list.push_back( line ) ;
	}
	return line_list ;
}

void G::MapFile::commentOut( List & line_list ) const
{
	for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
	{
		std::string & line = *line_p ;
		if( line.empty() || line.at(0U) == '#' )
			continue ;
		line = std::string(1U,'#') + line ;
	}
}

void G::MapFile::replace( List & line_list , bool utf8 ) const
{
	for( StringMap::const_iterator map_p = m_map.begin() ; map_p != m_map.end() ; ++map_p )
	{
		bool found = false ;
		for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
		{
			std::string & line = (*line_p) ;
			if( line.empty() ) continue ;
			StringArray part ;
			Str::splitIntoTokens( line , part , Str::ws()+"=#" ) ;
			if( part.size() == 0U ) continue ;
			if( part.at(0U) == (*map_p).first )
			{
				std::string value = toUtf( (*map_p).second , utf8 ) ;
				line = Str::trimmed( (*map_p).first + " " + quote(value) , Str::ws() ) ;
				found = true ;
				break ;
			}
		}

		if( !found )
		{
			std::string value = toUtf( (*map_p).second , utf8 ) ;
			line_list.push_back( Str::trimmed( (*map_p).first + " " + quote(value) , Str::ws() ) ) ;
		}
	}
}

void G::MapFile::backup( const G::Path & path )
{
	// ignore errors
	DateTime::BrokenDownTime now = DateTime::local( DateTime::now() ) ;
	std::string timestamp = Date(now).string(Date::yyyy_mm_dd) + Time(now).hhmmss() ;
	Path backup( path.dirname() , path.basename() + "." + timestamp ) ;
	Process::Umask umask( Process::Umask::Tightest ) ;
	File::copy( path , backup , File::NoThrow() ) ;
}

void G::MapFile::save( const G::Path & path , List & line_list , bool allow_write_error )
{
	std::ofstream file_out( path.str().c_str() , std::ios_base::out | std::ios_base::trunc ) ;
	std::copy( line_list.begin() , line_list.end() , std::ostream_iterator<std::string>(file_out,"\n") ) ;
	file_out.close() ;
	if( file_out.fail() && !allow_write_error )
		throw std::runtime_error( std::string() + "cannot write \"" + path.str() + "\"" ) ;
}

std::string G::MapFile::value( const std::string & key , const std::string & default_ ) const
{
	StringMap::const_iterator p = m_map.find( key ) ;
	std::string result = ( p == m_map.end() || (*p).second.empty() ) ? default_ : (*p).second ;
	return result ;
}

std::string G::MapFile::value( const std::string & key , const char * default_ ) const
{
	return value( key , std::string(default_) ) ;
}

std::string G::MapFile::mandatoryValue( const std::string & key ) const
{
	if( m_map.find(key) == m_map.end() )
		throw std::runtime_error( std::string() + "cannot find config item \"" + key + "\"" ) ;
	return value( key ) ;
}

G::Path G::MapFile::pathValue( const std::string & key ) const
{
	return Path( mandatoryValue(key) ) ;
}

G::Path G::MapFile::expandedPathValue( const std::string & key ) const
{
	return Path( expand(mandatoryValue(key)) ) ;
}

G::Path G::MapFile::pathValue( const std::string & key , const G::Path & default_ ) const
{
	return Path( value(key,default_.str()) ) ;
}

G::Path G::MapFile::expandedPathValue( const std::string & key , const G::Path & default_ ) const
{
	return Path( expand(value(key,default_.str())) ) ;
}

unsigned int G::MapFile::numericValue( const std::string & key , unsigned int default_ ) const
{
	std::string s = value( key , std::string() ) ;
	return !s.empty() && Str::isUInt(s) ? Str::toUInt( s ) : default_ ;
}

bool G::MapFile::booleanValue( const std::string & key , bool default_ ) const
{
	std::string s = value( key , default_ ? "Y" : "N" ) ;
	return !s.empty() && ( s.at(0U) == 'y' || s.at(0U) == 'Y' ) ;
}

void G::MapFile::remove( const std::string & key )
{
	StringMap::iterator p = m_map.find( key ) ;
	if( p != m_map.end() )
	{
		m_map.erase( p ) ;
		G_ASSERT( std::find(m_keys.begin(),m_keys.end(),key) != m_keys.end() ) ;
		m_keys.erase( std::find(m_keys.begin(),m_keys.end(),key) ) ;
	}
}

std::string G::MapFile::expand( const std::string & value_in ) const
{
	std::string value( value_in ) ;
	expand_( value ) ;
	return value ;
}

namespace
{
	size_t find_single( std::string & s , char c , size_t start_pos )
	{
		char cc[] = { c , '\0' } ;
		size_t pos = start_pos ;
		for(;;)
		{
			pos = s.find( cc , pos ) ;
			if( pos == std::string::npos )
			{
				break ; // not found
			}
			else if( (pos+1U) < s.length() && s.at(pos+1U) == c )
			{
				s.erase( pos , 1U ) ;
				if( (pos+1U) == s.length() ) 
				{
					pos = std::string::npos ;
					break ;
				}
				pos++ ;
			}
			else
			{
				break ; // found
			}
		}
		return pos ;
	}
}

bool G::MapFile::expand_( std::string & value ) const
{
	bool changed = false ;
	size_t start = 0U ;
	size_t end = 0U ;
	size_t const npos = std::string::npos ;
	while( end < value.length() )
	{
		start = find_single( value , '%' , end ) ;
		if( start == npos ) break ;
		end = value.find( "%" , start+1U ) ;
		if( end == npos ) break ;
		end++ ;
		std::string key = value.substr( start+1U , end-start-2U ) ;
		StringMap::const_iterator p = m_map.find( key ) ;
		if( p != m_map.end() )
		{
			size_t old = end - start ;
			size_t new_ = (*p).second.length() ;
			value.replace( start , old , (*p).second ) ;
			end += new_ ;
			end -= old ;
			changed = true ;
		}
	}
	return changed ;
}

void G::MapFile::add( const std::string & key , const std::string & value )
{
	if( m_map.find(key) == m_map.end() )
		m_keys.push_back( key ) ;
	m_map[key] = value ;
}

bool G::MapFile::contains( const std::string & key ) const
{
	return m_map.find( key ) != m_map.end() ;
}

const G::StringMap & G::MapFile::map() const
{
	return m_map ;
}

const G::StringArray & G::MapFile::keys() const
{
	return m_keys ;
}

std::string G::MapFile::fromUtf( const std::string & value , bool utf8 )
{
	if( G::is_windows() && utf8 )
	{
		std::string result ;
		Convert::convert( result , Convert::utf8(value) , Convert::ThrowOnError() ) ;
		return result ;
	}
	else
	{
		return value ;
	}
}

std::string G::MapFile::toUtf( const std::string & value , bool utf8 )
{
	if( G::is_windows() && utf8 )
	{
		Convert::utf8 result ;
		Convert::convert( result , value ) ;
		return result.s ;
	}
	else
	{
		return value ;
	}
}

/// \file gmapfile.cpp

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
// ggetopt.cpp
//	

#include "gdef.h"
#include "ggetopt.h"
#include "gmapfile.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionparser.h"
#include "gstrings.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <fstream>

G::GetOpt::GetOpt( const Arg & args_in , const std::string & spec ) :
	m_spec(spec) ,
	m_args(args_in) ,
	m_parser(m_spec,m_map,m_errors)
{
	parseArgs( m_args ) ;
}

G::GetOpt::GetOpt( const G::StringArray & args_in , const std::string & spec ) :
	m_spec(spec) ,
	m_args(args_in) ,
	m_parser(m_spec,m_map,m_errors)
{
	parseArgs( m_args ) ;
}

void G::GetOpt::reload( const G::StringArray & args_in )
{
	m_map.clear() ;
	m_errors.clear() ;
	m_args = Arg( args_in ) ;

	parseArgs( m_args ) ;
}

void G::GetOpt::parseArgs( Arg & args )
{
	size_t pos = m_parser.parse( args.array() ) ;
	if( pos > 1U )
		m_args.removeAt( 1U , pos-2U ) ;
}

void G::GetOpt::addOptionsFromFile( size_t n )
{
	if( n < m_args.c() )
	{
		std::string filename = m_args.v( n ) ;
		m_args.removeAt( n ) ;

		if( !filename.empty() )
			addOptionsFromFile( filename ) ;
	}
}

G::StringArray G::GetOpt::optionsFromFile( const G::Path & filename ) const
{
	StringArray result ;
	StringMap map = MapFile(filename).map() ;
	for( StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		std::string key = (*p).first ;
		std::string value = (*p).second ;
		if( m_spec.valued(key) )
		{
			result.push_back( "--" + key ) ;
			result.push_back( value ) ;
		}
		else
		{
			result.push_back( "--" + key + "=" + value ) ;
		}
	}
	return result ;
}

void G::GetOpt::addOptionsFromFile( const G::Path & filename )
{
	m_parser.parse( optionsFromFile(filename) , 0U ) ;
}

const G::Options & G::GetOpt::options() const
{
	return m_spec ;
}

G::StringArray G::GetOpt::errorList() const
{
	return m_errors ;
}

bool G::GetOpt::contains( char c ) const
{
	return m_map.contains( m_spec.lookup(c) ) ;
}

bool G::GetOpt::contains( const std::string & name ) const
{
	return m_map.contains( name ) ;
}

unsigned int G::GetOpt::count( const std::string & name ) const
{
	return m_map.count( name ) ;
}

std::string G::GetOpt::value( char c , const std::string & default_ ) const
{
	G_ASSERT( contains(c) ) ;
	return value( m_spec.lookup(c) , default_ ) ;
}

std::string G::GetOpt::value( const std::string & name , const std::string & default_ ) const
{
	return m_map.contains(name) ? m_map.value(name) : default_ ;
}

G::Arg G::GetOpt::args() const
{
	return m_args ;
}

bool G::GetOpt::hasErrors() const
{
	return m_errors.size() != 0U ;
}

void G::GetOpt::showErrors( std::ostream & stream ) const
{
	showErrors( stream , m_args.prefix() + ": error" ) ;
}

void G::GetOpt::showErrors( std::ostream & stream , std::string prefix_1 , std::string prefix_2 ) const
{
	for( StringArray::const_iterator p = m_errors.begin() ; p != m_errors.end() ; ++p )
	{
		stream << prefix_1 << prefix_2 << *p << std::endl ;
	}
}

/// \file ggetopt.cpp

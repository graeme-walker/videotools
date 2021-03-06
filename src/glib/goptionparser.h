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
/// \file goptionparser.h
///

#ifndef G_OPTION_PARSER_H
#define G_OPTION_PARSER_H

#include "gdef.h"
#include "gstrings.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionmap.h"
#include <string>
#include <map>

namespace G
{
	class OptionParser ;
}

/// \class G::OptionParser
/// A parser for command-line arguments that operates according to an Options 
/// specification and returns an OptionValue multimap.
/// \see G::Options, G::OptionValue
/// 
class G::OptionParser
{
public:
	OptionParser( const Options & spec , OptionMap & values_out , StringArray & errors_out ) ;
		///< Constructor. Output references are kept. The output map is a
		///< multimap, but with methods that also allow it to be used as
		///< a simple map with multi-valued options concatenated into a
		///< comma-separated list.

	OptionParser( const Options & spec , OptionMap & values_out ) ;
		///< Constructor for when errors can be ignored.

	size_t parse( const StringArray & args , size_t start_position = 1U ) ;
		///< Parses the given command-line arguments into the value map and/or 
		///< error list defined by the constructor.
		///< 
		///< By default the program name is expected to be the first item in 
		///< the array and it is ignored, although the 'start-position' parameter 
		///< can be used to change this. See also G::Arg::array().
		///< 
		///< Individual arguments can be in short-form like "-c", or long-form 
		///< like "--foo" or "--foo=bar". Long-form arguments can be passed in 
		///< two separate arguments, eg. "--foo" followed by "bar". Short-form 
		///< options can be grouped (eg. "-abc"). Boolean options can be enabled
		///< by (eg.) "--verbose" or "--verbose=yes", and disabled by "--verbose=no". 
		///< Boolean options cannot use two separate arguments (eg. "--verbose" 
		///< followed by "yes").
		///< 
		///< Entries in the output map are keyed by the option's long name, 
		///< even if supplied in short-form.
		///< 
		///< Errors are reported into the error list.
		///< 
		///< Returns the position in the array where the non-option command-line
		///< arguments begin.

private:
	OptionParser( const OptionParser & ) ;
	void operator=( const OptionParser & ) ;
	bool haveSeen( const std::string & ) const ;
	static std::string::size_type eqPos( const std::string & ) ;
	static std::string eqValue( const std::string & , std::string::size_type ) ;
	void processOptionOn( char c ) ;
	void processOption( char c , const std::string & value ) ;
	void processOptionOn( const std::string & s ) ;
	void processOptionOff( const std::string & s ) ;
	void processOption( const std::string & s , const std::string & value , bool ) ;
	void errorNoValue( char ) ;
	void errorNoValue( const std::string & ) ;
	void errorUnknownOption( char ) ;
	void errorUnknownOption( const std::string & ) ;
	void errorDubiousValue( const std::string & , const std::string & ) ;
	void errorDuplicate( char ) ;
	void errorDuplicate( const std::string & ) ;
	void errorExtraValue( char ) ;
	void errorExtraValue( const std::string & ) ;
	void errorConflict( const std::string & ) ;
	bool haveSeenOn( const std::string & name ) const ;
	bool haveSeenOff( const std::string & name ) const ;
	static bool isOldOption( const std::string & ) ;
	static bool isNewOption( const std::string & ) ;
	static bool isAnOptionSet( const std::string & ) ;

private:
	bool m_multimap ;
	const Options & m_spec ;
	OptionMap & m_map ;
	StringArray m_errors_ignored ;
	StringArray & m_errors ;
} ;

#endif

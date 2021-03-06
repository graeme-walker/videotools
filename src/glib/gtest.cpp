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
// gtest.cpp
//

#include "gdef.h"
#include "gtest.h"
#include "gdebug.h"
#include "genvironment.h"
#include <string>
#include <set>

#if defined(_DEBUG) || defined(G_TEST_ENABLED)
bool G::Test::enabled( const char * name )
{
	static std::string p = G::Environment::get("G_TEST",std::string()) ;
	bool result = p.empty() ? false : ( p.find(name) != std::string::npos ) ;
	if( result )
	{
		static std::set<std::string> warned ;
		if( warned.find(name) == warned.end() )
		{
			warned.insert( name ) ;
			G_WARNING( "G::Test::enabled: test case enabled: [" << name << "]" ) ;
		}
	}
	return result ;
}
bool G::Test::enabled()
{
	return true ;
}
#else
bool G::Test::enabled()
{
	return false ;
}
#endif
/// \file gtest.cpp

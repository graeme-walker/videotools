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
// genvironment_unix.cpp
//

#include "gdef.h"
#include "genvironment.h"
#include <cstdlib> // std::getenv()

std::string G::Environment::get( const std::string & name , const std::string & default_ )
{
	const char * p = std::getenv( name.c_str() ) ;
	return p ? std::string(p) : default_ ;
}

/// \file genvironment_unix.cpp
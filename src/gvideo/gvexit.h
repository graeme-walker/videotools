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
/// \file gvexit.h
///

#ifndef GV_EXIT__H
#define GV_EXIT__H

#include "gdef.h"

namespace Gv
{
	class Exit ;
}

/// \class Gv::Exit
/// A simple exception structure holding a program exit value. This class
/// is deliberately not derived from std::exception.
/// 
class Gv::Exit
{
public:
	explicit Exit( int exit_value = EXIT_SUCCESS ) ;
		///< Constructor.

	int value() const ;
		///< Returns the exit value, as given to the ctor.

private:
	int m_value ;
} ;

inline
Gv::Exit::Exit( int value ) :
	m_value(value)
{
}

inline
int Gv::Exit::value() const
{
	return m_value ;
}

#endif

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
/// \file gxerror.h
///

#ifndef GX_ERROR_H
#define GX_ERROR_H

#include "gdef.h"
#include "gxdef.h"
#include <exception>
#include <string>

namespace GX
{
	class Error ;
}

/// \class GX::Error
/// An exception class for GX classes.
/// 
class GX::Error : public std::exception
{
public:
	Error() ;
		///< Default constructor.

	explicit Error( const std::string & ) ;
		///< Constructor.

	~Error() g__noexcept ;
		///< Destructor.

	const char * what() const g__noexcept override ;
		///< From std::exception.

private:
	std::string m_error ;
} ;

#endif

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
// gxerror.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxerror.h"

GX::Error::Error() :
	m_error("gxlib error")
{
}

GX::Error::Error( const std::string & s ) :
	m_error(s)
{
}

GX::Error::~Error() g__noexcept
{
}

const char * GX::Error::what() const g__noexcept
{
	return m_error.c_str() ;
}

/// \file gxerror.cpp

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
/// \file gvcapturefactory.h
///

#ifndef GV_CAPTURE_FACTORY__H
#define GV_CAPTURE_FACTORY__H

#include "gdef.h"
#include "gvtimezone.h"
#include <string>

namespace Gv
{
	class Capture ;
	class CaptureFactory ;
}

/// \class Gv::CaptureFactory
/// A factory class for G::Capture.
/// 
class Gv::CaptureFactory
{
public:
	static Capture * create( const std::string & dev_name , const std::string & config , Gv::Timezone = Gv::Timezone() ) ;
		///< Factory method.

private:
	CaptureFactory() ;
} ;

#endif

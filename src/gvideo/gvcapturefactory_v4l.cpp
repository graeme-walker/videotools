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
// gvcapturefactory_v4l.cpp
//

#include "gdef.h"
#include "gvcapturefactory.h"
#include "gvcapture_test.h"
#include "gvbars.h"
#include "gvdemo.h"
#include "gvcapture_v4l.h"
#include "gstr.h"

Gv::Capture * Gv::CaptureFactory::create( const std::string & dev_name , const std::string & config , Gv::Timezone tz )
{
	if( dev_name == "demo" || ( dev_name == "test" && G::Str::splitMatch(config,"demo",";") ) )
		return new CaptureTest( config , new Demo(CaptureTest::dx(0),CaptureTest::dy(0),config,tz) ) ;
	else if( dev_name == "test" )
		return new CaptureTest( config , new Bars(CaptureTest::dx(0),CaptureTest::dy(0),config) ) ;
	else
		return new CaptureV4l( dev_name , config ) ;
}

/// \file gvcapturefactory_v4l.cpp

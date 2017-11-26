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
/// \file gvdemo.h
///

#ifndef GV_DEMO__H
#define GV_DEMO__H

#include "gdef.h"
#include "gvimagegenerator.h"
#include "gvtimezone.h"

namespace Gv
{
	class Demo ;
	class DemoImp ;
}

/// \class Gv::Demo
/// An image generator class for a demo animation.
/// 
class Gv::Demo : public ImageGenerator
{
public:
	Demo( int dx , int dy , const std::string & dev_config , Gv::Timezone ) ;
		///< Constructor.

	virtual ~Demo() ;
		///< Destructor.

	virtual bool init() override ;
		///< Override from Gv::ImageGenerator.

	virtual void fillBuffer( CaptureBuffer & , const CaptureBufferScale & ) override ;
		///< Override from Gv::ImageGenerator.

private:
	Demo( const Demo & ) ;
	void operator=( const Demo & ) ;
	void drawScene( CaptureBuffer & , const CaptureBufferScale & ) ;
	void drawItem( CaptureBuffer & , const CaptureBufferScale & , const char * , 
		size_t , size_t , int , int , bool first , size_t = 0U ) ;

private:
	DemoImp * m_imp ;
} ;

#endif

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
/// \file gvbars.h
///

#ifndef GV_BARS__H
#define GV_BARS__H

#include "gdef.h"
#include "gvimagegenerator.h"

namespace Gv
{
	class Bars ;
	class BarsImp ;
}

/// \class Gv::Bars
/// An image generator for test images comprising horizontal or vertical colour bars. 
/// Writes into a v4l capture buffer.
/// 
class Gv::Bars : public ImageGenerator
{
public:
	Bars( int dx , int dy , const std::string & config ) ;
		///< Constructor.

	virtual ~Bars() ;
		///< Destructor.

	virtual bool init() override ;
		///< Override from Gv::ImageGenerator.

	virtual void fillBuffer( CaptureBuffer & , const CaptureBufferScale & ) override ;
		///< Override from Gv::ImageGenerator.

private:
	Bars( const Bars & ) ;
	void operator=( const Bars & ) ;

private:
	BarsImp * m_imp ;
} ;

#endif

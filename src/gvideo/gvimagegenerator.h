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
/// \file gvimagegenerator.h
///

#ifndef GV_IMAGE_GENERATOR__H
#define GV_IMAGE_GENERATOR__H

#include "gdef.h"
#include "gvcapturebuffer.h"

namespace Gv
{
	class ImageGenerator ;
}

/// \class Gv::ImageGenerator
/// An abstract interface for filling a YUYV capture buffer with a generated image.
/// 
class Gv::ImageGenerator
{
public:
	virtual bool init() = 0 ;
		///< Initialisation function. Returns true for rgb, false for yuyv.

	virtual void fillBuffer( CaptureBuffer & , const CaptureBufferScale & ) = 0 ;
		///< Fills the given capture buffer with a generated image.
		///< The buffer will have been already imbued with the 
		///< relevant format.

	virtual ~ImageGenerator() ;
		///< Destructor.

private:
	void operator=( const ImageGenerator & ) ;
} ;

#endif

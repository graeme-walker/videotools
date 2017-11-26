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
/// \file grimagebuffer.h
///

#ifndef GR_IMAGE_BUFFER__H
#define GR_IMAGE_BUFFER__H

#include "gdef.h"
#include "grvectors.h"

namespace Gr
{
	/// An ImageBuffer is used to hold raw image data, typically in more than 
	/// one chunk. The (extrinsic) data format can be 'raw', jpeg, png, pnm, etc. 
	/// 
	/// In the 'raw' format each chunk contains exactly one row of image pixels 
	/// ('segmented'), or alternatively the first chunk contains the whole 
	/// image ('contiguous'). The Gr::ImageData class is normally used to wrap 
	/// image buffers that are in the raw format.
	/// 
	/// Free functions in the Gr::imagebuffer namespace are used for low-level 
	/// operations on the contrete class, and a traits class in the Gr::traits 
	/// namespace is used to define associated streambuf and iterator types.
	/// Standard streaming operators are also defined.
	/// 
	/// The current definition is a vector-of-vectors, but a vector of malloced
	/// buffers might be a good alternative. Classes affected by the choice of
	/// image buffer type include Gr::ImageData, GNet::SocketProtocol,
	/// G::Publisher and G::FatPipe.
	/// 
	typedef Vectors ImageBuffer ;
}

#endif

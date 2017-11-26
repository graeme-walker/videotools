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
/// \file grtraits.h
///

#ifndef GR_TRAITS__H
#define GR_TRAITS__H

namespace Gr
{
	namespace traits
	{
		/// \class Gr::traits::imagebuffer
		/// A traits class that can be specialised for Gr::ImageBuffer candidates.
		/// This allows the Gr::ImageBuffer type to be defined by a single
		/// typedef (see grimagebuffer.h).
		/// 
		template <typename T>
		struct imagebuffer
		{
		} ;
	}
}

#endif

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
/// \file grdef.h
///

#ifndef GR_DEF__H
#define GR_DEF__H

#include "gdef.h"
#include <algorithm>
#include <utility>

namespace Gr
{
	inline
	int scaled( int n , int scale )
	{
		return n > 0 ? ( scale == 1 ? n : ( 1 + (n-1)/std::max(1,scale) ) ) : 0 ;
	}

	inline
	size_t sizet( int a )
	{
		return a >= 0 ? static_cast<size_t>(a) : size_t(0U) ;
	}

	inline
	size_t sizet( int a , int b )
	{
		return sizet(a) * sizet(b) ;
	}

	inline
	size_t sizet( int a , int b , int c )
	{
		return sizet(a) * sizet(b) * sizet(c) ;
	}

	template <typename T>
	struct add_size
	{
		explicit add_size( size_t & n ) : m_n(n) {}
		void operator()( const T & a ) { m_n += a.size() ; }
		size_t & m_n ;
	} ;
}

#endif

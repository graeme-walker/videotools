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
/// \file gstaticassert.h
///

#ifndef G_STATIC_ASSERT_H
#define G_STATIC_ASSERT_H

#include "gdef.h"

#ifdef G_COMPILER_IS_GNU
#define G_STATIC_ASSERT( expr ) typedef char G_STATIC_ASSERT_JOIN(static_assertion_on_line_,__LINE__) [(expr)?1:-1] __attribute__((unused))
#else
#define G_STATIC_ASSERT( expr ) typedef char G_STATIC_ASSERT_JOIN(static_assertion_on_line_,__LINE__) [(expr)?1:-1]
#endif
#define G_STATIC_ASSERT_JOIN( p1 , p2 ) G_STATIC_ASSERT_JOIN_HELPER( p1 , p2 )
#define G_STATIC_ASSERT_JOIN_HELPER( p1 , p2 ) p1##p2

#endif

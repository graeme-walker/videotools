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
/// \file grcolourspacemap.h
///

#ifndef GR_COLOURSPACE_MAP__H
#define GR_COLOURSPACE_MAP__H

#include "gdef.h"

namespace Gr
{
namespace ColourSpace
{

/// Does min/max clamping.
/// 
template <typename T>
constexpr
T clamp( T min , T value , T max )
{
	return value < min ? min : ( value > max ? max : value ) ;
}

/// Does min/max clamping to a range.
/// 
template <typename Trange, typename Tout, typename Tin>
constexpr
Tout clamp( Tin value )
{
	return static_cast<Tout>(clamp(static_cast<Tin>(Trange::min),value,static_cast<Tin>(Trange::max))) ;
}

/// Scales an unsigned fp value to another fp value in the relevant range.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Tconverter::fp_type map_to_range_from_unsigned( typename Tconverter::fp_type fp ) g__noexcept
{
	typedef typename Tconverter::fp_type fp_t ;
	return ( fp * fp_t(Trange::max-Trange::min) + fp_t(Trange::min) ) ;
}

/// Scales a signed fp value to another fp value in the relevant range.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Tconverter::fp_type map_to_range_from_signed( typename Tconverter::fp_type fp ) g__noexcept
{
	return map_to_range_from_unsigned<Trange,Tconverter>( fp + Tconverter::make_half() ) ;
}

/// Scales an fp value to another fp value in the relevant range.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Tconverter::fp_type map_to_range_imp( typename Tconverter::fp_type fp ) g__noexcept
{
	return 
		Trange::is_signed ? 
			map_to_range_from_signed<Trange,Tconverter>(fp) : 
			map_to_range_from_unsigned<Trange,Tconverter>(fp) ;
}

/// Maps an "analogue" space value to a "digial" range value.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Trange::value_type map_to_range( typename Tconverter::fp_type fp ) g__noexcept
{
	typedef typename Tconverter::fp_type fp_t ;
	return Tconverter::to_range(
		clamp( fp_t(Trange::min) , map_to_range_imp<Trange,Tconverter>(fp) , fp_t(Trange::max) ) ) ;
}


/// Maps a "digital" range value to a signed "analogue" space value.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Tconverter::fp_type map_to_space_signed( typename Trange::value_type n ) g__noexcept
{
	typedef typename Tconverter::fp_type fp_t ;
	return fp_t(n-Trange::min) / fp_t(Trange::max-Trange::min) - Tconverter::make_half() ;
}

/// Maps a "digital" range value to an unsigned "analogue" space value.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Tconverter::fp_type map_to_space_unsigned( typename Trange::value_type n ) g__noexcept
{
	typedef typename Tconverter::fp_type fp_t ;
	return fp_t(n-Trange::min) / fp_t(Trange::max-Trange::min) ;
}

/// Maps a "digital" range value to an "analogue" space value.
/// 
template <typename Trange, typename Tconverter>
constexpr
typename Tconverter::fp_type map_to_space_imp( typename Trange::value_type n ) g__noexcept
{
	return 
		Trange::is_signed ?
			map_to_space_signed<Trange,Tconverter>( n ) :
			map_to_space_unsigned<Trange,Tconverter>( n ) ;
}

/// Maps a "digital" range triple to an "analogue" space triple.
/// 
template <typename Tconverter, typename Trange1, typename Trange2, typename Trange3>
constexpr
triple<typename Tconverter::fp_type> map_to_space( triple<typename Trange1::value_type> t ) g__noexcept
{
	typedef typename Tconverter::fp_type fp_t ;
	return triple<fp_t>( 
		map_to_space_imp<Trange1,Tconverter>(t.m_1) , 
		map_to_space_imp<Trange2,Tconverter>(t.m_2) , 
		map_to_space_imp<Trange3,Tconverter>(t.m_3) ) ;
}

}
}

#endif

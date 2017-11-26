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
/// \file grhistogram.h
///

#ifndef GR_HISTOGRAM__H
#define GR_HISTOGRAM__H

#include "gdef.h"
#include "gstaticassert.h"
#include <vector>
#include <algorithm>

namespace Gr
{
	class Histogram ;
}

/// \class Gr::Histogram
/// A class that does histogram equalisation for eight-bit samples. 
/// 
/// The implementation goes for speed over space by holding a complete 
/// map of pixel values rather than just the pixel values that appear 
/// in the image.
/// 
class Gr::Histogram
{
public:
	typedef unsigned long sum_type ;
	typedef unsigned char value_type ;
	enum { N = 256 } ;
	G_STATIC_ASSERT( sizeof(value_type) == 1 ) ;

	Histogram() ;
		///< Constructor.
		///< Postcondition: active()

	explicit Histogram( bool active ) ;
		///< Constructor, optionally for an unusable, zero-cost object.

	void clear() ;
		///< Clears the map.

	void add( value_type ) ;
		///< Adds a pixel to the histogram.
		///< Precondition: active()

	void compute() ;
		///< Computes the equalisation map once all the pixels
		///< have been add()ed.

	value_type map( value_type ) const ;
		///< Does the equalisation mapping.

	bool active() const ;
		///< Returns true if constructed as active.

private:
	typedef std::vector<sum_type> Vector ;
	Vector m_data ;

private:
	template <typename TValue,typename TSum>
	struct Accumulator
	{
		typedef TValue value_type ;
		typedef TSum sum_type ;
		sum_type & n ;
		Accumulator( sum_type & n_ ) : n(n_) {}
		void operator()( sum_type & v ) { n += v ; v = n ; }
	} ;
	template <typename TValue,typename TSum>
	struct Scaler
	{
		typedef TValue value_type ;
		typedef TSum sum_type ;
		sum_type min ;
		sum_type divisor ;
		Scaler( value_type min_ , sum_type total_ ) : min(min_) , divisor(total_-min_) {}
		void operator()( sum_type & v ) 
		{ 
			const sum_type n = ( v - min ) * ( N - 1 ) ;
			v = (value_type)( n / divisor ) ;
		}
	} ;
} ;

inline
Gr::Histogram::Histogram() :
	m_data(N)
{
}

inline
Gr::Histogram::Histogram( bool active )
{
	if( active )
		m_data.resize( N ) ;
}

inline
void Gr::Histogram::add( value_type n )
{
	m_data[n]++ ;
}

inline
void Gr::Histogram::clear()
{
	m_data.assign( N , 0 ) ;
}

inline
void Gr::Histogram::compute()
{
	const Vector::iterator end = m_data.end() ;
	Vector::iterator min_p = std::find_if( m_data.begin() , end , std::bind2nd(std::not_equal_to<value_type>(),0) ) ;
	if( min_p == end ) return ;
	sum_type n = 0 ;
	std::for_each( m_data.begin() , end , Accumulator<value_type,sum_type>(n) ) ;
	std::for_each( m_data.begin() , end , Scaler<value_type,sum_type>(*min_p,m_data.back()) ) ;
}

inline
Gr::Histogram::value_type Gr::Histogram::map( value_type n ) const
{
	return m_data[n] ;
}

inline
bool Gr::Histogram::active() const
{
	return !m_data.empty() ;
}

#endif

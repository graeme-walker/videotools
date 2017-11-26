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
/// \file grlinedraw.h
///

#ifndef GR_LINE_DRAW__H
#define GR_LINE_DRAW__H

#include "gdef.h"
#include <utility> // std::pair<>

namespace Gr
{
	template <typename T> class LineDrawImp ;
	template <typename T> class LineDraw ;
}

/// \class Gr::LineDrawImp
/// A low-level line-drawing class used in the implementation
/// of Gr::LineDraw.
/// 
/// The client code iterates over the points in the longest of
/// the two dimensions, without having to know which dimension 
/// is the longest or which direction they are going.
/// 
/// Usage:
/// \code
///   Gr::LineDrawImp<int> line( x1 , y1 , x2 , y2 ) ;
///   plot( line.x() , line.y() ) ;
///   for( int i = 0 ; i < line.n() ; i++ )
///   {
///     line.nextPoint() ;
///     plot( line.x() , line.y() ) ;
///   }
/// \endcode
/// 
template <typename T>
class Gr::LineDrawImp
{
public:
	LineDrawImp( T x1 , T y1 , T x2 , T y2 ) ;
		///< Constructor.

	T n() const ;
		///< Returns the iteration count.

	bool nextPoint() ;
		///< Steps to the next point for point-wise iteration.
		///< Returns false if (x(),y()) is now off the end.

	T x() const ;
		///< Returns the current x coordinate for point-wise iteration.

	T y() const ;
		///< Returns the current y coordinate for point-wise iteration.

	bool flat() const ;
		///< Returns true if the x dimension of the
		///< line is greater the the y dimension.

private:
	bool m_flat ;
	T m_dx ;
	T m_dy ;
	T m_big ;
	T m_small ;
	T m_dbig ;
	T m_dsmall ;
	T m_step_big ;
	T m_step_small ;
	T m_brmax ;
	T m_brc ;
	T m_big_off_end ;
} ;


/// \class Gr::LineDraw
/// A class template for drawing lines in terms of separate horizontal line 
/// segments.
/// Usage:
/// \code
/// LineDraw<int> line_scan( ... ) ;
/// while( line_scan.more() )
///   drawSegment( line_scan.x1() , line_scan.x2() , line_scan.y() ) ;
/// \endcode
/// 
template <typename T>
class Gr::LineDraw
{
public:
	LineDraw( T x1 , T y1 , T x2 , T y2 ) ;
		///< Constructor.

	bool more() ;
		///< Iterator for the next line segment. Returns true at least once.

	T x1() const ;
		///< Returns the current smaller x coordinate.

	T x2() const ;
		///< Returns the current larger x coordinate.

	T y() const ;
		///< Returns the current y coordinate.

	bool flat() const ;
		///< Returns true if the x dimension of the
		///< line is greater the the y dimension.

private:
	void assign() ;
	void iterate() ;

private:
	LineDrawImp<T> m_line ;
	T m_x1 ;
	T m_x2 ;
	T m_y ;
	bool m_horizontal ;
	bool m_first ;
	bool m_off_end ;
} ; 


template <typename T>
Gr::LineDrawImp<T>::LineDrawImp( T x1 , T y1 , T x2 , T y2 )
{
	m_dx = (x2 > x1) ? (x2 - x1) : (x1 - x2) ;
	m_dy = (y2 > y1) ? (y2 - y1) : (y1 - y2) ;
	m_flat = m_dx > m_dy ;
	m_big = m_flat ? x1 : y1 ;
	m_small = m_flat ? y1 : x1 ;
	m_step_big = m_flat ? ( x2 < x1 ? -1 : 1 ) : ( y2 < y1 ? -1 : 1 ) ;
	m_step_small = m_flat ? ( y2 < y1 ? -1 : 1 ) : ( x2 < x1 ? -1 : 1 ) ;
	m_dbig = m_flat ? m_dx : m_dy ;
	m_dsmall = m_flat ? m_dy : m_dx ;
	m_brmax = m_dbig ;
	m_brc = m_dbig / T(2) ;
	m_big_off_end = (m_flat ? x2 : y2) + m_step_big ;
}

template <typename T>
T Gr::LineDrawImp<T>::x() const
{
	return m_flat ? m_big : m_small ;
}

template <typename T>
T Gr::LineDrawImp<T>::y() const
{
	return m_flat ? m_small : m_big ;
}

template <typename T>
T Gr::LineDrawImp<T>::n() const
{
	return m_dbig ;
}

template <typename T>
bool Gr::LineDrawImp<T>::nextPoint()
{
	if( m_big != m_big_off_end ) 
	{
		m_big += m_step_big ;
		m_brc += m_dsmall ;
		if( m_brc > m_brmax )
		{
			m_brc -= m_dbig ;
			m_small += m_step_small ;
		}
	}
	return m_big != m_big_off_end ;
}

template <typename T>
bool Gr::LineDrawImp<T>::flat() const
{
	return m_flat ;
}


template <typename T>
Gr::LineDraw<T>::LineDraw( T x1 , T y1 , T x2 , T y2 ) :
	m_line( x1 , y1 , x2 , y2 ) ,
	m_x1( x1 ) ,
	m_x2( x2 ) ,
	m_y( y1 ) ,
	m_horizontal( y1 == y2 ) ,
	m_first( true ) ,
	m_off_end( false )
{
}

template <typename T>
void Gr::LineDraw<T>::assign()
{
	m_x1 = m_x2 = m_line.x() ;
	m_y = m_line.y() ;
}

template <typename T>
void Gr::LineDraw<T>::iterate()
{
	// walk along the line's larger dimension until the y value 
	// changes; set x2 as the x value just prior to that point
	bool ok = true ;
	do
	{
		m_x2 = m_line.x() ;
		ok = m_line.nextPoint() ;
	} while( ok && m_line.y() == m_y ) ;
	m_off_end = !ok ;
}

template <typename T>
bool Gr::LineDraw<T>::more()
{
	if( m_first )
	{
		m_first = false ;
		if( m_horizontal )
		{
			; // no-op -- set up in ctor
		}
		else if( m_line.flat() )
		{
			assign() ;
			iterate() ;
		}
		else
		{
			assign() ;
		}
		return true ;
	}
	else if( m_horizontal )
	{
		return false ;
	}
	else if( m_line.flat() && m_off_end )
	{
		return false ;
	}
	else if( m_line.flat() )
	{
		assign() ;
		iterate() ;
		return true ;
	}
	else
	{
		bool rc = m_line.nextPoint() ;
		assign() ;
		return rc ;
	}
}

template <typename T>
bool Gr::LineDraw<T>::flat() const
{
	return m_line.flat() ;
}

template <typename T>
T Gr::LineDraw<T>::x1() const
{
	return m_x1 < m_x2 ? m_x1 : m_x2 ;
}

template <typename T>
T Gr::LineDraw<T>::x2() const
{
	return m_x1 < m_x2 ? m_x2 : m_x1 ;
}

template <typename T>
T Gr::LineDraw<T>::y() const
{
	return m_y ;
}

#endif

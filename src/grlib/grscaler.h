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
/// \file grscaler.h
///

#ifndef GR_SCALER__H
#define GR_SCALER__H

#include "gdef.h"

namespace Gr
{
	class ScalerImp ;
	class Scaler ;
}

/// \class Gr::ScalerImp
/// A base class for Gr::Scaler that does scaling using the Bresenham 
/// algorithm, but limited to the first quadrant.
/// 
class Gr::ScalerImp
{
public:
	ScalerImp( int dfirst , int dsecond ) ;
		///< Constructor for iterating over the first range. The first dimension is 
		///< 'fast' since it increases on each iteration, and the second is 'slow' 
		///< since it increases conditionally.
		///< Precondition: dfirst >= dsecond

	operator bool() const ;
		///< Returns true if the current iteration position is still in range.

	bool operator!() const ;
		///< Returns false if the current iteration position is still in range.

	int a() const ;
		///< Returns the current value in the first range.

	int b() const ;
		///< Returns the calculated value in the second range.

	void operator++() ;
		///< Moves on to the next position in the first range.

private:
	int dfast ;
	int dslow ;
	int fast ;
	int slow ;
	int e ;
} ;

/// \class Gr::Scaler
/// A class that allows for iterating over one integer range while accessing values 
/// from another, with no requirement for one range to be a multiple of the other,
/// eg. for image scaling:
/// \code
///
/// for( Scaler y_scaler(display.dy(),image.dy()) ; !!y_scaler ; ++y_scaler )
/// {
///   for( Scaler x_scaler(display.dx(),image.dx()) ; !!x_scaler ; ++x_scaler )
///   {
///     int display_x = x_scaler.first() ;
///     int display_y = y_scaler.first() ;
///     int image_x = x_scaler.second() ;
///     int image_y = y_scaler.second() ;
///     ...
///   }
/// }
/// \endcode
/// 
/// Note that the iteration methods are in the base class.
/// 
class Gr::Scaler : public Gr::ScalerImp
{
public:
	Scaler( int dfirst , int dsecond ) ;
		///< Constructor for iterating over the first range while calculating 
		///< scaled values in the second.

	int first() const ;
		///< Returns the current value in the first range.
		///< Postcondition: first() >= 0 && first() < dfirst

	int second() const ;
		///< Returns the corresponding value in the second range.
		///< Postcondition: second() >= 0 && second() < dsecond

private:
	bool swap ;
} ;

inline Gr::ScalerImp::ScalerImp( int dfirst , int dsecond ) : dfast(dfirst) , dslow(dsecond) , fast(0) , slow(0) , e(2*dslow-dfast) {}
inline Gr::ScalerImp::operator bool() const { return fast < dfast ; }
inline bool Gr::ScalerImp::operator!() const { return fast >= dfast ; }
inline int Gr::ScalerImp::a() const { return fast ; }
inline int Gr::ScalerImp::b() const { return slow ; }
inline void Gr::ScalerImp::operator++() { fast++ ; if( e >= 0 ) { if((slow+1)<dslow) slow++ ; e -= dfast ; } e += dslow ; }

inline Gr::Scaler::Scaler( int dfirst , int dsecond ) : ScalerImp(dfirst>dsecond?dfirst:dsecond,dfirst>dsecond?dsecond:dfirst) , swap(!(dfirst>dsecond)) {}
inline int Gr::Scaler::first() const { return swap ? ScalerImp::b() : ScalerImp::a() ; }
inline int Gr::Scaler::second() const { return swap ? ScalerImp::a() : ScalerImp::b() ; }

#endif

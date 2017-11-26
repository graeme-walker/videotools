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
/// \file grpnm.h
///

#ifndef GR_PNM__H
#define GR_PNM__H

#include "gdef.h"
#include "gpath.h"
#include "grimagedata.h"
#include "grimagebuffer.h"
#include "gexception.h"
#include <istream>
#include <utility>
#include <vector>

namespace Gr
{
	class PnmInfo ;
	class PnmReader ;
}

/// \class Gr::PnmInfo
/// A structure holding portable-anymap metadata.
/// 
class Gr::PnmInfo
{ 
public:
	PnmInfo() ;
		///< Default constructor for an invalid structure.

	explicit PnmInfo( std::istream & ) ;
		///< Constructor reading from an istream.

	explicit PnmInfo( const std::vector<char> & ) ;
		///< Constructor reading from a buffer.
		///< 
		///< There is no limit on the header size (because of comments 
		///< and whitespace) so the whole image should be supplied.

	explicit PnmInfo( const ImageBuffer & ) ;
		///< Constructor reading from a Gr::ImageBuffer.

	PnmInfo( const char * , size_t ) ;
		///< Constructor overload for a char buffer. 

	PnmInfo( const unsigned char * , size_t ) ;
		///< Constructor overload for unsigned char.

	PnmInfo( int pn , int dx , int dy , int maxval , size_t offset ) ;
		///< Constructor taking the individual metadata items.

	bool valid() const ;
		///< Returns true if successfully constructed.

	bool binary() const ;
		///< Returns true if a binary format.

	bool binary8() const ;
		///< Returns true if an eight-bit binary format, ie. P5 or P6.

	int dx() const ;
		///< Returns the image width.

	int dy() const ;
		///< Returns the image height.

	int channels() const ;
		///< Returns the number of channels.

	int pn() const ;
		///< Returns the p-number.

	int maxval() const ;
		///< Returns the maximum value, or zero for bitmap formats (p1/p4).

	size_t offset() const ;
		///< Returns the size of the header.

	size_t rowsize() const ;
		///< Returns dx() * channels().

private:
	int m_pn ; ///< Px value, 1..6
	int m_dx ; 
	int m_dy ; 
	int m_maxval ;
	size_t m_offset ; ///< header size, ie. body offset
} ;

/// \class Gr::PnmReader
/// A static interface for reading portable-anymap (pnm) files.
/// 
class Gr::PnmReader
{
public:
	G_EXCEPTION( Error , "pnm image format error" ) ;

	explicit PnmReader( int scale = 1 , bool monochrome_out = false ) ;
		///< Constructor.

	void setup( int scale , bool monochrome_out = false ) ;
		///< Sets the decoding scale factor.

	void decode( ImageData & out , const G::Path & in ) ;
		///< Decodes a pnm file into an image. Throws on error.

	void decode( ImageData & out , std::istream & in ) ;
		///< Decodes a pnm stream into an image. Throws on error.

	void decode( ImageData & out , const char * p_in , size_t n_in ) ;
		///< Decodes a pnm buffer into an image. Throws on error.

	void decode( ImageData & out , const ImageBuffer & ) ;
		///< Decodes a pnm image buffer into an image. Throws on error.

	const PnmInfo & info() const ;
		///< Returns the pnm header info, ignoring scaling.

private:
	PnmReader( const PnmReader & ) ;
	void operator=( const PnmReader & ) ;
	static bool readBody( std::istream & , const PnmInfo & , ImageData & , int , bool ) ;

private:
	PnmInfo m_info ;
	int m_scale ;
	bool m_monochrome_out ;
} ;

inline Gr::PnmInfo::PnmInfo() : m_pn(0) , m_dx(0) , m_dy(0) , m_maxval(0) , m_offset(0U) {}
inline bool Gr::PnmInfo::binary() const { return m_pn >= 4 ; }
inline bool Gr::PnmInfo::binary8() const { return m_pn >= 5 ; }
inline int Gr::PnmInfo::dx() const { return m_dx ; }
inline int Gr::PnmInfo::dy() const { return m_dy ; }
inline int Gr::PnmInfo::channels() const { return ( m_pn == 3 || m_pn == 6 ) ? 3 : 1 ; }
inline int Gr::PnmInfo::pn() const { return m_pn ; }
inline int Gr::PnmInfo::maxval() const { return m_maxval ; }
inline size_t Gr::PnmInfo::offset() const { return m_offset ; }
inline bool Gr::PnmInfo::valid() const { return m_dx > 0 && m_dy > 0 ; }

inline const Gr::PnmInfo & Gr::PnmReader::info() const { return m_info ; }

#endif

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
//
// grpng_none.cpp
//

#include "grpng.h"
#include <stdexcept>

static const char * help = ": install libpng and re-run configure" ;
class Gr::PngReaderImp {} ;
class Gr::PngWriterImp {} ;
bool Gr::Png::available() { return false ; }
Gr::PngReader::PngReader( int , bool ) {}
Gr::PngReader::~PngReader() {}
void Gr::PngReader::setup( int , bool ) { throw Png::Error( "pngreader not implemented" + std::string(help) ) ; }
void Gr::PngReader::decode( ImageData & , const char * , size_t ) { throw Png::Error( "pngreader not implemented" + std::string(help) ) ; }
void Gr::PngReader::decode( ImageData & , const G::Path & ) { throw Png::Error( "pngreader not implemented" + std::string(help) ) ; }
void Gr::PngReader::decode( ImageData & , const ImageBuffer & ) { throw Png::Error( "pngreader not implemented" + std::string(help) ) ; }
Gr::PngReader::Map Gr::PngReader::tags() const { return Map() ; }
Gr::PngWriter::PngWriter( const ImageData & , Map ) { throw Png::Error( "pngwriter not implemented" + std::string(help) ) ; }
void Gr::PngWriter::write( const G::Path & ) {}
void Gr::PngWriter::write( Output & ) {}

void Gr::PngInfo::init( std::istream & stream )
{
	unsigned char buffer[31] = { 0 } ;
	stream.get( reinterpret_cast<char*>(buffer) , sizeof(buffer) ) ;
	std::pair<int,int> pair = parse( buffer , stream.gcount() ) ;
	m_dx = pair.first ;
	m_dy = pair.second ;
}

void Gr::PngInfo::init( const unsigned char * p , size_t n )
{
	std::pair<int,int> pair = parse( p , n ) ;
	m_dx = pair.first ;
	m_dy = pair.second ;
}

/// \file grpng_none.cpp

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
// grjpeg_none.cpp
//

#include "gdef.h"
#include "grjpeg.h"
#include <stdexcept>

static const char * help = ": install libjpeg and re-run configure" ;
bool Gr::Jpeg::available() { return false ; }
class Gr::JpegReaderImp {} ;
Gr::JpegReader::JpegReader( int , bool ) {}
Gr::JpegReader::~JpegReader() {}
void Gr::JpegReader::setup( int , bool ) { throw Jpeg::Error( "jpegreader not implemented" + std::string(help) ) ; }
void Gr::JpegReader::decode( ImageData & , const G::Path & ) { throw Jpeg::Error( "jpegreader not implemented" + std::string(help) ) ; }
void Gr::JpegReader::decode( ImageData & , const unsigned char * , size_t ) { throw Jpeg::Error( "jpegreader not implemented" + std::string(help) ) ; }
void Gr::JpegReader::decode( ImageData & , const char * , size_t ) { throw Jpeg::Error( "jpegreader not implemented" + std::string(help) ) ; }
void Gr::JpegReader::decode( ImageData & out , const ImageBuffer & ) { throw Jpeg::Error( "jpegreader not implemented" + std::string(help) ) ; }
;
//
class Gr::JpegWriterImp {} ;
Gr::JpegWriter::JpegWriter( int , bool ) {}
Gr::JpegWriter::~JpegWriter() {}
void Gr::JpegWriter::setup( int , bool ) { throw Jpeg::Error( "jpegwriter not implemented" ) ; }
void Gr::JpegWriter::encode( const ImageData & , const G::Path & ) {}
void Gr::JpegWriter::encode( const ImageData & , std::vector<char> & ) {}
void Gr::JpegWriter::encode( const ImageData & , ImageBuffer & ) {}
/// \file grjpeg_none.cpp

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
// gvavcreader_none.cpp
//

#include "gdef.h"
#include "gravc.h"
#include "gvavcreader.h"
#include <stdexcept>

Gv::AvcReaderStream::AvcReaderStream()
{
}

Gv::AvcReaderStream::AvcReaderStream( const Gr::Avc::Configuration & )
{
	throw std::runtime_error( "AvcReaderStream not implemented" ) ;
}

Gv::AvcReaderStream::~AvcReaderStream()
{
}

int Gv::AvcReaderStream::dx() const
{
	return 0 ;
}

int Gv::AvcReaderStream::dy() const
{
	return 0 ;
}

// ==

void Gv::AvcReader::init( const unsigned char * , size_t )
{
}

Gr::ImageType Gv::AvcReader::fill( std::vector<char> & , int , bool )
{
	return Gr::ImageType() ;
}

bool Gv::AvcReader::keyframe() const
{
	return false ;
}

bool Gv::AvcReader::available()
{
	return false ;
}
/// \file gvavcreader_none.cpp

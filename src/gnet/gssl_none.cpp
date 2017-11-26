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
// gssl_none.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gexception.h"
#include <utility>

GSsl::Library * GSsl::Library::m_this = nullptr ;

GSsl::Library::Library( const bool , const std::string & )
{
	if( m_this == nullptr )
		m_this = this ;
}

GSsl::Library::Library( bool , const std::string & , const std::string & , const std::string & )
{
	if( m_this == nullptr )
		m_this = this ;
}

GSsl::Library::~Library()
{
	if( this == m_this )
		m_this = nullptr ;
}

GSsl::Library * GSsl::Library::instance()
{
	return m_this ;
}

void GSsl::Library::add( const std::string & , const std::string & , const std::string & , const std::string & )
{
}

bool GSsl::Library::has( const std::string & ) const
{
	return false ;
}

const GSsl::Profile & GSsl::Library::profile( const std::string & ) const
{
	throw G::Exception( "internal error" ) ;
}

bool GSsl::Library::enabled() const
{
	return false ;
}

std::string GSsl::Library::id()
{
	return "none" ;
}

std::string GSsl::Library::credit( const std::string & , const std::string & , const std::string & )
{
	return std::string() ;
}

//

GSsl::Protocol::Protocol( const Profile & , LogFn )
{
}

GSsl::Protocol::~Protocol()
{
}

GSsl::Protocol::Result GSsl::Protocol::connect( G::ReadWrite & )
{
	return Result_error ;
}

GSsl::Protocol::Result GSsl::Protocol::accept( G::ReadWrite & )
{
	return Result_error ;
}

GSsl::Protocol::Result GSsl::Protocol::stop()
{
	return Result_error ;
}

GSsl::Protocol::Result GSsl::Protocol::read( char * , size_type , ssize_type & )
{
	return Result_error ;
}

GSsl::Protocol::Result GSsl::Protocol::write( const char * , size_type , ssize_type & )
{
	return Result_error ;
}

std::string GSsl::Protocol::str( Protocol::Result )
{
	return std::string() ;
}

std::pair<std::string,bool> GSsl::Protocol::peerCertificate( int format )
{
	return std::make_pair( std::string() , false ) ;
}

/// \file gssl_none.cpp

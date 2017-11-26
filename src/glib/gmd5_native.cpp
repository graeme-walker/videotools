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
// gmd5_native.cpp
//

#include "gdef.h"
#include "gmd5.h"
#include "gstr.h"
#include "gstrings.h"
#include "gassert.h"
#include "md5.h"
#include <sstream>

namespace
{
	typedef md5::big_t big_t ;
	typedef md5::digest_stream md5_state_t ;
	typedef md5::digest::state_type state_type ;
	typedef md5::format format ;

	void init( md5_state_t & )
	{
	}
	void update( md5_state_t & context , const std::string & input )
	{
		context.add( input ) ;
	}
	std::string finish( md5_state_t & context )
	{
		context.close() ;
		return format::raw( context.state().d ) ;
	}
	std::string writeOut( const md5_state_t & context )
	{
		G_ASSERT( context.state().s.length() == 0U ) ;
		G_ASSERT( context.state().n == 64U ) ; // ie. magic number below
		std::ostringstream ss ;
		ss <<
			context.state().d.a << "." <<
			context.state().d.b << "." <<
			context.state().d.c << "." <<
			context.state().d.d ;
		return ss.str() ;
	}
	big_t toUnsigned( const std::string & s , bool limited = true )
	{
		// big_t can be bigger than unsigned long but "long long" might not
		// be supported by the compiler, so do it old-school
		if( s.empty() ) throw G::Md5::Error( s ) ;
		big_t result = 0U ;
		for( std::string::const_iterator p = s.begin() ; p != s.end() ; ++p )
		{
			result *= 10U ;
			if( *p < '0' || *p > '9' ) throw G::Md5::Error( s ) ;
			const unsigned int n = static_cast<unsigned int>(*p) - static_cast<unsigned int>('0') ;
			if( limited && (result+n) < result ) throw G::Md5::Error( s ) ;
			result += n ;
		}
		return result ;
	}
	G::StringArray::iterator readIn( md5_state_t & context , G::StringArray::iterator p )
	{
		big_t a = toUnsigned( *p++ ) ;
		big_t b = toUnsigned( *p++ ) ;
		big_t c = toUnsigned( *p++ ) ;
		big_t d = toUnsigned( *p++ ) ;
		state_type state = { a , b , c , d } ;
		md5::small_t magic_number = 64U ;
		context = md5_state_t( state , magic_number ) ;
		return p ;
	}
}

std::string G::Md5::xor_( const std::string & s1 , const std::string & s2 )
{
	G_ASSERT( s1.length() == s2.length() ) ;
	std::string::const_iterator p1 = s1.begin() ;
	std::string::const_iterator p2 = s2.begin() ;
	std::string result ;
	result.reserve( s1.length() ) ;
	for( ; p1 != s1.end() ; ++p1 , ++p2 )
	{
		unsigned char c1 = static_cast<unsigned char>(*p1) ;
		unsigned char c2 = static_cast<unsigned char>(*p2) ;
		unsigned char c = static_cast<unsigned char>( c1 ^ c2 ) ;
		result.append( 1U , static_cast<char>(c) ) ;
	}
	return result ;
}

std::string G::Md5::key64( std::string k )
{
	const size_t B = 64U ;
	if( k.length() > B )
		k = digest(k) ;
	if( k.length() < B )
		k.append( std::string(B-k.length(),'\0') ) ;
	G_ASSERT( k.length() == B ) ;
	return k ;
}

std::string G::Md5::ipad()
{
	const size_t B = 64U ;
	return std::string( B , '\066' ) ; // 00110110 = 00,110,110
}

std::string G::Md5::opad()
{
	const size_t B = 64U ;
	return std::string( B , '\134' ) ; // 01011100 = 01,011,100
}

std::string G::Md5::mask( const std::string & k )
{
	std::string k64 = key64( k ) ;
	return mask( k64 , ipad() ) + "." + mask( k64 , opad() ) ;
}

std::string G::Md5::mask( const std::string & k64 , const std::string & pad )
{
	md5_state_t context ;
	init( context ) ;
	update( context , xor_(k64,pad) ) ;
	return writeOut( context ) ;
}

std::string G::Md5::hmac( const std::string & masked_key , const std::string & input , Masked )
{
	G::StringArray part_list ;
	part_list.reserve( 8U ) ;
	G::Str::splitIntoTokens( masked_key , part_list , "." ) ;
	if( part_list.size() != 8U )
		throw InvalidMaskedKey( masked_key ) ;

	md5_state_t inner_context ;
	md5_state_t outer_context ;
	readIn( outer_context , readIn( inner_context , part_list.begin() ) ) ;
	update( inner_context , input ) ;
	update( outer_context , finish(inner_context) ) ;
	return finish( outer_context ) ;
}

std::string G::Md5::hmac( const std::string & k , const std::string & input )
{
	std::string k64 = key64( k ) ;
	return digest( xor_(k64,opad()) , digest(xor_(k64,ipad()),input) ) ;
}

std::string G::Md5::digest( const std::string & input )
{
	return digest( input , nullptr ) ;
}

std::string G::Md5::digest( const std::string & input_1 , const std::string & input_2 )
{
	return digest( input_1 , &input_2 ) ;
}

std::string G::Md5::digest( const std::string & input_1 , const std::string * input_2 )
{
	md5_state_t context ;
	init( context ) ;
	update( context , input_1 ) ;
	if( input_2 != nullptr )
		update( context , *input_2 ) ;
	return finish( context ) ;
}

std::string G::Md5::printable( const std::string & input )
{
	G_ASSERT( input.length() == 16U ) ;

	std::string result ;
	const char * hex = "0123456789abcdef" ;
	const size_t n = input.length() ;
	for( size_t i = 0U ; i < n ; i++ )
	{
		unsigned char c = static_cast<unsigned char>(input.at(i)) ;
		result.append( 1U , hex[(c>>4U)&0x0F] ) ;
		result.append( 1U , hex[(c>>0U)&0x0F] ) ;
	}

	return result ;
}

/// \file gmd5_native.cpp

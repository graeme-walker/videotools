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
/// \file gmd5.h
///

#ifndef G_MD5_H
#define G_MD5_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class Md5 ;
}

/// \class G::Md5
/// MD5 message digest class.
/// 
class G::Md5
{
public:
	G_EXCEPTION( InvalidMaskedKey , "invalid md5 key" ) ;
	G_EXCEPTION( Error , "internal md5 error" ) ;
	struct Masked  /// An overload discriminator for G::Md5::hmac()
		{} ;

	static std::string digest( const std::string & input ) ;
		///< Creates an MD5 digest. The resulting string is not generally printable 
		///< and it may have embedded NULs.

	static std::string digest( const std::string & input_1 , const std::string & input_2 ) ;
		///< An overload which processes two input strings.

	static std::string printable( const std::string & input ) ;
		///< Converts a binary string into a printable form, using a 
		///< lowercase hexadecimal encoding. See also RFC2095.

	static std::string hmac( const std::string & key , const std::string & input ) ;
		///< Computes a Hashed Message Authentication Code using MD5 as the hash 
		///< function. This is typically for challenge-response authentication
		///< where the plaintext input is an arbitrary challenge string from the
		///< server that the client has to hmac() using their shared private key.
		///< 
		///< See also RFC2104 [HMAC-MD5].
		///< 
		///< For hash function H with block size B (64) and output size L (16), 
		///< using shared key SK:
		///< 
		///< \code
		///< K = large(SK) ? H(SK) : SK
		///< ipad = 0x36 repeated B times
		///< opad = 0x5C repeated B times
		///< HMAC = H( K XOR opad , H( K XOR ipad , plaintext ) )
		///< \endcode
		///< 
		///< The H() function processes a stream of blocks; the first parameter
		///< above represents the first block, and the second parameter is the 
		///< rest of the stream (zero-padded up to a block boundary).
		///< 
		///< The shared key can be up to B bytes, or if more than B bytes then 
		///< K is the L-byte result of hashing the shared-key. K is zero-padded 
		///< up to B bytes for XOR-ing.

	static std::string hmac( const std::string & masked_key , const std::string & input , Masked ) ;
		///< An hmac() overload using a masked key. 
		///< 
		///< A masked key (MK) is the result of doing the initial, plaintext-independent 
		///< parts of HMAC computation, taking the intermediate results of both the
		///< inner and outer hash functions.
		///< 
		///< \code
		///< K = large(SK) ? H(SK) : SK
		///< HKipad = H( K XOR ipad , )
		///< HKopad = H( K XOR opad , )
		///< MK := ( HKipad , HKopad )
		///< \endcode

	static std::string mask( const std::string & key ) ;
		///< Computes a masked key for hmac() from the given shared key, returning
		///< a printable string.

private:
	static std::string digest( const std::string & input_1 , const std::string * input_2 ) ;
	static std::string mask( const std::string & k64 , const std::string & pad ) ;
	static std::string xor_( const std::string & , const std::string & ) ;
	static std::string key64( std::string ) ;
	static std::string ipad() ;
	static std::string opad() ;
	Md5() ;
} ;

#endif

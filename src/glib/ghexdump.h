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
/// \file ghexdump.h
///
/// Synopsis:
/// \code
/// G::hex_dump<16>( std::cout , data.begin() , data.end() ) ; // hex_dump
/// std::cout << G::hexdump<16>( data.begin() , data.end() ) ; // hexdump
/// \endcode
/// 

#ifndef G_HEXDUMP__H
#define G_HEXDUMP__H

#include "gdef.h"
#include <iostream>
#include <cstddef>
#include <iomanip>

namespace G
{
	/// \namespace G::HexdumpImp
	/// A private scope used for the implementation details of G::hexdump and G::hex_dump.
	/// 
	namespace HexdumpImp
	{
		struct hexdump_tohex /// Nibble-to-hex-digit functor.
		{
			char operator()( unsigned int n ) { return "0123456789abcdef"[n] ; }
		} ;

		struct hexdump_toprintable /// Char-to-printable-ascii functor.
		{
			char operator()( char c ) 
			{ 
				// allow only seven-bit ascii to avoid upsetting utf-8 xterms etc.
				const unsigned int n = static_cast<unsigned char>(c) ;
				return n >= 32U && n < 127U ? c : '.' ;
			}
		} ;

		template <unsigned N, typename Tin, typename Temit, typename Ttohex, typename Ttoprintable>
		void hexdump_imp( Tin begin , Tin end , Temit & emitter , unsigned int width , Ttohex tohex , Ttoprintable toprintable )
		{
			width = width < N ? width : N ;

			char hexbuffer[N*3+2] ; // also holds last-line padding after the hex nul terminator
			hexbuffer[N*3+1] = '\0' ;
			char * const hexbuffer_end = hexbuffer+width*3 ;
			*hexbuffer_end = '\0' ;
			char * hex = hexbuffer ;

			char printablebuffer[N+1] ;
			char * const printablebuffer_end = printablebuffer + width ;
			*printablebuffer_end = '\0' ;
			char * printable = printablebuffer ;

			std::size_t line_number  = 0U ;
			std::size_t address  = 0U ;
			for( Tin p = begin ; p != end ; ++p )
			{
				const unsigned int c = static_cast<unsigned char>(*p) ;
				*hex++ = ' ' ;
				*hex++ = tohex((c>>4)&0xf) ;
				*hex++ = tohex(c&0xf) ;
				*printable++ = toprintable(*p) ;
				if( printable == printablebuffer_end )
				{
					emitter.emit( address , line_number , width , hexbuffer+1 , hexbuffer_end , printablebuffer ) ;
					line_number++ ;
					hex = hexbuffer ;
					printable = printablebuffer ;
					address += width ;
				}
			}
			if( printable != printablebuffer )
			{
				*hex++ = '\0' ;
				for( char * p = hex ; p != hexbuffer_end ; ++p ) *p = ' ' ;
				*hexbuffer_end = ' ' ;
				*printable = '\0' ;
				emitter.emit( address , line_number , printable-printablebuffer , hexbuffer+1 , hex , printablebuffer ) ;
			}
		}

		struct hexdump_ostream_emitter /// An output adaptor to write to std::ostream.
		{
			explicit hexdump_ostream_emitter( std::ostream & out , unsigned int address_width , const char * prefix , const char * space , const char * bar ) :
				m_out(out) ,
				m_address_width(address_width) ,
				m_prefix(prefix) ,
				m_space(space) ,
				m_bar(bar)
			{
			}
			void emit( std::size_t address , std::size_t line_number , std::size_t /*line_length*/ , const char * hex , const char * padding , const char * printable )
			{
				m_out
					<< (line_number?"\n":"")
					<< m_prefix
					<< std::setfill('0') << std::setw(static_cast<int>(m_address_width)) << std::hex << address << std::dec
					<< m_space << hex 
					// (padding is always empty except for the last line; if there is padding on the first line
					// then dont use it because a single line with lots of padding can look silly)
					<< (line_number?padding:"") 
					<< m_bar << printable ;
			}
			std::ostream & m_out ;
			unsigned int m_address_width ;
			const char * m_prefix ;
			const char * m_space ;
			const char * m_bar ;
		} ;

		template <unsigned N,typename T,typename Ttohex,typename Ttoprintable>
		struct hexdump_streamable /// A streamable class used by G::hexdump().
		{
			hexdump_streamable( T begin , T end , int w , 
				const char * prefix , const char * space , const char * bar , 
				unsigned int width ,
				Ttohex tohex , Ttoprintable toprintable ) :
					m_begin(begin) ,
					m_end(end) ,
					m_w(w) ,
					m_prefix(prefix) ,
					m_space(space) ,
					m_bar(bar) ,
					m_width(width) ,
					m_tohex(tohex) ,
					m_toprintable(toprintable)
			{
			}
			T m_begin ;
			T m_end ;
			int m_w ;
			const char * m_prefix ;
			const char * m_space ;
			const char * m_bar ;
			unsigned int m_width ;
			Ttohex m_tohex ;
			Ttoprintable m_toprintable ;
		} ;

		template <unsigned N,typename T,typename Ttohex,typename Ttoprintable>
		std::ostream & operator<<( std::ostream & stream , const hexdump_streamable<N,T,Ttohex,Ttoprintable> & hd )
		{
			hexdump_ostream_emitter emitter( stream , hd.m_w , hd.m_prefix , hd.m_space , hd.m_bar ) ;
			hexdump_imp<N>( hd.m_begin , hd.m_end , emitter , hd.m_width , hd.m_tohex , hd.m_toprintable ) ;
			return stream ;
		}
	}

	namespace imp = HexdumpImp ;

	/// Performs a hex dump to the given stream.
	/// Eg.
	/// \code
	/// G::hex_dump<16>( std::cout , data.begin() , data.end() ) ; 
	/// \endcode
	/// 
	template <int N,typename T,typename Ttohex,typename Ttoprintable>
	void 
	hex_dump( std::ostream & out , T begin , T end , 
		unsigned int address_width , 
		const char * prefix , const char * space , const char * bar ,
		unsigned int width ,
		Ttohex tohex , Ttoprintable toprintable )
	{
		imp::hexdump_ostream_emitter emitter( out , address_width , prefix , space , bar ) ;
		imp::hexdump_imp<N>( begin , end , emitter , width , tohex , toprintable ) ;
		out << '\n' ;
	}

	///< defaulting overloads...

	template <int N,typename T> void 
	hex_dump( std::ostream & out , T begin , T end , 
		unsigned int address_width , 
		const char * prefix , const char * space , const char * bar ,
		unsigned int width )
	{ 
		hex_dump<N,T,typename imp::hexdump_tohex,typename imp::hexdump_toprintable>( out , begin , end , 
			address_width , prefix , space , bar , width , 
			imp::hexdump_tohex() , imp::hexdump_toprintable() ) ; 
	}

	template <int N,typename T> void 
	hex_dump( std::ostream & out , T begin , T end , 
		unsigned int address_width , 
		const char * prefix , const char * space , const char * bar )
	{ 
		hex_dump<N,T>( out , begin , end , 
			address_width , prefix , space , bar , 
			N ) ;
	}

	template <int N,typename T> void 
	hex_dump( std::ostream & out , T begin , T end , 
		unsigned int address_width )
	{ 
		hex_dump<N,T>( out , begin , end , address_width , "" , "  " , " | " ) ;
	}

	template <int N,typename T> void 
	hex_dump( std::ostream & out , T begin , T end )
	{ 
		hex_dump<N,T>( out , begin , end , 6 ) ;
	}

	///< convenience overloads...

	template <int N,typename T> void 
	hex_dump( std::ostream & out , const T & str )
	{ 
		hex_dump<N,typename T::const_iterator>( out , str.begin() , str.end() ) ;
	}

	/// Returns a streamable object that does a hex dump.
	/// Eg.
	/// \code
	///  std::cout << G::hexdump<16>(data.begin(),data.end()) << std::endl ;
	/// \endcode
	/// 
	template <int N,typename T,typename Ttohex,typename Ttoprintable> imp::hexdump_streamable<N,T,Ttohex,Ttoprintable> 
	hexdump( T begin , T end , 
		unsigned int address_width , 
		const char * prefix , const char * space , const char * bar ,
		unsigned int width ,
		Ttohex tohex , Ttoprintable toprintable )
	{
		return imp::hexdump_streamable<N,T,Ttohex,Ttoprintable>( begin , end , address_width , prefix , space , bar , width , tohex , toprintable ) ;
	}

	///< defaulting overloads...

	template <int N,typename T> imp::hexdump_streamable<N,T,typename imp::hexdump_tohex,typename imp::hexdump_toprintable> 
	hexdump( T begin , T end ,
		unsigned int address_width , 
		const char * prefix , const char * space , const char * bar ,
		unsigned int width )
	{ 
		return hexdump<N>( begin , end , address_width , prefix , space , bar , width , 
			imp::hexdump_tohex() , imp::hexdump_toprintable() ) ; 
	}

	template <int N,typename T> imp::hexdump_streamable<N,T,typename imp::hexdump_tohex,typename imp::hexdump_toprintable> 
	hexdump( T begin , T end ,
		unsigned int address_width , 
		const char * prefix , const char * space , const char * bar )
	{ 
		return hexdump<N>( begin , end , address_width , prefix , space , bar , N ) ;
	}

	template <int N,typename T> imp::hexdump_streamable<N,T,typename imp::hexdump_tohex,typename imp::hexdump_toprintable> 
	hexdump( T begin , T end ,
		unsigned int address_width )
	{ 
		return hexdump<N>( begin , end , address_width , "" , "  " , " | " ) ;
	}

	template <int N,typename T> imp::hexdump_streamable<N,T,typename imp::hexdump_tohex,typename imp::hexdump_toprintable> 
	hexdump( T begin , T end )
	{ 
		return hexdump<N>( begin , end , 6 ) ;
	}

	///< convenience overloads...

	template <int N, typename T> imp::hexdump_streamable<N,typename T::const_iterator,typename imp::hexdump_tohex, typename imp::hexdump_toprintable> 
	hexdump( const T & seq )
	{
		return hexdump<N,typename T::const_iterator>( seq.begin() , seq.end() ) ;
	}
}

#endif

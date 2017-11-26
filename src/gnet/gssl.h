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
/// \file gssl.h
///
/// An interface to an underlying TLS library.
/// 

#ifndef G_SSL_H
#define G_SSL_H

#include "gdef.h"
#include "greadwrite.h"
#include <string>
#include <utility>

namespace GSsl
{
	class Library ;
	class Profile ;
	class Protocol ;
	class LibraryImp ;
	class ProtocolImp ;
}

/// \class GSsl::Protocol
/// A TLS protocol class. A protocol object should be constructed for each 
/// secure socket. The Protocol::connect() and Protocol::accept() methods 
/// are used to link the connection's i/o methods with the Protocol object. 
/// Event handling for the connection is performed by the client code 
/// according to the result codes from read(), write(), connect() and 
/// accept().
/// 
/// Client code will generally need separate states to reflect an incomplete 
/// read(), write(), connect() or accept() in order that they can be retried. 
/// The distinction between a return code of Result_read or Result_write 
/// should dictate whether the connection is put into the event loop's read 
/// list or write list but it should not influence the resulting state; in 
/// each state socket read events and write events can be handled identically, 
/// by retrying the incomplete function call.
/// 
/// The protocol is half-duplex in the sense that it is not possible to read() 
/// data while a write() is incomplete or write() data while a read() is 
/// incomplete. (Nor is it allowed to issue a second call while the first is 
/// still incomplete.) 
/// 
/// All logging is done indirectly through a logging function pointer; the 
/// first parameter is the logging level which is 1 for verbose debug 
/// messages, 2 for useful information, and 3 for warnings and errors.
/// 
class GSsl::Protocol
{
public:
	typedef size_t size_type ;
	typedef ssize_t ssize_type ;
	enum Result { Result_ok , Result_read , Result_write , Result_error , Result_more } ;
	typedef void (*LogFn)( int , const std::string & ) ;

	explicit Protocol( const Profile & , LogFn log_fn ) ;
		///< Constructor.

	~Protocol() ;
		///< Destructor.

	Result connect( G::ReadWrite & io ) ;
		///< Starts the protocol actively (as a client).

	Result accept( G::ReadWrite & io ) ;
		///< Starts the protocol passively (as a server).

	Result stop() ;
		///< Initiates the protocol shutdown.

	Result read( char * buffer , size_type buffer_size_in , ssize_type & data_size_out ) ;
		///< Reads user data into the supplied buffer. 
		///< 
		///< Returns Result_read if there is not enough transport data 
		///< to complete the internal TLS data packet. In this case the 
		///< file descriptor should remain in the select() read list and 
		///< the Protocol::read() should be retried using the same parameters
		///< once the file descriptor is ready to be read.
		///< 
		///< Returns Result_write if the TLS layer tried to write to the
		///< file descriptor and had flow control asserted. In this case
		///< the file descriptor should be added to the select() write
		///< list and the Protocol::read() should be retried using the
		///< same parameters once the file descriptor is ready to be 
		///< written.
		///< 
		///< Returns Result_ok if the internal TLS data packet is complete
		///< and it has been completely deposited in the supplied buffer.
		///< 
		///< Returns Result_more if the internal TLS data packet is complete
		///< and the supplied buffer was too small to take it all. In this
		///< case there will be no read event to trigger more read()s, so
		///< beware of stalling.
		///< 
		///< Returns Result_error if the transport connnection was lost 
		///< or if the TLS session was shut down by the peer or on error.

	Result write( const char * buffer , size_type data_size_in , ssize_type & data_size_out ) ;
		///< Writes user data.
		///< 
		///< Returns Result_ok if fully sent.
		///< 
		///< Returns Result_read if the TLS layer needs more transport
		///< data (eg. for a renegotiation). The write() should be repeated
		///< using the same parameters on the file descriptor's next 
		///< readable event.
		///< 
		///< Returns Result_write if the TLS layer was blocked in 
		///< writing transport data. The write() should be repeated
		///< using the same parameters on the file descriptor's next
		///< writable event.
		///< 
		///< Never returns Result_more.
		///< 
		///< Returns Result_error if the transport connnection was lost 
		///< or if the TLS session was shut down by the peer or on error.

	static std::string str( Result result ) ;
		///< Converts a result enumeration into a printable string. 
		///< Used in logging and diagnostics.

	std::pair<std::string,bool> peerCertificate( int format = 0 ) ;
		///< Returns the peer certificate and a verified flag.
		///< The default format of the certificate is printable 
		///< with embedded newlines. Depending on the implementation
		///< it may be in PEM format, which can be interpreted using
		///< (eg.) "openssl x509 -in foo.pem -noout -text".

private:
	Protocol( const Protocol & ) ; // not implemented
	void operator=( const Protocol & ) ; // not implemented

private:
	ProtocolImp * m_imp ;
} ;

/// \class GSsl::Library
/// A singleton class for initialising the underlying TLS library.
/// 
class GSsl::Library
{
public:
	explicit Library( bool active = true , const std::string & extra_config = std::string() ) ;
		///< Constructor. The 'active' parameter can be set to false as an 
		///< optimisation if the library is not going to be used; calls to
		///< add() will do nothing, calls to has() will return false, and calls 
		///< to profile() will throw.

	explicit Library( bool active , const std::string & server_key_and_cert_file ,
		const std::string & server_ca_file , const std::string & extra_config = std::string() ) ;
			///< A convenience constructor that adds two profiles; one called
			///< "server" using the two files, and one called "client".

	~Library() ;
		///< Destructor. Cleans up the underlying TLS library.

	static Library * instance() ;
		///< Returns a pointer to a library object, if any.

	void add( const std::string & profile_name , const std::string & key_and_cert_file = std::string() ,
		const std::string & ca_file = std::string() , const std::string & profile_extra_config = std::string() ) ;
			///< Creates a named Profile object that can be retrieved by profile().
			///< 
			///< A typical application will have two profiles named "client" and "server".
			///< 
			///< The "key-and-cert-file" parameter points to a file containing our own 
			///< key and certificate. This is mandatory if acting as a server, ie. using 
			///< Protocol::accept(). 
			///< 
			///< The "ca-file" parameter points to a file (or possibly a directory) 
			///< containing a list of CA certificates. If this is supplied then peer 
			///< certificates will be requested and verified. Special values of
			///< "<none>" and "<default>" have the obvious meaning. A "client" profile
			///< will normally use "<default>" so that it can verify the server.
			///< 
			///< The profile-extra-config string should be empty by default; the format 
			///< and interpretation are undefined at this interface. An extra-config 
			///< string can also be supplied to the Protocol, in which case this config 
			///< string is completely ignored.

	bool has( const std::string & profile_name ) const ;
		///< Returns true if the named profile has been add()ed.

	const Profile & profile( const std::string & profile_name ) const ;
		///< Returns an opaque reference to the named profile. The profile
		///< can be used to construct a protocol instance.

	bool enabled() const ;
		///< Returns true if this is a real TLS library and the constructor's active
		///< parameter was set.

	static std::string id() ;
		///< Returns a library identifier (typically name and version).

	static std::string credit( const std::string & prefix , const std::string & eol , const std::string & eot ) ;
		///< Returns a library credit string.

private:
	friend class GSsl::Profile ;
	Library( const Library & ) ; // not implemented
	void operator=( const Library & ) ; // not implemented
	const LibraryImp & imp() const ;
	LibraryImp & imp() ;

private:
	static Library * m_this ;
	LibraryImp * m_imp ;
} ;

#endif

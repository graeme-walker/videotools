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
/// \file gresolver.h
///

#ifndef G_RESOLVER_H
#define G_RESOLVER_H

#include "gdef.h"
#include "gnet.h"
#include "glocation.h"
#include "geventhandler.h"
#include "gexception.h"
#include "gaddress.h"
#include <vector>

namespace GNet
{
	class Resolver ;
	class ResolverImp ;
}

/// \class GNet::Resolver
/// A class for synchronous or asynchronous network name to address resolution. 
/// The implementation uses getaddrinfo() at its core. Consider using 
/// GNet::ResolverService rather than this class.
/// 
class GNet::Resolver
{
public:
	typedef std::vector<Address> AddressList ;
	G_EXCEPTION( Error , "asynchronous resolver error" ) ;
	G_EXCEPTION( BusyError , "asynchronous resolver still busy" ) ;
	struct Callback : public EventExceptionHandler /// An interface used for GNet::Resolver callbacks.
	{
		virtual ~Callback() ;
		virtual void onResolved( std::string error , Location ) = 0 ;
			///< Called on completion of GNet::Resolver name resolution.
	} ;

	explicit Resolver( Callback & ) ;
		///< Constructor taking a callback interface reference.
		///< The callback's onException() method is called if an exception is 
		///< thrown out of Callback::onResolved() -- see GNet::HeapClient.

	~Resolver() ;
		///< Destructor.

	void start( const Location & ) ;
		///< Starts asynchronous name-to-address resolution.
		///< Precondition: async() && !busy()

	static std::string resolve( Location & ) ;
		///< Does syncronous name resolution. Fills in the
		///< name and address fields of the supplied Location 
		///< structure. The returned error string is zero length 
		///< on success.

	static AddressList resolve( const std::string & host , const std::string & service , int family = AF_UNSPEC , bool dgram = false ) ;
		///< Does synchronous name resolution returning a list
		///< of addresses. Errors are not reported. The empty
		///< list is returned on error.

	static bool async() ;
		///< Returns true if the resolver supports asynchronous operation.
		///< If it doesnt then start() will always throw.

	bool busy() const ; 
		///< Returns true if there is a pending resolve request.

private:
	void operator=( const Resolver & ) ; // not implemented
	Resolver( const Resolver & ) ; // not implemented
	friend class GNet::ResolverImp ;
	void done( const std::string & , const Location & ) ;

private:
	Callback & m_callback ;
	bool m_busy ;
	unique_ptr<ResolverImp> m_imp ;
} ;

#endif

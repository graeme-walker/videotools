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
/// \file glocal.h
///

#ifndef G_GNET_LOCAL_H
#define G_GNET_LOCAL_H

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "glocation.h"
#include "gexception.h"

namespace GNet
{
	class Local ;
}

/// \class GNet::Local
/// A static class for getting information about the local machine's network 
/// name and address.
/// 
class GNet::Local
{
public:
	static std::string hostname() ;
		///< Returns the local hostname. 

	static std::string canonicalName() ;
		///< Returns the canonical network name assiciated with hostname().

	static void canonicalName( const std::string & override ) ;
		///< Sets the canonicalName() override.

	static std::string name() ;
		///< Returns canonicalName(), or hostname() if canonicalName()
		///< is the empty string.

	static bool isLocal( const Address & , std::string & reason ) ;
		///< Returns true if the given address appears to be associated
		///< with the local host, or a helpful error message if not.

private:
	Local() ;
	static std::string resolvedHostname() ;

private:
	static std::string m_name_override ;
	static bool m_name_override_set ;
} ;

#endif

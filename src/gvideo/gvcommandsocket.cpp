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
// gvcommandsocket.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gvcommandsocket.h"
#include "geventhandler.h"
#include "gcleanup.h"
#include "gstr.h"
#include "groot.h"
#include "gfile.h"
#include "gresolver.h"
#include "glimits.h"
#include "glocation.h"
#include "gassert.h"

namespace
{
	char * strdup_ignore_leak( const char * p )
	{
		return ::strdup( p ) ;
	}
}

Gv::CommandSocket::CommandSocket()
{
}

Gv::CommandSocket::CommandSocket( const std::string & bind_name )
{
	if( bind_name.empty() )
	{
		G_ASSERT( fd() == -1 ) ;
	}
	else
	{
		Type type = parse( bind_name ) ;
		if( type.net )
		{
			GNet::Location location( type.path ) ;
			std::string e = GNet::Resolver::resolve( location ) ;
			if( !e.empty() ) throw std::runtime_error( e ) ;
			m_net.reset( new GNet::DatagramSocket(location.address().domain()) ) ;
			{
				G::Root claim_root ;
				m_net->bind( location.address() ) ;
			}
		}
		else
		{
			{
				G::Root claim_root ;
				G::File::remove( type.path , G::File::NoThrow() ) ;
			}
			m_local.reset( new G::LocalSocket(true) ) ;
			{
				G::Root claim_root ;
				m_local->bind( type.path ) ;
			}
			m_local->nonblock() ;
			G::Cleanup::add( CommandSocket::cleanup , strdup_ignore_leak(type.path.c_str()) ) ;
		}
	}
}

void Gv::CommandSocket::cleanup( G::SignalSafe signal_safe , const char * name )
{
	G::Identity id = G::Root::start( signal_safe ) ;
	std::remove( name ) ;
	G::Root::stop( signal_safe , id ) ;
}

void Gv::CommandSocket::connect( const std::string & connect_name )
{
	Type type = parse( connect_name ) ;
	if( type.net )
	{
		GNet::Location location( type.path ) ;
		std::string e = GNet::Resolver::resolve( location ) ;
		if( !e.empty() ) throw std::runtime_error( e ) ;
		m_net.reset( new GNet::DatagramSocket(location.address().domain()) ) ;
		{
			G::Root claim_root ;
			m_net->connect( location.address() ) ;
		}
	}
	else
	{
		m_local.reset( new G::LocalSocket(true) ) ;
		{
			G::Root claim_root ;
			m_local->connect( type.path ) ;
		}
		m_local->nonblock() ;
	}
}

std::string Gv::CommandSocket::connect( const std::string & connect_name , Gv::CommandSocket::NoThrow )
{
	try
	{
		connect( connect_name ) ;
		return std::string() ;
	}
	catch( std::exception & e )
	{
		return e.what() ;
	}
}

void Gv::CommandSocket::close()
{
	m_local.reset() ;
	m_net.reset() ;
}

Gv::CommandSocket::Type Gv::CommandSocket::parse( const std::string & name )
{
	// heuristics to decide whether name is a local-domain path or ip transport address

	Type type ;
	type.net = true ;
	type.path = name ;
	type.port = 0U ;

	size_t pos = name.rfind(':') ;
	std::string tail = G::Str::tail( name , pos , "" ) ;
	if( !tail.empty() && G::Str::isUInt(tail) ) 
		type.port = G::Str::toUInt(tail) ;

	if( type.port && name.find("udp://") == 0U ) 
	{
		type.path = name.substr(6U) ;
	}
	else if( type.port && pos > 3U && name.find("udp:") == 0U ) 
	{
		type.path = name.substr(4U) ;
	}
	else if( name.find("unix:") == 0U )
	{
		type.net = false ;
		type.path = name.substr(5U) ;
	}
	else if( type.port == 0U || name.find('/') != std::string::npos ) 
	{
		type.net = false ;
	}
	return type ;
}

int Gv::CommandSocket::fd() const
{
	if( m_local.get() ) return m_local->fd() ;
	if( m_net.get() ) return m_net->fd() ;
	return -1 ;
}

std::string Gv::CommandSocket::read()
{
	std::string result ;
	m_buffer.resize( G::limits::pipe_buffer ) ;
	ssize_t n = ::read( fd() , &m_buffer[0] , m_buffer.size() ) ;
	if( n > 0 )
	{
		result.assign( &m_buffer[0] , n ) ;
		G::Str::trim( result , G::Str::ws() ) ;
	}
	return result ;
}

// ==

/// \class Gv::CommandSocketMixinImp
/// A pimple-pattern implementation class for Gv::CommandSocketMixin.
///
class Gv::CommandSocketMixinImp : private GNet::EventHandler
{
public:
	CommandSocketMixinImp( CommandSocketMixin & , const std::string & ) ;
	virtual ~CommandSocketMixinImp() ;

private:
	virtual void readEvent() override ;
	virtual void onException( std::exception & ) override ;

private:
	CommandSocketMixin & m_outer ;
	CommandSocket m_command_socket ;
	int m_fd ;
} ;

Gv::CommandSocketMixinImp::CommandSocketMixinImp( CommandSocketMixin & outer , const std::string & bind_name ) :
	m_outer(outer) ,
	m_command_socket(bind_name) ,
	m_fd(m_command_socket.fd())
{
	if( m_fd != -1 )
		GNet::EventLoop::instance().addRead( GNet::Descriptor(m_fd) , *this ) ;
}

Gv::CommandSocketMixinImp::~CommandSocketMixinImp()
{
	if( m_fd != -1 )
		GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_fd) ) ;
}

void Gv::CommandSocketMixinImp::readEvent()
{
	m_outer.onCommandSocketData( m_command_socket.read() ) ;
}

void Gv::CommandSocketMixinImp::onException( std::exception & )
{
	throw ;
	//if( m_fd != -1 )
	//{
		//GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_fd) ) ;
		//m_fd = -1 ;
	//}
}

Gv::CommandSocketMixin::CommandSocketMixin( const std::string & bind_name ) :
	m_imp(new CommandSocketMixinImp(*this,bind_name))
{
}

Gv::CommandSocketMixin::~CommandSocketMixin()
{
	delete m_imp ;
}


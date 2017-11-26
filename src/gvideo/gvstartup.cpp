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
// gvstartup.cpp
//

#include "gdef.h"
#include "gvstartup.h"
#include "gvexit.h"
#include "genvironment.h"
#include "gdaemon.h"
#include "gsharedmemory.h"
#include "groot.h"
#include "gprocess.h"
#include "geventloop.h"
#include "locale.h"
#include <iostream>
#include <signal.h>

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.1"
#endif

static const char * legal_text = 
	"Copyright (C) 2017 Graeme Walker\n"
	"\n"
	"This program is free software: you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation, either version 3 of the License, or\n"
	"(at your option) any later version.\n"
	"\n"
	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public License\n"
	"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
;

namespace
{
	struct gconfig
	{
		static bool have_libjpeg ;
		static bool have_libpng ;
		static bool have_libav ;
		static bool have_libv4l ;
		static bool have_v4l ;
		static bool have_curses ;
		static bool have_x11 ;
	} ;
	bool with_debug( bool b )
	{
		return b || G::Environment::get("VT_DEBUG","")=="1" ;
	}
	bool with_verbose( bool b )
	{
		return b || G::Environment::get("VT_VERBOSE","")=="1" ;
	}
}

Gv::Startup::Startup( const G::GetOpt & opt , const std::string & args_help , 
	bool argc_ok , const std::string & extra_help ) :
		m_debug(with_debug(opt.contains("debug"))) ,
		m_log_output(opt.args().prefix() ,
			true , // output from G_*()
			true , // output from G_LOG_S()
			with_verbose(opt.contains("verbose")||m_debug) , // output from G_LOG()
			m_debug , // output from G_DEBUG()
			true , // with level
			opt.contains("log-time") , // with timestamp
			!m_debug , // strip context
			opt.contains("syslog")) ,
		m_daemon(opt.contains("daemon")) ,
		m_syslog(opt.contains("syslog")) ,
		m_verbose(opt.contains("verbose")) ,
		m_pid_file(opt.value("pid-file")) ,
		m_user(opt.value("user","")) ,
		m_started(false)
{
	if( opt.hasErrors() )
	{
		opt.showErrors( std::cerr ) ;
		throw Gv::Exit( EXIT_FAILURE ) ;
	}
	if( opt.contains("help") )
	{
		opt.options().showUsage( std::cout , opt.args().prefix() , args_help ) ;
		if( !extra_help.empty() )
			std::cout << "\n" << extra_help << std::endl ;
		std::cout << "\nCopyright (C) 2017 Graeme Walker" << std::endl ;
		throw Gv::Exit( EXIT_SUCCESS ) ;
	}
	if( opt.contains("version") )
	{
		std::cout << PACKAGE_VERSION << std::endl ; // version from automake header
		if( opt.contains("verbose") )
		{
			std::cout << "libjpeg: " << gconfig::have_libjpeg << std::endl ;
			std::cout << "libpng: " << gconfig::have_libpng << std::endl ;
			std::cout << "libav: " << gconfig::have_libav << std::endl ;
			std::cout << "libv4l: " << gconfig::have_libv4l << std::endl ;
			std::cout << "v4l: " << gconfig::have_v4l << std::endl ;
			std::cout << "curses: " << gconfig::have_curses << std::endl ;
			std::cout << "xlib: " << gconfig::have_x11 << std::endl ;
		}
		std::cout << "\n" << legal_text << std::endl ;
		throw Gv::Exit( EXIT_SUCCESS ) ;
	}
	if( !argc_ok )
	{
		opt.options().showUsage( std::cerr , opt.args().prefix() , args_help ) ;
		std::cerr << "\nCopyright (C) 2017 Graeme Walker" << std::endl ;
		throw Gv::Exit( EXIT_FAILURE ) ;
	}

	setlocale( LC_ALL , "" ) ;
	G::SharedMemory::help( ": consider doing \"vt-channel %s delete\" to clean up if the channel publisher is not running" ) ;
	G::SharedMemory::prefix( "vt-" ) ;
	G::SharedMemory::filetext( "# shmem placeholder -- see man videotools\n" ) ;

	if( m_daemon )
	{
		// during startup we keep stderr open so that startup errors are visible 
		const bool keep_stderr = true ; // see closeStderr() below
		G::Process::closeFiles( keep_stderr ) ;

		// but verbose logging is probably not wanted until after we have gone 
		// syslog-only, with stderr closed
		if( m_syslog )
			m_log_output.verbose( false ) ;
	}

	// switch to the non-privileged account early so that any file-system 
	// and ipc resources have the correct group ownership
	if( !m_user.empty() )
	{
		// do not change group-id by default -- resources will be created
		// with the parent process's real group-id or the exe's group id
		// if group-suid
		G::Root::init( m_user , false/*chgrp(root)*/ ) ; 
	}
}

void Gv::Startup::start()
{
	if( m_daemon ) 
		G::Daemon::detach( m_pid_file ) ;

	{
		G::Root claim_root ;
		G::Process::Umask umask( G::Process::Umask::Readable ) ;
		m_pid_file.commit() ; 
	}

	if( m_daemon && m_syslog )
	{
		G::Process::closeStderr() ;
		m_log_output.verbose( m_verbose ) ;
	}

	// set a SIGHUP signal handler to quit the event loop, esp. for profiling
	if( GNet::EventLoop::exists() )
		::signal( SIGHUP , Gv::Startup::onSignal ) ;

	m_started = true ;
}

void Gv::Startup::onSignal( int )
{
	GNet::EventLoop::stop( G::SignalSafe() ) ;
}

void Gv::Startup::report( const std::string & prefix , std::exception & e )
{
	report( prefix , e.what() ) ;
}

void Gv::Startup::report( const std::string & prefix , const std::string & what )
{
	try
	{
		if( m_started && ( m_daemon || m_syslog ) )
			G_ERROR( "" << prefix << ": " << what ) ;
	}
	catch(...)
	{
	}
}

void Gv::Startup::sanitise( int , char * [] )
{
	// TODO remove passwords from urls in argv
}




#if GCONFIG_HAVE_LIBJPEG
bool gconfig::have_libjpeg = true ;
#else
bool gconfig::have_libjpeg = false ;
#endif

#if GCONFIG_HAVE_LIBPNG
bool gconfig::have_libpng = true ;
#else
bool gconfig::have_libpng = false ;
#endif

#if GCONFIG_HAVE_LIBAV
bool gconfig::have_libav = true ;
#else
bool gconfig::have_libav = false ;
#endif

#if GCONFIG_HAVE_LIBV4L
bool gconfig::have_libv4l = true ;
#else
bool gconfig::have_libv4l = false ;
#endif

#if GCONFIG_HAVE_V4L
bool gconfig::have_v4l = true ;
#else
bool gconfig::have_v4l = false ;
#endif

#if GCONFIG_HAVE_CURSES
bool gconfig::have_curses = true ;
#else
bool gconfig::have_curses = false ;
#endif

#if GCONFIG_HAVE_X11
bool gconfig::have_x11 = true ;
#else
bool gconfig::have_x11 = false ;
#endif

/// \file gvstartup.cpp

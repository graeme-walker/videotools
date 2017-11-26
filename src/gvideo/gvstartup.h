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
/// \file gvstartup.h
///

#ifndef GV_STARTUP__H
#define GV_STARTUP__H

#include "gdef.h"
#include "glogoutput.h"
#include "gpidfile.h"
#include "ggetopt.h"

namespace Gv
{
	class Startup ;
}

/// \class Gv::Startup
/// A handy synthesis of G::LogOutput, G::GetOpt, G::Root, G::Daemon and 
/// G::PidFile, used to initialise a server process.
/// 
class Gv::Startup
{
public:
	Startup( const G::GetOpt & , const std::string & args_help , bool argc_ok , 
		const std::string & extra_help = std::string() ) ;
			///< Constructor, typically used very early in main(). Parses the
			///< command-line and processes "--help" and "--version". Assumes
			///< there are options for "daemon", "user", "verbose", "debug",
			///< "log-time", "syslog" and "pid-file". Throws Gv::Exit for
			///< an immediate exit.

	void start() ;
		///< Called just before the event loop. This should be used after all 
		///< non-event-driven initialisation has completed. The process forks and exits 
		///< the parent, if required.

	void report( const std::string & prefix , std::exception & e ) ;
		///< Reports an error if start()ed and "daemon" or "syslog" are in effect.
		///< This is useful for errors that would otherwise only go to stderr
		///< since stderr might be closed.

	void report( const std::string & prefix , const std::string & what ) ;
		///< Reports an error if start()ed and "daemon" or "syslog" are in effect.

	static void sanitise( int argc , char * argv [] ) ;
		///< Removes sensitive information from the command-line, if possible.

private:
	Startup( const Startup & ) ;
	void operator=( const Startup & ) ;
	static void onSignal( int ) ;

private:
	bool m_debug ;
	G::LogOutput m_log_output ;
	bool m_daemon ;
	bool m_syslog ;
	bool m_verbose ;
	G::PidFile m_pid_file ;
	std::string m_user ;
	bool m_started ;
} ;

#endif

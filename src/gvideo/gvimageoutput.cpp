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
// gvimageoutput.cpp
//

#include "gdef.h"
#include "gvimageoutput.h"
#include "grimagetype.h"
#include "gprocess.h"
#include "gdatetime.h"
#include "gstr.h"
#include "groot.h"
#include "gfile.h"
#include "gpath.h"
#include "gassert.h"
#include "gtest.h"
#include "genvironment.h"
#include "glogoutput.h"
#include "glog.h"
#include "garg.h"
#include <unistd.h> // fork()
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>

namespace
{
	struct Args
	{
		G::StringArray list ;
		std::vector<char*> pointers ;
		void add( const std::string & s ) { list.push_back(s) ; }
		void finish() 
		{ 
			for( G::StringArray::iterator p = list.begin() ; p != list.end() ; ++p )
				pointers.push_back( const_cast<char*>((*p).c_str()) ) ;
			pointers.push_back(nullptr) ; 
		}
	} ;
}

static pid_t fork_()
{
	pid_t pid = fork() ;
	if( pid == static_cast<pid_t>(-1) )
		throw std::runtime_error( "fork error" ) ;
	return pid ;
}

Gv::ImageOutput::ImageOutput() :
	m_fast(false) ,
	m_tz(0) ,
	m_save_test_mode(false) ,
	m_viewer_up(false)
{
}

Gv::ImageOutput::ImageOutput( GNet::EventExceptionHandler & handler ) :
	m_fast(false) ,
	m_tz(0) ,
	m_save_test_mode(false) ,
	m_viewer_up(false)
{
	m_ping_timer_ptr.reset( new GNet::Timer<ImageOutput>(*this,&ImageOutput::onPingTimeout,handler) ) ;
}

void Gv::ImageOutput::startPublisher( const std::string & channel )
{
	G::Item info = G::Item::map() ;
	if( !info.has("type") )
		info.add( "type" , G::Str::tail(G::Path(G::Arg::v0()).basename(),"-",false) ) ;

	m_publisher.reset( new G::Publisher(channel,info) ) ;
}

void Gv::ImageOutput::startPublisher( const std::string & channel , const G::Item & info_in )
{
	G::Item info( info_in ) ;
	if( !info.has("type") )
		info.add( "type" , G::Str::tail(G::Path(G::Arg::v0()).basename(),"-",false) ) ;

	m_publisher.reset( new G::Publisher(channel,info) ) ;
}

void Gv::ImageOutput::startPublisher( G::Publisher * p )
{
	m_publisher.reset( p ) ;
}

void Gv::ImageOutput::startViewer( const std::string & title , unsigned int viewer_scale , const std::string & event_channel )
{
	const bool debug = G::Test::enabled("viewer-debug") ;
	const bool qforquit = !G::Environment::get("VT_VIEWER_Q","").empty() ;
	const bool syslog = G::LogOutput::instance() && G::LogOutput::instance()->syslog() ;

	std::string viewer_exe ;
	if( G::Test::enabled("viewer-path-from-environment") ) // security risk if environment is poisoned
	{
		viewer_exe = G::Environment::get( "VT_VIEWER" , "" ) ;
	}
	if( viewer_exe.empty() )
	{
		std::string this_exe = G::Arg::exe() ; // throws on error eg. if no procfs
		viewer_exe = viewer( this_exe ) ;
	}

	m_fat_pipe.reset( new G::FatPipe ) ;
	if( 0 == fork_() )
	{
		m_fat_pipe->doChild() ;
		std::string sscale = G::Str::fromUInt(viewer_scale) ;
		Args args ;
		args.add( viewer_exe ) ;
		if( qforquit ) { args.add( "--quit" ) ; }
		if( debug ) { args.add( "--debug" ) ; }
		if( syslog ) { args.add( "--syslog" ) ; }
		if( !title.empty() ) { args.add( "--title=" + sanitise(title) ) ; }
		if( viewer_scale != 1U ) { args.add( "--scale=" + sanitise(sscale) ) ; }
		if( !event_channel.empty() ) { args.add( "--channel=" + sanitise(event_channel) ) ; }
		args.add( m_fat_pipe->shmemfd() ) ;
		args.add( m_fat_pipe->pipefd() ) ;
		args.finish() ;
		::execv( viewer_exe.c_str() , &args.pointers[0] ) ;
		int e = errno ;
		std::cerr << "error: exec failed for [" << viewer_exe << "]: " << G::Process::strerror(e) << "\n" ;
		_exit( 1 ) ;
	}
	m_fat_pipe->doParent() ;

	if( m_ping_timer_ptr.get() != nullptr )
		m_ping_timer_ptr->startTimer( 1U ) ;
}

std::string Gv::ImageOutput::sanitise( const std::string & s_in )
{
	std::string meta_in = G::Str::meta() ;
	std::string meta_out( meta_in.size() , '.' ) ;
	return G::Str::escaped( G::Str::printable(s_in) , '\\' , meta_in , meta_out ) ;
}

bool Gv::ImageOutput::viewing() const
{
	return m_fat_pipe.get() != nullptr ;
}

bool Gv::ImageOutput::pingViewer()
{
	return m_fat_pipe.get() ? m_fat_pipe->ping() : false ;
}

void Gv::ImageOutput::onPingTimeout()
{
	G_ASSERT( m_ping_timer_ptr.get() != nullptr ) ;
	pingViewerCheck() ;
	m_ping_timer_ptr->startTimer( 1U ) ;
}

void Gv::ImageOutput::pingViewerCheck()
{
	if( pingViewer() )
		m_viewer_up = true ;
	else if( m_viewer_up )
		throw std::runtime_error( "viewer has gone away" ) ;
}

void Gv::ImageOutput::saveTo( const std::string & base_dir , const std::string & name , bool fast , const Gv::Timezone & tz , bool test_mode )
{
	m_base_dir = base_dir ;
	m_name = name ;
	m_fast = fast ;
	m_tz = tz ;
	m_save_test_mode = test_mode ;
}

const std::string & Gv::ImageOutput::dir() const
{
	return m_base_dir ;
}

G::Path Gv::ImageOutput::send( const char * p , size_t n , Gr::ImageType type , G::EpochTime time )
{
	Gr::ImageType::String type_str ; type.set( type_str ) ;

	if( m_publisher.get() )
		m_publisher->publish( p , n , type_str.c_str() ) ;

	if( m_fat_pipe.get() )
		m_fat_pipe->send( p , n , type_str.c_str() ) ;

	if( m_base_dir.empty() )
		return G::Path() ;
	else
		return save( p , n , type , time ) ;
}

G::Path Gv::ImageOutput::send( const Gr::ImageBuffer & buffer , Gr::ImageType type , G::EpochTime time )
{
	Gr::ImageType::String type_str ; type.set( type_str ) ;

	if( m_publisher.get() )
		m_publisher->publish( buffer , type_str.c_str() ) ;

	if( m_fat_pipe.get() )
		m_fat_pipe->send( buffer , type_str.c_str() ) ;

	if( m_base_dir.empty() )
		return G::Path() ;
	else
		return save( buffer , type , time ) ;
}

void Gv::ImageOutput::sendText( const char * p , size_t n , const std::string & type )
{
	if( m_publisher.get() )
		m_publisher->publish( p , n , type.c_str() ) ;

	if( m_fat_pipe.get() )
		m_fat_pipe->send( p , n , type.c_str() ) ;
}

std::string Gv::ImageOutput::viewer( const G::Path & this_exe )
{
	G_ASSERT( this_exe != G::Path() ) ;
	G::Path dir = this_exe.dirname() ;
	std::string this_name = this_exe.basename() ;
	size_t pos = this_name.find_last_of( "-_" ) ;
	std::string name =
		pos == std::string::npos ?
			std::string("viewer") :
			( G::Str::head(this_name,pos,std::string()) + this_name.at(pos) + "viewer" ) ;
	std::string result = G::Path(dir,name).str() ;
	G_DEBUG( "Gv::ImageOutput::viewerExecutable: viewer-exe=[" << result << "]" ) ;
	return result ;
}

namespace
{
	void append( std::string & s , short n , char c = '\0' )
	{
		char cc[3] ;
		cc[0] = '0' + (n/10) ;
		cc[1] = '0' + (n%10) ;
		cc[2] = c ;
		s.append( cc , c ? 3U : 2U ) ;
	}
	void append3( std::string & s , short n , char c = '\0' )
	{
		char cc[4] ;
		cc[0] = '0' + ((n/100)%10) ;
		cc[1] = '0' + ((n/10)%10) ;
		cc[2] = '0' + (n%10) ;
		cc[3] = c ;
		s.append( cc , c ? 4U : 3U ) ;
	}
	void append4( std::string & s , short n , char c = '\0' )
	{
		char cc[5] ;
		cc[0] = '0' + ((n/1000)%10) ;
		cc[1] = '0' + ((n/100)%10) ;
		cc[2] = '0' + ((n/10)%10) ;
		cc[3] = '0' + (n%10) ;
		cc[4] = c ;
		s.append( cc , c ? 5U : 4U ) ;
	}
}

void Gv::ImageOutput::path( std::string & out , const std::string & base_dir , const std::string & name ,
	G::EpochTime time , Gr::ImageType type , bool fast , const Gv::Timezone & tz , bool test_mode )
{
	G_ASSERT( name.find('/') == std::string::npos ) ;

	out.reserve( base_dir.length() + name.length() + 30U ) ;
	out.assign( base_dir ) ; 
	out.append( 1U , '/' ) ;

	G::DateTime::BrokenDownTime tm = G::DateTime::utc(time+tz.seconds()) ;

	if( test_mode )
	{
		G_WARNING_ONCE( "Gv::ImageOutput::path: saving in test mode: date fixed to 2000" ) ;
		tm.tm_year = 100 ;
		tm.tm_mon = 0 ;
		tm.tm_mday = 1 + (tm.tm_mday & 1) ;
	}

	append4( out , tm.tm_year+1900 , '/' ) ;
	append( out , tm.tm_mon+1 , '/' ) ;
	append( out , tm.tm_mday , '/' ) ;
	append( out , tm.tm_hour , '/' ) ;
	append( out , tm.tm_min , '/' ) ;
	if( fast )
	{
		append( out , tm.tm_sec , '/' ) ;
		out.append( name ) ;
		if( !name.empty() ) out.append( 1U , '.' ) ;
		append3( out , time.us >> 10 ) ; // 0..976
	}
	else
	{
		out.append( name ) ;
		if( !name.empty() ) out.append( 1U , '.' ) ;
		append( out , tm.tm_sec , '\0' ) ;
	}

	if( type.isJpeg() )
		out.append( ".jpg" ) ;
	else if( type.isRaw() )
		out.append( type.channels() == 1 ? ".pgm" : ".ppm" ) ;
	else
		out.append( ".dat" ) ;
}

std::string Gv::ImageOutput::path( G::EpochTime time , Gr::ImageType type , bool fast ) const
{
	std::string result ;
	path( result , m_base_dir , m_name , time , type , fast , m_tz , m_save_test_mode ) ;
	return result ;
}

G::Path Gv::ImageOutput::save( const char * p , size_t n , Gr::ImageType type , G::EpochTime time )
{
	std::ofstream file ;
	G::Path path = openFile( file , type , time ) ;
	if( path != G::Path() )
	{
		file.write( p , n ) ;
		commitFile( file , path ) ;
	}
	return path ;
}

G::Path Gv::ImageOutput::save( const Gr::ImageBuffer & image_buffer , Gr::ImageType type , G::EpochTime time )
{
	std::ofstream file ;
	G::Path path = openFile( file , type , time ) ;
	if( path != G::Path() )
	{
		file << image_buffer ;
		commitFile( file , path ) ;
	}
	return path ;
}

G::Path Gv::ImageOutput::openFile( std::ofstream & file , Gr::ImageType type , G::EpochTime time )
{
	if( m_base_dir.empty() )
		return G::Path() ;

	// prepare the filename
	if( time.s == 0 ) time = G::DateTime::now() ;
	G::Path path = this->path( time , type , m_fast ) ;

	// if the same filename as last time keep the old contents
	if( path == m_old_path ) 
		return G::Path() ;

	// create the directory if necessary
	{
		G::Path dir = path.dirname() ;
		if( m_old_dir != dir )
		{
			bool ok = false ;
			{
				G::Root claim_root ;
				ok = G::File::mkdirs( dir , G::File::NoThrow() ) ;
			}
			if( !ok )
				m_old_dir = dir ;
		}
	}

	// create the file
	{
		std::string path_str = path.str() ;
		G::Root claim_root ;
		file.open( path_str.c_str() ) ;
	}

	// for raw images write out the header
	if( type.isRaw() )
	{
		// prefix with 'portable-anymap' header
		file
			<< (type.channels()==1?"P5":"P6") << "\n" 
			<< type.dx() << " " << type.dy() << "\n"
			<< "255\n" ;
	}

	return path ;
}

void Gv::ImageOutput::commitFile( std::ofstream & file , const G::Path & path )
{
	file.close() ;
	if( file.fail() ) // ie. badbit or failbit
		G_ERROR( "Gv::ImageOutput::save: file write error [" << path << "]" ) ;
	else
		m_old_path = path ;
}

bool Gv::ImageOutput::fast() const
{
	return m_fast ;
}

Gv::ImageOutput::~ImageOutput()
{
}

/// \file gvimageoutput.cpp

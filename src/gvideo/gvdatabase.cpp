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
// gvdatabase.cpp
//

#include "gdef.h"
#include "gvdatabase.h"
#include "gfile.h"
#include "gpath.h"
#include "glog.h"
#include "gassert.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Gv
{
	class DatabaseRrd ;
	class DatabaseWrapper ;
}

/// \class Gv::DatabaseWrapper
/// A concrete Database class that aggregates up to one second intervals
/// and then delegates to (eg.) DatabaseRrd.
///
class Gv::DatabaseWrapper : public Gv::Database
{
public:
	explicit DatabaseWrapper( const std::string & path ) ;
	virtual ~DatabaseWrapper() ;
	virtual void add( time_t , unsigned int n , unsigned int dx , unsigned int dy , unsigned int squelch , unsigned int apathy ) override ;
	virtual std::string graph( std::string & , time_t start_time , time_t end_time ) override ;

private:
	DatabaseRrd * m_imp ;
	unsigned int m_items ;
	unsigned int m_total ;
	unsigned int m_max ;
	time_t m_time ;
} ;

/// \class Gv::DatabaseRrd
/// Manages an rrdtool database under the Gv::Database interface.
///
class Gv::DatabaseRrd
{
public:
	explicit DatabaseRrd( const std::string & ) ;
	~DatabaseRrd() ;
	bool exists() const ;
	void create() ;
	std::string graph( std::string & , time_t start_time , time_t end_time ) ;
	void add( time_t time , unsigned int total , unsigned int items , unsigned int max ,
		unsigned int dx , unsigned int dy , unsigned int squelch , unsigned int apathy ) ;

private:
	static void run( const std::string & s ) ;
	static std::string png( const std::string & rrd ) ;
	void writeGraph( time_t start_time , time_t end_time ) ;
	void readGraph( std::string & data ) ;

private:
	std::string m_rrd_filename ;
	std::string m_png_filename ;
} ;

Gv::Database * Gv::Database::create( const std::string & path )
{
	return new DatabaseWrapper( path ) ;
}

Gv::DatabaseWrapper::DatabaseWrapper( const std::string & path ) :
	m_imp(nullptr) ,
	m_items(0U) ,
	m_total(0U) ,
	m_max(0U) ,
	m_time(0)
{
	if( !path.empty() )
		m_imp = new DatabaseRrd( path ) ;
}

Gv::DatabaseWrapper::~DatabaseWrapper()
{
	delete m_imp ;
}

void Gv::DatabaseWrapper::add( time_t time , unsigned int n , unsigned int dx , unsigned int dy , unsigned int squelch , unsigned int apathy )
{
	if( m_imp == nullptr )
		return ;

	if( !m_imp->exists() )
		m_imp->create() ;

	// we guarantee no more than one data point per second (as per rrd), 
	// so generate an average and a maximum
	//
	if( m_time == 0 || time == m_time )
	{
		m_items++ ;
		m_total += n ;
		m_max = std::max(m_max,n) ;
	}
	else
	{
		m_imp->add( time , m_total , m_items , m_max , dx , dy , squelch , apathy ) ;
		m_items = 0U ;
		m_total = 0U ;
		m_max = 0U ;
	}
	m_time = time ;
}

std::string Gv::DatabaseWrapper::graph( std::string & buffer , time_t start_time , time_t end_time )
{
	if( m_imp == nullptr ) return std::string() ;
	return m_imp->graph( buffer , start_time , end_time ) ;
}

// ==

Gv::DatabaseRrd::DatabaseRrd( const std::string & rrd_filename ) :
	m_rrd_filename(rrd_filename) ,
	m_png_filename(png(rrd_filename))
{
	G_LOG( "Gv::DatabaseRrd::ctor: rrd=[" << m_rrd_filename << "] png=[" << m_png_filename << "]" ) ;
}

Gv::DatabaseRrd::~DatabaseRrd()
{
}

std::string Gv::DatabaseRrd::png( const std::string & rrd )
{
	G_ASSERT( !rrd.empty() ) ;
	G::Path path( rrd ) ;
	return (path.dirname()+(path.withoutExtension().basename()+".png")).str() ;
}

bool Gv::DatabaseRrd::exists() const
{
	return G::File::exists( m_rrd_filename ) ;
}

void Gv::DatabaseRrd::create()
{
	G_ASSERT( !m_rrd_filename.empty() ) ;

	G::EpochTime now = G::DateTime::now() ;
	std::ostringstream ss ;
	ss 
		<< "rrdtool create " << m_rrd_filename << " "
		<< "--start " << now.s << " "
		<< "--step 1 " // populated every second
		<< "DS:items:GAUGE:3:U:U "
		<< "DS:total:GAUGE:3:U:U " // 3 => value is 'unknown' if no value for 3s, U => no min/max
		<< "DS:maximum:GAUGE:3:U:U "
		<< "DS:squelch:GAUGE:3:U:U "
		<< "DS:apathy:GAUGE:3:U:U "
		<< "DS:dx:GAUGE:3:U:U "
		<< "DS:dy:GAUGE:3:U:U "
		<< "RRA:AVERAGE:0:1:108000 "
		<< "RRA:MAX:0:1:108000 " ;
		// << "RRA:AVERAGE:0.5:10s:1y "
		// << "RRA:MAX:0.5:10s:1y " ;

	G_LOG( "Gv::DatabaseRrd::create: " << ss.str() ) ;
	run( ss.str() ) ;
	if( !exists() )
		throw std::runtime_error( "failed to create rrd" ) ;
}

void Gv::DatabaseRrd::run( const std::string & s )
{
	int rc = system( s.c_str() ) ;
	if( rc != 0 )
		G_WARNING_ONCE( "Gv::DatabaseRrd::run: command failed: [" << s << "]" ) ;
}

void Gv::DatabaseRrd::add( time_t time , unsigned int total , unsigned int items , unsigned int max ,
	unsigned int dx , unsigned int dy , unsigned int squelch , unsigned int apathy )
{
	std::ostringstream ss ;
	ss 
		<< "rrdtool update " << m_rrd_filename << " " 
		<< time << ":" 
		<< items << ":" 
		<< total << ":" 
		<< max << ":" 
		<< squelch << ":" 
		<< apathy << ":" 
		<< dx << ":" 
		<< dy ;

	G_LOG( "Gv::DatabaseRrd::emit: " << ss.str() ) ;
	run( ss.str() ) ;
}

std::string Gv::DatabaseRrd::graph( std::string & data , time_t start_time , time_t end_time )
{
	writeGraph( start_time , end_time ) ;
	readGraph( data ) ;
	return "image/png" ;
}

void Gv::DatabaseRrd::writeGraph( time_t start_time , time_t end_time )
{
	std::ostringstream ss ;
	ss 
		<< "rrdtool graph " << m_png_filename << " "
		<< "--start " << start_time << " " ;
		if( end_time ) ss
		<< "--end " << end_time << " " ; ss
		<< "DEF:c=" << m_rrd_filename << ":maximum:MAX "
		<< "DEF:a=" << m_rrd_filename << ":apathy:AVERAGE "
		<< "LINE2:c#802020 LINE2:a#208020" ;
	G_LOG( "Gv::DatabaseRrd::emitDatum: " << ss.str() ) ;
	run( ss.str() ) ;
}

void Gv::DatabaseRrd::readGraph( std::string & data )
{
	std::stringstream ss ;
	std::ifstream png( m_png_filename.c_str() ) ;
	ss << png.rdbuf() ;
	data = ss.str() ;
}


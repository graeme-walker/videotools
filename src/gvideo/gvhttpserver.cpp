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
// gvhttpserver.cpp
//

#include "gdef.h"
#include "gvhttpserver.h"
#include "gvexit.h"
#include "geventloop.h"
#include "grjpeg.h"
#include "groot.h"
#include "ghexdump.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <fstream>

Gv::HttpServerConfig::HttpServerConfig() :
	m_streaming(false) ,
	m_refresh(0U) ,
	m_type("jpeg") ,
	m_scale(1) ,
	m_monochrome(false) ,
	m_idle_timeout(20U) ,
	m_first_image_timeout(3U) ,
	m_image_repeat_timeout(1U) ,
	m_gateway(GNet::Address::defaultAddress()) ,
	m_more_verbose(true) ,
	m_quick(true)
{
}

void Gv::HttpServerConfig::init( unsigned int idle_timeout , 
	unsigned int first_image_timeout , G::EpochTime image_repeat_timeout , unsigned int refresh , 
	const GNet::Address & gateway , bool more_verbose )
{
	m_idle_timeout = idle_timeout ;
	m_first_image_timeout = std::max( 1U , first_image_timeout ) ;
	m_image_repeat_timeout = image_repeat_timeout ;
	m_refresh = refresh ;
	m_gateway = gateway ;
	m_more_verbose = more_verbose ;
}

void Gv::HttpServerConfig::init( unsigned int idle_timeout , 
	unsigned int first_image_timeout , G::EpochTime image_repeat_timeout , unsigned int refresh ,
	const std::string & type )
{
	m_idle_timeout = idle_timeout ;
	m_first_image_timeout = std::max( 1U , first_image_timeout ) ;
	m_image_repeat_timeout = image_repeat_timeout ;
	m_refresh = refresh ;
	m_type = type ;
}

bool Gv::HttpServerConfig::integral( const G::Url & url , const std::string & key )
{
	return url.has(key) && !url.parameter(key).empty() && G::Str::isUInt(url.parameter(key)) ;
}

void Gv::HttpServerConfig::init( const G::Url & url )
{
	{
		typedef G::Url::Map Map ;
		Map params = url.pmap() ;
		std::vector<std::string> errors ;
		for( Map::iterator p = params.begin() ; p != params.end() ; ++p )
		{
			if( (*p).first != "streaming" &&
				(*p).first != "refresh" &&
				(*p).first != "scale" &&
				(*p).first != "monochrome" &&
				(*p).first != "type" &&
				(*p).first != "quick" &&
				(*p).first != "wait" )
			{
				errors.push_back( (*p).first ) ;
			}
		}
		if( !errors.empty() )
			G_WARNING( "Gv::HttpServerConfig::init: ignoring invalid url parameters: " << G::Str::printable(G::Str::join(",",errors)) ) ;
	}

	if( integral(url,"streaming") )
		m_streaming = !! G::Str::toUInt( url.parameter("streaming") ) ;

	if( integral(url,"refresh") )
		m_refresh = G::Str::toUInt( url.parameter("refresh") ) ;

	if( integral(url,"scale") )
		m_scale = G::Str::toUInt( url.parameter("scale") ) ;

	if( integral(url,"monochrome") )
		m_monochrome = !! G::Str::toUInt( url.parameter("monochrome") ) ;

	if( url.parameter("type","") == "raw" ) 
		m_type = "raw" ;
	else if( url.parameter("type","") == "jpeg" )
		m_type = "jpeg" ;
	else if( url.parameter("type","") == "pnm" ) 
		m_type = "pnm" ;
	else if( url.parameter("type","") == "any" )
		m_type = "any" ; // ie. no conversion
	else if( !url.parameter("type","").empty() )
		G_WARNING( "Gv::HttpServerConfig::init: ignoring unknown image type in url: [" + url.parameter("type","") + "]" ) ;

	if( integral(url,"quick") )
		m_quick = !! G::Str::toUInt( url.parameter("quick") ) ;

	if( integral(url,"wait") )
		m_first_image_timeout = G::Str::toUInt( url.parameter("wait") ) ;
}

unsigned int Gv::HttpServerConfig::idleTimeout() const
{
	return m_idle_timeout ;
}

unsigned int Gv::HttpServerConfig::firstImageTimeout() const
{
	return m_first_image_timeout ;
}

bool Gv::HttpServerConfig::moreVerbose() const
{
	return m_more_verbose ;
}

std::string Gv::HttpServerConfig::type() const
{
	return m_type ;
}

bool Gv::HttpServerConfig::quick() const
{
	return m_quick ;
}

bool Gv::HttpServerConfig::streaming() const
{
	return m_streaming ;
}

GNet::Address Gv::HttpServerConfig::gateway() const
{
	return m_gateway ;
}

G::EpochTime Gv::HttpServerConfig::imageRepeatTimeout() const
{
	return m_image_repeat_timeout ;
}

int Gv::HttpServerConfig::scale() const
{
	return m_scale ;
}

bool Gv::HttpServerConfig::monochrome() const
{
	return m_monochrome ;
}

unsigned int Gv::HttpServerConfig::refresh() const
{
	return m_refresh ;
}

// ==

Gv::HttpServerResources::HttpServerResources() :
	m_any_file(false) ,
	m_any_channel(false) ,
	m_with_specials(false)
{
}

std::string Gv::HttpServerResources::guessType( const G::Path & path )
{
	std::string extension = path.extension() ;
	if( extension == "js" ) return "text/javascript" ;
	if( extension == "css" ) return "text/css" ;
	if( extension == "html" ) return "text/html" ;
	if( extension == "png" ) return "image/png" ;
	if( extension == "jpeg" ) return "image/jpeg" ;
	if( extension == "jpg" ) return "image/jpeg" ;
	if( extension == "pnm" ) return "image/x-portable-anymap" ;
	if( extension == "pgm" ) return "image/x-portable-anymap" ; // -graymap
	if( extension == "pbm" ) return "image/x-portable-anymap" ; // -bitmap
	if( extension == "ppm" ) return "image/x-portable-anymap" ; // -pixmap
	return std::string() ;
}

void Gv::HttpServerResources::addChannelAny()
{
	m_any_channel = true ;
	m_with_specials = true ;
}

bool Gv::HttpServerResources::anyChannel() const
{
	return m_any_channel ;
}

void Gv::HttpServerResources::addChannel( const std::string & channel_name )
{
	if( channel_name == "*" )
		m_any_channel = true ;
	else
		m_channels.push_back( channel_name ) ;
}

void Gv::HttpServerResources::setDirectory( const G::Path & dir )
{
	//G_ASSERT( m_dir == G::Path() ) ;
	m_dir = dir ;
}

void Gv::HttpServerResources::addFileAny()
{
	m_any_file = true ;
}

std::string Gv::HttpServerResources::addFile( const G::Path & path_in )
{
	if( path_in.isAbsolute() )
		return "invalid absolute path" ;
	G::Path path = normalise( path_in ) ;

	if( m_files.find(path.str()) != m_files.end() )
		return "duplicate" ;

	m_files[path.str()] = G::Path::join(m_dir,path).str() ;
	return std::string() ;
}

void Gv::HttpServerResources::addFileType( const G::Path & path_in , const std::string & type )
{
	G::Path path = normalise( path_in ) ;
	m_file_types[path.str()] = type ;
}

std::string Gv::HttpServerResources::setDefault( const std::string & name )
{
	m_default_resource = name ;

	if( name.find('_') == 0U )
	{
		if( !m_any_channel && std::find(m_channels.begin(),m_channels.end(),name) == m_channels.end() )
			return "no such channel: [" + name + "]" ;
	}
	else 
	{
		if( m_files.find(name) == m_files.end() && !m_any_file )
			return "no such file: [" + name + "]" ;
	}
	return std::string() ;
}

bool Gv::HttpServerResources::empty() const
{
	return !m_any_channel && !m_any_file && m_files.empty() && m_channels.empty() ;
}

Gv::HttpServerResources::ResourceType Gv::HttpServerResources::resourceType( const G::Path & url_path ) const
{
	std::string basename = url_path == "/" ? m_default_resource : url_path.basename() ;
	if( empty() ) // nothing configured
		return t_source ;
	else if( m_with_specials && basename.find("__") == 0U )
		return t_special ;
	else if( basename.find("_") == 0U )
		return t_channel ;
	else
		return t_file ;
}

std::vector<std::string> Gv::HttpServerResources::channels() const
{
	return m_channels ;
}

std::string Gv::HttpServerResources::channelName( const G::Path & url_path ) const
{
	std::string basename = url_path == "/" ? m_default_resource : url_path.basename() ;
	if( basename.find('_') == 0U && basename.find("__") != 0U )
		return basename.substr( 1U ) ;
	else
		return std::string() ;
}

bool Gv::HttpServerResources::channelResource( const G::Path & url_path ) const
{
	return resourceType(url_path) == t_channel ;
}

bool Gv::HttpServerResources::specialResource( const G::Path & url_path ) const
{
	return resourceType(url_path) == t_special ;
}

bool Gv::HttpServerResources::fileResource( const G::Path & url_path ) const
{
	return resourceType(url_path) == t_file ;
}

std::string Gv::HttpServerResources::fileResourcePath( const G::Path & url_path ) const
{
	return fileResourcePair(url_path).first ;
}

std::string Gv::HttpServerResources::fileResourceType( const G::Path & url_path ) const
{
	return fileResourcePair(url_path).second ;
}

bool Gv::HttpServerResources::readable( const G::Path & path )
{
	std::ifstream f ;
	{
		G::Root claim_root ;
		f.open( path.str().c_str() ) ;
	}
	return f.good() ;
}

G::Path Gv::HttpServerResources::normalise( const G::Path & path_in )
{
	// eg. /foo////.././bar -> bar
	G::StringArray parts = path_in.collapsed().split() ;
	if( !parts.empty() && parts.at(0U) == "/" ) 
		parts.erase( parts.begin() ) ;
	return G::Path::join( parts ) ;
}

G::Path Gv::HttpServerResources::normalise( const G::Path & path_in , const std::string & default_ )
{
	G::StringArray parts = path_in.collapsed().split() ;
	if( parts.size() == 1U && parts.at(0U) == "/" )
		parts = G::Path(default_).collapsed().split() ;
	if( !parts.empty() && parts.at(0U) == "/" ) 
		parts.erase( parts.begin() ) ;
	return G::Path::join( parts ) ;
}

std::pair<std::string,std::string> Gv::HttpServerResources::fileResourcePair( const G::Path & url_path_in ) const
{
	G_ASSERT( resourceType(url_path_in) == t_file ) ;
	std::pair<std::string,std::string> result ; // path,type or "",reason
	std::string reason ;

	// normalise the url path and make it relative
	G::Path url_path = normalise( url_path_in , m_default_resource ) ;

	if( m_dir == G::Path() )
	{
		// error
		reason = "no base directory configured" ;
	}
	else if( url_path == G::Path() || url_path == "." || url_path.str().find("..") == 0U )
	{
		// error
		reason = "invalid path" ;
	}
	else if( m_any_file )
	{
		G::Path file_path = G::Path::join( m_dir , url_path ) ;
		std::string file_key = url_path.str() ;
		Map::const_iterator file_type_p = m_file_types.find( file_key ) ;
		std::string type = file_type_p == m_file_types.end() ? guessType(file_key) : (*file_type_p).second ;
		if( readable(file_path) )
		{
			result.first = file_path.str() ;
			result.second = type ;
		}
		else
		{
			reason = "not readable: [" + file_path.str() + "]" ;
		}
	}
	else
	{
		G::Path file_path = G::Path::join( m_dir , url_path ) ;
		std::string file_key = url_path.str() ;
		Map::const_iterator file_type_p = m_file_types.find( file_key ) ;
		std::string type = file_type_p == m_file_types.end() ? guessType(file_key) : (*file_type_p).second ;
		Map::const_iterator file_p = m_files.find( file_key ) ;
		if( file_p == m_files.end() )
		{
			// not allowed
			reason = std::string("not in the allowed file list: [") + file_key + "]" ;
		}
		else if( readable(file_path) )
		{
			result.first = file_path.str() ;
			result.second = type ;
		}
		else
		{
			reason = "not readable: [" + file_path.str() + "]" ;
		}
	}
	if( !reason.empty() )
	{
		result.first = std::string() ;
		result.second = reason ;
	}
	return result ;
}

void Gv::HttpServerResources::log( std::ostream & out ) const
{
	if( m_dir != G::Path() )
	{
		out << "directory: path=[" << m_dir << "]\n" ;
	}
	if( m_any_file )
	{
		out << "file: <any>\n" ;
	}
	for( Map::const_iterator file_p = m_files.begin() ; file_p != m_files.end() ; ++file_p )
	{
		out << "file: filename=[" << (*file_p).first << "]\n" ;
	}
	for( Map::const_iterator file_type_p = m_file_types.begin() ; file_type_p != m_file_types.end() ; ++file_type_p )
	{
		out << "file-type: filename=[" << (*file_type_p).first << "] type=[" << (*file_type_p).second << "]\n" ;
	}
	for( G::StringArray::const_iterator channel_p = m_channels.begin() ; channel_p != m_channels.end() ; ++channel_p )
	{
		out << "channel: name=[" << *channel_p << "]\n" ;
	}
	if( m_any_channel )
	{
		out << "channel: <any>\n" ;
	}
	out << "default: [" << m_default_resource << "]\n" ;
}

void Gv::HttpServerResources::log() const
{
	if( !empty() )
	{
		std::ostringstream ss ;
		log( ss ) ;
		G::StringArray lines ; G::Str::splitIntoFields( ss.str() , lines , "\n" ) ;
		for( G::StringArray::iterator p = lines.begin() ; p != lines.end() ; ++p )
		{
			if( !(*p).empty() )
				G_LOG( "Gv::HttpServerResources::log: server resource: " << *p ) ;
		}
	}
}

// ==

Gv::HttpServerInput::HttpServerInput( Gr::ImageConverter & converter , 
	const std::string & channel_name , unsigned int reopen_timeout ) :
		Gv::ImageInput(converter,channel_name,true/*lazy*/) ,
		m_fd(-1) ,
		m_reopen_timeout(reopen_timeout) ,
		m_reopen_timer(*this,&HttpServerInput::onTimeout,static_cast<GNet::EventHandler&>(*this))
{
}

void Gv::HttpServerInput::start()
{
	if( m_fd == -1 )
	{
		if( open() ) // Gv::ImageInput::open()
		{
			m_fd = fd() ;
			if( m_fd != -1 )
				attach() ;
		}
	}
}

void Gv::HttpServerInput::attach()
{
	G_ASSERT( m_fd != -1 ) ;
	G_LOG( "Gv::HttpServerInput::attach: starting channel [" << name() << "] fd=" << m_fd ) ;
	GNet::EventLoop::instance().addRead( GNet::Descriptor(m_fd) , *this ) ;
}

void Gv::HttpServerInput::detach()
{
	G_ASSERT( m_fd != -1 ) ;
	G_LOG( "Gv::HttpServerInput::detach: stopping channel [" << name() << "] fd=" << m_fd ) ;
	GNet::EventLoop::instance().dropRead( GNet::Descriptor(m_fd) ) ;
	m_fd = -1 ;
}

Gv::HttpServerInput::~HttpServerInput()
{
	if( m_fd != -1 )
		detach() ;
}

void Gv::HttpServerInput::onException( std::exception & )
{
	throw ;
}

void Gv::HttpServerInput::readEvent()
{
	if( !handleReadEvent() )
	{
		close() ; // Gv::ImageInput::close()
		detach() ;
		G_LOG( "Gv::HttpServerInput::readEvent: channel failed [" << name() << "]" ) ;
		if( m_reopen_timeout != 0U )
			m_reopen_timer.startTimer( m_reopen_timeout ) ;
	}
}

void Gv::HttpServerInput::onTimeout()
{
	G_DEBUG( "Gv::HttpServerInput::onTimeout: checking for channel [" << name() << "]" ) ;
	start() ;
	if( m_fd == -1 )
		m_reopen_timer.startTimer( m_reopen_timeout ) ;
}

// ==

Gv::HttpServerSources::HttpServerSources( Gr::ImageConverter & converter , 
	const HttpServerResources & resources , unsigned int reopen_timeout ) :
		m_converter(converter) ,
		m_resources(&resources) ,
		m_reopen_timeout(reopen_timeout)
{
	// add configured channels
	G::StringArray list = m_resources->channels() ;
	for( G::StringArray::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		addChannel( *p , true/*throw*/ ) ;
		G_LOG( "Gv::HttpServerSources::refresh: channel _" << (m_list.size()-1U) << "=[" << *p << "]" ) ;
	}
}

Gv::HttpServerSources::HttpServerSources( Gr::ImageConverter & converter , ImageInputSource & source ) :
	m_converter(converter) ,
	m_resources(nullptr) ,
	m_reopen_timeout(0U)
{
	G_DEBUG( "Gv::HttpServerSources::addSource: adding non-channel source [" << source.name() << "]" ) ;
	m_list.push_back( Pair(&source,nullptr) ) ;
}

Gr::ImageConverter & Gv::HttpServerSources::converter()
{
	return m_converter ;
}

Gv::HttpServerSources::~HttpServerSources()
{
	typedef std::vector<Pair> List ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
		delete (*p).second ;
}

bool Gv::HttpServerSources::valid( const std::string & url_path )
{
	return m_resources == nullptr || m_resources->empty() || m_resources->channelResource(url_path) ;
}

bool Gv::HttpServerSources::select( const std::string & url_path , HttpServerSource & source_holder )
{
	G_ASSERT( valid(url_path) ) ;

	// update the "--channel=*" list
	//
	if( m_resources != nullptr && m_resources->anyChannel() )
	{
		G::StringArray channels = G::Publisher::list() ;
		for( G::StringArray::iterator p = channels.begin() ; p != channels.end() ; ++p )
		{
			Pair pair = findByName( *p ) ;
			if( pair.first == nullptr )
			{
				if( addChannel( *p , false/*throw*/ ) )
					G_LOG( "Gv::HttpServerSources::refresh: adding channel " << (m_list.size()-1U) << "=[" << *p << "]" ) ;
			}
		}
	}

	// find the source
	//
	Pair pair( nullptr , nullptr ) ; // ImageInputSource, HttpServerInput
	if( m_resources == nullptr || m_resources->empty() )
	{
		if( m_list.empty() ) throw std::runtime_error( "no sources configured" ) ;
		std::string source_name = G::Path(url_path).basename() ;
		pair = findByName( source_name ) ;
		if( pair.first == nullptr )
			pair = m_list.front() ; // _0
	}
	else 
	{
		std::string channel_name = m_resources->channelName( url_path ) ;
		if( !channel_name.empty() && G::Str::isUInt(channel_name) )
		{
			size_t channel_number = G::Str::toUInt( channel_name ) ;
			pair = findByNumber( channel_number ) ;
		}
		else
		{
			std::string channel_name = m_resources->channelName( url_path ) ;
			pair = findByName( channel_name ) ;
		}
	}

	// select it
	//
	if( pair.second != nullptr ) pair.second->start() ;
	return source_holder.set( pair.first , pair.second?pair.second->info():std::string() ) ;
}

bool Gv::HttpServerSources::addChannel( const std::string & channel_name , bool do_throw )
{
	try
	{
		HttpServerInput * p = new HttpServerInput( m_converter , channel_name , m_reopen_timeout ) ; 
		m_list.push_back( Pair(p,p) ) ;
		return true ;
	}
	catch( std::exception & ) // esp. G::PublisherError
	{
		G_DEBUG( "Gv::HttpServerSources::addChannel: invalid channel [" << channel_name << "]" ) ;
		if( do_throw ) throw ;
		return false ;
	}
}

Gv::HttpServerSources::Pair Gv::HttpServerSources::findByName( const std::string & source_name )
{
	typedef std::vector<Pair> List ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		ImageInputSource * source = (*p).first ;
		if( source->name() == source_name )
			return *p ;
	}
	return Pair( nullptr , nullptr ) ;
}

Gv::HttpServerSources::Pair Gv::HttpServerSources::findByNumber( size_t channel_number )
{
	if( channel_number < m_list.size() )
		return m_list.at( channel_number ) ;
	else
		return Pair( nullptr , nullptr ) ;
}

// ==

Gv::HttpServerSource::HttpServerSource( ImageInputHandler & handler ) :
	m_handler(handler) ,
	m_source(nullptr)
{
}

Gv::HttpServerSource::~HttpServerSource()
{
	if( m_source != nullptr )
		m_source->removeImageInputHandler( m_handler ) ;
}

std::string Gv::HttpServerSource::name() const
{
	return m_source == nullptr ? std::string() : m_source->name() ;
}

std::string Gv::HttpServerSource::info() const
{
	return m_info ;
}

void Gv::HttpServerSource::resend()
{
	if( m_source != nullptr )
		m_source->resend( m_handler ) ;
}

Gv::ImageInputSource * Gv::HttpServerSource::get()
{
	return m_source ;
}

bool Gv::HttpServerSource::set( ImageInputSource * source , const std::string & info )
{
	bool new_source = m_source != source ;
	if( new_source )
	{
		if( m_source != nullptr )
			m_source->removeImageInputHandler( m_handler ) ;
		m_source = source ;
		m_info = info ;
		if( m_source != nullptr )
			m_source->addImageInputHandler( m_handler ) ;
	}
	return new_source ;
}

/// \file gvhttpserver.cpp

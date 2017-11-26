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
/// \file gvhttpserver.h
///

#ifndef GV_HTTPSERVER__H
#define GV_HTTPSERVER__H

#include "gdef.h"
#include "gvimageinput.h"
#include "gdatetime.h"
#include "gaddress.h"
#include "gurl.h"
#include "gpath.h"
#include <string>
#include <memory>
#include <vector>
#include <utility>

namespace Gv
{
	class HttpServerSource ;
	class HttpServerSources ;
	class HttpServerConfig ;
	class HttpServerResources ;
	class HttpServerInput ;
}

/// \class Gv::HttpServerInput
/// An ImageInput class that integrates with the event loop, with automatic
/// re-opening if the channel goes away for a while.
/// 
class Gv::HttpServerInput : public Gv::ImageInput , private GNet::EventHandler
{
public:
	HttpServerInput( Gr::ImageConverter & , const std::string & channel_name , unsigned int reopen_timeout ) ;
		///< Constructor taking a channel name.

	virtual ~HttpServerInput() ;
		///< Destructor.

	void start() ;
		///< Starts the input.

private:
	HttpServerInput( const HttpServerInput & ) ;
	void operator=( const HttpServerInput & ) ;
	void attach() ;
	void detach() ;
	virtual void onException( std::exception & ) override ; // GNet::EventExceptionHandler
	virtual void readEvent() override ; // GNet::EventHandler
	void onTimeout() ;

private:
	int m_fd ;
	unsigned int m_reopen_timeout ;
	GNet::Timer<HttpServerInput> m_reopen_timer ;
} ;

/// \class Gv::HttpServerSource
/// Used by a ImageInputHandler-derived object to hold a ImageInputSource 
/// pointer. The pointer is normally set() by HttpServerSources::select().
/// 
class Gv::HttpServerSource
{
public:
	explicit HttpServerSource( ImageInputHandler & ) ;
		///< Constructor.

	~HttpServerSource() ;
		///< Destructor.

	std::string name() const ;
		///< Returns the source name. In practice this is likely
		///< to be the channel name or the camera device name.

	std::string info() const ;
		///< Returns information on the channel publisher, or the 
		///< empty string for non-channel sources.

	void resend() ;
		///< Calls resend() on the source.

	ImageInputSource * get() ;
		///< Returns the source pointer.

	bool set( ImageInputSource * , const std::string & info ) ;
		///< Sets the source pointer. Used by Gv::HttpServerSources.

private:
	HttpServerSource( const HttpServerSource & ) ;
	void operator=( const HttpServerSource & ) ;

private:
	ImageInputHandler & m_handler ;
	ImageInputSource * m_source ;
	std::string m_info ;
} ;

/// \class Gv::HttpServerSources
/// A container for ImageInputSource pointers, used by Gv::HttpServerPeer.
/// Image sources can be publication channels or not (cf. httpserver
/// vs. webcamplayer). Consequently the pointers are actually pairs of 
/// pointers; one pointing to the base class, and the other possibly-null 
/// pointer pointing at the channel-specific derived class.
/// 
class Gv::HttpServerSources
{
public:
	HttpServerSources( Gr::ImageConverter & , const HttpServerResources & , 
		unsigned int channel_reopen_timeout ) ;
			///< Constructor. The converter reference is kept. The resources 
			///< object simply provides a list of channel names.

	HttpServerSources( Gr::ImageConverter & , ImageInputSource & ) ;
		///< Constructor for a single non-channel source, typically 
		///< a Gv::Camera reference. The references are kept, 
		///< with no transfer of ownership.

	~HttpServerSources() ;
		///< Destructor.

	bool valid( const std::string & url_path ) ;
		///< Returns true if the given url path looks like it is for
		///< an input-source rather than a file.

	bool select( const std::string & url_path , HttpServerSource & ) ;
		///< Looks up the required input source and deposits its pointer 
		///< into the supplied holder. Returns true if the source changes.

	Gr::ImageConverter & converter() ;
		///< Returns the image-converter reference, as passed in to the
		///< constructor.

private:
	typedef std::pair<ImageInputSource*,HttpServerInput*> Pair ;
	HttpServerSources( const HttpServerSources & ) ;
	void operator=( const HttpServerSources & ) ;
	Pair findByName( const std::string & ) ;
	Pair findByNumber( size_t ) ;
	bool addChannel( const std::string & , bool do_throw ) ;

private:
	Gr::ImageConverter & m_converter ;
	const HttpServerResources * m_resources ;
	std::vector<Pair> m_list ;
	unsigned int m_reopen_timeout ;
} ;

/// \class Gv::HttpServerResources
/// A configuration structure for resources that Gv::HttpServerPeer makes
/// available.
/// 
class Gv::HttpServerResources
{
public:
	HttpServerResources() ;
		///< Default constructor.

	void setDirectory( const G::Path & ) ;
		///< Sets a directory for file resources.

	void addFileAny() ;
		///< Allow any file in the directory.

	std::string addFile( const G::Path & ) ;
		///< Adds a file resource. Returns the empty string
		///< or a failure reason.

	void addFileType( const G::Path & , const std::string & type ) ;
		///< Adds a filename-to-type mapping that overrides
		///< the extension mapping.

	void addChannel( const std::string & channel_name ) ;
		///< Adds a channel resource.

	void addChannelAny() ;
		///< Adds the wildcard channel.

	std::string setDefault( const std::string & ) ;
		///< Sets the default resource. Returns a warning string.

	bool empty() const ;
		///< Returns true if just default-constructed with no
		///< "add" or "set" methods called.

	std::vector<std::string> channels() const ;
		///< Returns a list of added channel names.

	bool anyChannel() const ;
		///< Returns true if addChannelAny() called.

	bool fileResource( const G::Path & url_path ) const ;
		///< Returns true if the url path is for a file (eg. "/index.html").

	bool channelResource( const G::Path & url_path ) const ;
		///< Returns true if the url path is for a channel (eg. "/_foo").

	bool specialResource( const G::Path & url_path ) const ;
		///< Returns true if the url path is for a special resource (eg. "/__").

	std::string fileResourcePath( const G::Path & url_path ) const ;
		///< Returns the file-system path for the given url path, or the
		///< empty string if there is no matching resource map entry.
		///< Precondition: fileResource(url_path)

	std::string fileResourceType( const G::Path & url_path ) const ;
		///< Returns the content-type path for the given url path, or the
		///< empty string if there is no matching resource map entry.
		///< Precondition: fileResource(url_path)

	std::pair<std::string,std::string> fileResourcePair( const G::Path & url_path ) const ;
		///< Returns fileResourcePath() and fileResourceType() as a pair.
		///< If not a valid fileResourcePath() then the reason is returned
		///< in the second part.

	std::string channelName( const G::Path & url_path ) const ;
		///< Returns the channel name for the given channel-like url path.
		///< Precondition: channelResource(url_path)

	static std::string guessType( const G::Path & path ) ;
		///< Returns a probable content-type based on the filename,
		///< or the empty string.

	void log() const ;
		///< Emits diagnostic logging for the configured resources.

private:
	enum ResourceType 
	{ 
		t_file , 
		t_channel , 
		t_source , 
		t_special 
	} ;
	HttpServerResources( HttpServerResources & ) ;
	void operator=( HttpServerResources & ) ;
	ResourceType resourceType( const G::Path & url_path ) const ;
	static bool readable( const G::Path & ) ;
	void log( std::ostream & ) const ;
	static G::Path normalise( const G::Path & path_in , const std::string & default_ ) ;
	static G::Path normalise( const G::Path & path_in ) ;

private:
	typedef std::map<std::string,std::string> Map ;
	G::Path m_dir ;
	bool m_any_file ;
	bool m_any_channel ;
	bool m_with_specials ;
	std::string m_default_resource ;
	std::vector<std::string> m_channels ;
	Map m_files ;
	Map m_file_types ;
} ;

/// \class Gv::HttpServerConfig
/// A configuration structure for Gv::HttpServerPeer holding default settings 
/// from the command-line which are then overriden by url parameters.
/// 
class Gv::HttpServerConfig
{
public:
	HttpServerConfig() ;
		///< Default constructor for reasonable default values.

	void init( unsigned int idle_timeout , unsigned int first_image_timeout , G::EpochTime repeat_timeout ,
		unsigned int refresh_header , 
		const GNet::Address & gateway , bool more_verbose ) ;
			///< Initialises the configuration from command-line parameters.

	void init( unsigned int idle_timeout , unsigned int first_image_timeout , G::EpochTime repeat_timeout ,
		unsigned int refresh_header , 
		const std::string & type ) ;
			///< Initialises the configuration from command-line parameters.

	void init( const G::Url & ) ;
		///< Further initialises the configuration from url parameters.

	unsigned int idleTimeout() const ;
		///< Returns the connection idle timeout. This terminates the
		///< connection if no http commands are received, unless
		///< streaming.

	unsigned int firstImageTimeout() const ;
		///< Returns the first-image timeout. If no input image is
		///< received in this period after the http get request then
		///< the get fails with '503 image unavailable'.

	G::EpochTime imageRepeatTimeout() const ;
		///< Returns the repeat timeout. When streaming this is the
		///< maximum time spent waiting for a new input image; if nothing
		///< is available by then then the old image is resent.

	bool moreVerbose() const ;
		///< Returns true for more verbosity.

	std::string type() const ;
		///< Returns the required content-type.

	bool quick() const ;
		///< Returns true if requesting the most-recent data from the channel rather
		///< than waiting for the next update.

	bool streaming() const ;
		///< Returns true if streaming using multipart/x-mixed-replace.

	GNet::Address gateway() const ;
		///< Returns the gateway network address, or the default address if none.

	int scale() const ;
		///< Returns the required scale factor.

	bool monochrome() const ;
		///< Returns the monochrome flag.

	unsigned int refresh() const ;
		///< Returns the value for the http refresh header.

	bool withStatus() const ;
		///< Returns true if the status url is enabled.

private:
	static bool integral( const G::Url & url , const std::string & key ) ;

private:
	bool m_streaming ; // multipart/x-mixed-replace
	unsigned int m_refresh ; // refresh header value if not streaming
	std::string m_type ; // jpeg/raw/pnm
	int m_scale ; // image size scale factor
	bool m_monochrome ; // greyscale image
	unsigned int m_idle_timeout ; // input idle timeout
	unsigned int m_first_image_timeout ; // first-image timeout
	G::EpochTime m_image_repeat_timeout ; // minimum output frame rate, even if no new input images
	GNet::Address m_gateway ;
	bool m_more_verbose ; // log the http protocol
	bool m_quick ; // start with the most recent image available, even if old
} ;

#endif

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
/// \file gvimageoutput.h
///

#ifndef GV_IMAGEOUTPUT__H
#define GV_IMAGEOUTPUT__H

#include "gdef.h"
#include "gpath.h"
#include "gfatpipe.h"
#include "gpublisher.h"
#include "gdatetime.h"
#include "gvtimezone.h"
#include "grimagetype.h"
#include "gtimer.h"
#include <string>

namespace Gv
{
	class ImageOutput ;
}

/// \class Gv::ImageOutput
/// A class for emiting images etc to a fat pipe that feeds a spawned viewer 
/// process, and/or to a publication channel, and/or to a filesystem image 
/// repository.
/// 
class Gv::ImageOutput
{
public:
	ImageOutput() ;
		///< Constructor.

	explicit ImageOutput( GNet::EventExceptionHandler & ) ;
		///< Constructor taking a callback interface for handling 
		///< failed viewer pings.

	~ImageOutput() ;
		///< Destructor.

	void startViewer( const std::string & title = std::string() , unsigned int scale = 1U , 
		const std::string & output_channel = std::string() ) ;
			///< Starts the viewer. Does periodic pings of the viewer's
			///< fat pipe if the relevant constructor overload was used.

	void startPublisher( const std::string & channel ) ;
		///< Starts the publisher channel.

	void startPublisher( const std::string & channel , const G::Item & publisher_info ) ;
		///< Starts the publisher channel with additional infomation describing
		///< the publisher.

	void startPublisher( G::Publisher * ) ;
		///< Starts the publisher channel by taking ownership of a new-ed publisher object.

	void saveTo( const std::string & base_dir , const std::string & name , bool fast , 
		const Gv::Timezone & tz , bool test_mode = false ) ;
			///< Saves images to the filesystem inside a deeply-nested directory 
			///< hierarchy with the given base directory. Every filename is prefixed
			///< with the given name. Directory paths are derived from a timestamp; 
			///< if the 'fast' flag is true then sub-second parts of the timestamp
			///< are used.

	bool viewing() const ;
		///< Returns true if startViewer() has been called.

	bool pingViewer() ;
		///< Returns true if the viewer seems to be running. The transition
		///< from true to false can be used to terminate the caller's event 
		///< loop.

	void pingViewerCheck() ;
		///< Checks that the viewer is running if it should be (see pingViewer()), 
		///< and throws an exeception if not.

	G::Path send( const char * data , size_t size , Gr::ImageType type , G::EpochTime = G::EpochTime(0) ) ;
		///< Emits an image. Returns the path if saveTo()d the filesystem.

	G::Path send( const Gr::ImageBuffer & data , Gr::ImageType type , G::EpochTime = G::EpochTime(0) ) ;
		///< Emits an image. Returns the path if saveTo()d the filesystem.

	void sendText( const char * data , size_t size , const std::string & type ) ;
		///< Emits a non-image message. Non-image messages are not saveTo()d the filesystem.

	const std::string & dir() const ;
		///< Returns the saveTo() path.

	static void path( std::string & out , const std::string & base_dir , const std::string & name ,
		G::EpochTime , Gr::ImageType type , bool fast , const Gv::Timezone & tz , bool test_mode ) ;
			///< Returns by reference the filesystem path corresponding
			///< to the given base directory and timestamp.

	std::string path( G::EpochTime , Gr::ImageType type , bool fast ) const ;
		///< Returns the filesystem path corresponding to the given
		///< timestamp.

	bool fast() const ;
		///< Returns the saveTo() fast flag.

private:
	ImageOutput( const ImageOutput & ) ;
	void operator=( const ImageOutput & ) ;
	G::Path save( const char * p , size_t n , Gr::ImageType , G::EpochTime ) ;
	G::Path save( const Gr::ImageBuffer & , Gr::ImageType , G::EpochTime ) ;
	G::Path openFile( std::ofstream & , Gr::ImageType , G::EpochTime ) ;
	void commitFile( std::ofstream & , const G::Path & ) ;
	static std::string viewer( const G::Path & ) ;
	static std::string sanitise( const std::string & ) ;
	void onPingTimeout() ;

private:
	unique_ptr<GNet::Timer<ImageOutput> > m_ping_timer_ptr ;
	std::string m_base_dir ;
	std::string m_name ;
	bool m_fast ;
	Gv::Timezone m_tz ;
	bool m_save_test_mode ;
	unique_ptr<G::Publisher> m_publisher ;
	unique_ptr<G::FatPipe> m_fat_pipe ;
	G::Path m_old_path ;
	G::Path m_old_dir ;
	bool m_viewer_up ;
} ;

#endif

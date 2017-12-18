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
/// \file gpublisher.h
///

#ifndef G_PUBLISHER__H
#define G_PUBLISHER__H

#include "gdef.h"
#include "gsharedmemory.h"
#include "gdatetime.h"
#include "gitem.h"
#include "gsemaphore.h"
#include "gexception.h"
#include "gsignalsafe.h"
#include <utility> // pair
#include <string>
#include <vector>

namespace G
{
	class Publisher ;
	class PublisherChannel ;
	class PublisherSubscription ;
	class PublisherSubscriber ;
	class PublisherInfo ;
	G_EXCEPTION( PublisherError , "publish/subscribe error" ) ;
}

/// \class G::Publisher
/// A broadcast communication channel between unrelated processes using shared 
/// memory. The shared memory name is public so that subscribers can find it 
/// (see shm_open()). Subscribers are then notified via individual unix-domain 
/// sockets.
/// 
/// Communication is message-based and unreliable; newer messages will overwrite 
/// older ones if not consumed. Messages have an associated type name.
/// 
/// There are two shared memory segments per channel; one control and one data.
/// This allows the data segment to be discarded and replaced if it is too small.
/// 
/// The named sockets are created by the subscribers, their name is put into the
/// shared memory segment, and they are unlinked by the publisher on first use. 
/// Consequently, sockets exist in the filesystem in the interval between 
/// binding by the subscriber and the next publish event in the publisher.
/// 
/// As the publisher goes away it marks the shared memory as defunct, notifies 
/// subscribers one last time, and unlinks the shared memory from the 
/// filesystem. It also unlinks any sockets that have not been previously 
/// unlinked.
/// 
class G::Publisher
{
public:
	explicit Publisher( const std::string & channel_name , bool auto_cleanup = true ) ;
		///< Constructor for a publisher.

	Publisher( const std::string & channel_name , Item publisher_info , bool auto_cleanup = true ) ;
		///< A constructor overload including arbitrary "info" metadata that
		///< is stored in the channel (subject to buffer size limits).
		///< The metadata is enriched with a "type" field that is
		///< initialised with the basename of argv0.

	~Publisher() ;
		///< Destructor. Marks the shared memory as defunct and sends out 
		///< notification to subscribers causing their receive()s to return 
		///< false. 
		///< 
		///< Any recent subscribers that have never been publish()ed to before 
		///< will get a notification and as a result their named sockets will 
		///< be unlink()d. This means that the subscribers do not have to do 
		///< any special filesystem cleanup themselves.

	void publish( const char * p , size_t n , const char * type = nullptr ) ;
		///< Publishes a chunk of data to subscribers.

	void publish( const std::vector<char> & , const char * type = nullptr ) ;
		///< Publishes a chunk of data to subscribers.

	void publish( const std::vector<std::pair<const char*,size_t> > & , const char * type ) ;
		///< Publishes segmented data to subscribers.

	void publish( const std::vector<std::vector<char> > & , const char * type ) ;
		///< Publishes chunked data to subscribers.

	static std::vector<std::string> list( std::vector<std::string> * others = nullptr ) ;
		///< Returns a list of channel names. Optionally returns by reference
		///< a list of visible-but-unreadable channels.

	static void purge( const std::string & channel_name ) ;
		///< Tries to clean up any failed slots in the named channel. 
		///< This should only be run administratively because it is the
		///< subscribers' job to clean up after themselves.

	static std::string delete_( const std::string & channel_name ) ;
		///< Deletes the channel. This should only be run administratively.
		///< Returns a failure reason on error.

	static Item info( const std::string & channel_name , bool all_slots = true ) ;
		///< Returns a variant containing information about the state 
		///< of the channel. The 'publisher' sub-item is the metadata
		///< from the constructor, enriched with a "pid" field that
		///< is the publisher's process id.

private:
	Publisher( const Publisher & ) ;
	void operator=( const Publisher & ) ;
	void publishStart() ;
	void publishPart() ;
	void publishEnd() ;

private:
	std::string m_name ;
	SharedMemory m_shmem_control ;
	unique_ptr<SharedMemory> m_shmem_data ;
	unique_ptr<PublisherInfo> m_info ;
	std::vector<const char*> m_data_p ;
	std::vector<size_t> m_data_n ;
} ;

/// \class G::PublisherChannel
/// A named publisher channel that can be subscribed to.
/// 
class G::PublisherChannel
{
public:
	explicit PublisherChannel( const std::string & channel_name , 
		const std::string & socket_path_prefix = std::string() ) ;
			///< Constructor. 
			///< 
			///< The socket path prefix should be an absolute path with 
			///< a basename; the actual path will have ".<pid>" added. 
			///< The default is "/tmp/<channel_name>".
			///< 
			///< The implementation maps the publisher's 'control' 
			///< memory segment.

	~PublisherChannel() ;
		///< Destructor.

	PublisherSubscriber * subscribe() ;
		///< Subscribes to the publisher returning a new()ed object.
		///< Notification is via a named socket. The actual socket 
		///< name is the given prefix (or some sensible default)
		///< plus ".<pid>". The lifetime of the subscriber object 
		///< must be less than the lifetime of the channel.
		///< 
		///< The implementation creates a socket, grabs a free slot
		///< in the control memory segment, and installs the socket
		///< file descriptor in the slot. The slot and the socket
		///< are freed in the PublisherSubscriber's destructor.

	bool receive( size_t , int , std::vector<char> & , std::string * = nullptr , G::EpochTime * = nullptr ) ; 
		///< Used by PublisherSubscriber.

	void releaseSlot( size_t slot_id ) ;
		///< Used by PublisherSubscriber.

	std::string name() const ;
		///< Returns the channel name, as passed in to the constructor.

private:
	PublisherChannel( const PublisherChannel & ) ;
	void operator=( const PublisherChannel & ) ;

private:
	std::string m_name ;
	SharedMemory m_shmem_control ;
	unique_ptr<SharedMemory> m_shmem_data ;
	std::string m_path_prefix ;
} ;

/// \class G::PublisherSubscriber 
/// A publication-channel subscriber endpoint.
/// 
class G::PublisherSubscriber
{ 
public:
	PublisherSubscriber( PublisherChannel & , size_t slot_id , int socket_fd ) ;
		///< Pseudo-private constructor, used by G::PublisherChannel.

	~PublisherSubscriber() ;
		///< Destructor.

	size_t id() const ;
		///< Returns the slot id.

	int fd() const ;
		///< Returns the named socket file descriptor.

	bool receive( std::vector<char> & buffer , std::string * type_p = nullptr , G::EpochTime * time_p = nullptr ) ; 
		///< Does a read for new publish()ed data. Blocks if there is nothing 
		///< to read. Returns false if the publisher has gone away. Returns 
		///< an empty buffer in the case of an ignorable event.

	bool peek( std::vector<char> & buffer , std::string * type_p = nullptr , G::EpochTime * time_p = nullptr ) ; 
		///< Does a receive() but without requiring a publication event.
		///< Returns false if no data is available.

private:
	PublisherSubscriber( const PublisherSubscriber & ) ;
	void operator=( const PublisherSubscriber & ) ;

private:
	PublisherChannel & m_channel ;
	size_t m_slot_id ; 
	int m_socket_fd ;
} ;

/// \class G::PublisherSubscription
/// An easy-to-use combination of a G::PublisherChannel object
/// and a single G::PublisherSubscriber.
/// 
class G::PublisherSubscription
{ 
public:
	explicit PublisherSubscription( const std::string & channel_name , bool lazy = false ) ;
		///< Constructor. The channel is normally subscribed to immediately, 
		///< with errors resulting in exceptions; but if the lazy option is 
		///< used then then initialisation happens in init() and the constructor 
		///< will not throw.
		///< 
		///< Channel names are simple names (eg. "foo"), but at this interface 
		///< they can be overloaded with the socket prefix, eg. "/var/tmp/foo.s/foo".

	bool open() ;
		///< Tries to initialise the object after lazy construction or
		///< after close(). Returns true when successfully initialised.
		///< Does not throw.

	int fd() const ;
		///< Returns the subscriber's file descriptor or minus one.

	bool receive( std::vector<char> & buffer , std::string * type_p = nullptr , G::EpochTime * time_p = nullptr ) ; 
		///< Does a read for new publish()ed data. Blocks if there is 
		///< nothing to read. Returns false if the publisher has gone 
		///< away. Returns an empty buffer in the case of an ignorable 
		///< event.

	bool peek( std::vector<char> & buffer , std::string * type_p = nullptr , G::EpochTime * time_p = nullptr ) ; 
		///< Does a receive() but without requiring a publication event.

	std::string name() const ;
		///< Returns the channel name, as passed in to the constructor.

	std::string info() const ;
		///< Returns some channel metadata as a short json string (see
		///< G::Item::str()). This is the 'publisher' subset of Publisher::info(),
		///< so originally passed in to the Publisher constructor, plus some 
		///< enrichment with "type" and "pid".

	unsigned int age() const ;
		///< Returns the age of the latest data in seconds, or zero.
		///< The implementation uses peek().

	void close() ;
		///< Closes the channel subscription, releasing resources and
		///< becoming inactive. The fd() method will return -1, 
		///< although name() remains valid.

private:
	PublisherSubscription( const PublisherSubscription & ) ;
	void operator=( const PublisherSubscription & ) ;

private:
	std::string m_channel_name ;
	std::string m_channel_info ;
	bool m_lazy ;
	bool m_closed ;
	std::string m_path_prefix ;
	unique_ptr<PublisherChannel> m_channel ; // first
	unique_ptr<PublisherSubscriber> m_subscriber ; // second
} ;

#endif

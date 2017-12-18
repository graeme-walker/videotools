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
// gpublisher.cpp
//

#include "gdef.h"
#include "gpublisher.h"
#include "glocalsocket.h"
#include "gprocess.h"
#include "gmsg.h"
#include "gfile.h"
#include "gdatetime.h"
#include "gstr.h"
#include "groot.h"
#include "gpath.h"
#include "garg.h"
#include "gcleanup.h"
#include "gassert.h"
#include "glog.h"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

namespace G
{
	/// \namespace G::PublisherImp
	/// A private scope used in the implementation of G::Publisher and friends.
	///
	namespace PublisherImp
	{
		struct DataMemory ;
		struct ControlMemory ;
		struct Slot ;
		struct InfoSlot ;
		struct Lock ;
		struct SocketHolder ;
		struct Snapshot ;
		enum { ERRORS = 5 } ;
		enum { SLOTS = 10 } ;
		enum { MAGIC = 0xa5a5 } ;
		enum { e_connect , e_unlink , e_send } ; // error[] index
		enum { socket_path_size = 200 } ;
		struct path_t /// A character array for storing filesystem paths in shared memory.
		{ 
			char s[socket_path_size] ; 
		} ;

		static void initialise( SharedMemory & , const std::string & , const G::Item & , bool ) ;
		static void check( SharedMemory & , const std::string & ) ;
		static std::string check( const std::string & ) ;
		static std::string checkName( const std::string & ) ;
		static void deactivate( SignalSafe , void * ) ;
		static void nop( SignalSafe , void * ) ;
		static void releaseSlots( SignalSafe signal_safe , void * ) ;
		static void closeAllSlots( SignalSafe , ControlMemory * mem ) ;
		static void closeSlot( SignalSafe , Slot & ) ;
		static void clearSlot( SignalSafe , Slot & ) ;
		static void notifyAll( SignalSafe , ControlMemory * , PublisherInfo & ) ;
		static void notifyOne( SignalSafe , Slot & , InfoSlot & ) ;
		static void cleanupControlMemory( SignalSafe , const char * ) ;
		static void cleanupDataMemory( SignalSafe , const char * ) ;
		static void cleanupProcess( SignalSafe signal_safe , const char * ) ;
		static Snapshot snapshot( const std::string & ) ;
		static void createSocket( SocketHolder & holder , const std::string & path_prefix ) ;
		static size_t findFreeSlot( ControlMemory & ) ;
		static void claimSlot( Slot & , const std::string & , pid_t ) ;
		static void publish( SharedMemory & , unique_ptr<SharedMemory> & , PublisherInfo & , const std::string & ,
			size_t data_total , size_t data_count , const char ** data_p_p , size_t * data_n_p , const char * type ) ;
		static size_t subscribe( SharedMemory & shmem_control , SocketHolder & , const std::string & ) ;
		static void releaseSlot( SharedMemory & shmem_control , size_t slot_id ) ;
		static void releaseSlot( SignalSafe , Lock & , Slot & ) ;
		static size_t flush( int socket_fd ) ;
		static bool getch( int socket_fd ) ;
		static bool receive( SharedMemory & shmem_control , unique_ptr<SharedMemory> & shmem_data ,
			const std::string & , size_t slot_id , int socket_fd , std::vector<char> & buffer , 
			std::string * type_p , G::EpochTime * time_p ) ;
		static std::vector<std::string> list( std::vector<std::string> * ) ;
		static G::Item info( const std::string & channel_name , bool detail , bool all_slots ) ;
		static void purge( const std::string & channel_name ) ;
		static std::string delete_( const std::string & channel_name ) ;
		static char * strdup_ignore_leak( const char * ) ;
		static void strset( char * p , size_t n , const std::string & s_in ) ;
		static void strget( const char * p_in , size_t n_in , std::string & s_out ) ;
		static std::string strget( const char * p_in , size_t n_in ) ;
		static size_t morethan( size_t ) ;
	}
} ;

/// \class G::PublisherImp::Slot
/// A shared-memory structure used by G::Publisher.
///
struct G::PublisherImp::Slot
{
	bool in_use ; // subscriber lock
	bool failed ; // publisher failure
	pid_t pid ; // subscriber's pid
	unsigned long seq ;
	int socket_fd ; // publisher's fd
	int error[ERRORS] ; // publisher errno set
	path_t socket_path ;
} ;

/// \class G::PublisherImp::ControlMemory
/// A shared-memory structure used by G::Publisher.
///
struct G::PublisherImp::ControlMemory
{
	int magic ;
	Semaphore::storage_type mutex ;
	pid_t publisher_pid ;
	char publisher_info[2048] ; // eg. json
	unsigned long seq ;
	Slot slot[SLOTS] ;
} ;

/// \class G::PublisherImp::DataMemory
/// A shared-memory structure used by G::Publisher.
///
struct G::PublisherImp::DataMemory
{
	size_t size_limit ;
	char type[60] ;
	time_t time_s ;
	g_uint32_t time_us ;
	size_t data_size ;
	char data[1] ;
} ;

/// \class G::PublisherImp::Lock
/// A RAII semaphore lock class used by G::PublisherImp.
///
struct G::PublisherImp::Lock
{
	explicit Lock( Semaphore * s ) ;
	~Lock() ;
	//
	Semaphore * m_s ;
} ;

/// \class G::PublisherImp::SocketHolder
/// A private implementation class used by G::PublisherImp.
///
struct G::PublisherImp::SocketHolder
{
	explicit SocketHolder( int = -1 ) ;
	int release() ;
	~SocketHolder() ;
	//
	int fd ;
	std::string path ;
} ;

/// \class G::PublisherImp::Snapshot
/// A private implementation class used by G::PublisherImp.
///
struct G::PublisherImp::Snapshot
{
	ControlMemory control ;
	bool has_data ;
	DataMemory data ;
} ;

/// \class G::PublisherImp::InfoSlot
/// A structure used by G::Publisher, having a sub-set of G::PublisherImp::Slot 
/// members, used for diagnostics and error reporting. This structure is
/// modified by low-level code while the shared-memory is locked, but used 
/// for error reporting without any locking.
///
struct G::PublisherImp::InfoSlot
{
	InfoSlot() ;
	int e() const ;
	std::string path() const ;
	void report() ;
	//
	bool failed ;
	int error[ERRORS] ;
	path_t socket_path ;
} ;

/// \class G::PublisherInfo
/// A structure that holds diagnostic information for each G::Publisher slot.
///
struct G::PublisherInfo
{
	void clear( SignalSafe ) ;
	//
	PublisherImp::InfoSlot slot[PublisherImp::SLOTS] ;
} ;

// ==

G::Publisher::Publisher( const std::string & name , bool auto_cleanup ) :
	m_name(PublisherImp::checkName(name)) ,
	m_shmem_control(name,sizeof(PublisherImp::ControlMemory),SharedMemory::Control()) ,
	m_info(new PublisherInfo)
{
	G::Item info = G::Item::map() ;
	info.add( "type" , G::Path(G::Arg::v0()).basename() ) ;
	PublisherImp::initialise( m_shmem_control , name , info , auto_cleanup ) ;
}

G::Publisher::Publisher( const std::string & name , G::Item info , bool auto_cleanup ) :
	m_name(PublisherImp::checkName(name)) ,
	m_shmem_control(name,sizeof(PublisherImp::ControlMemory),SharedMemory::Control()) ,
	m_info(new PublisherInfo)
{
	if( info.type() == G::Item::t_map && !info.has("type") )
		info.add( "type" , G::Path(G::Arg::v0()).basename() ) ;
	PublisherImp::initialise( m_shmem_control , name , info , auto_cleanup ) ;
}

G::Publisher::~Publisher()
{
	PublisherImp::deactivate( SignalSafe() , m_shmem_control.ptr() ) ;
}

void G::Publisher::publish( const char * data_p , size_t data_n , const char * type )
{
	if( data_p == nullptr || data_n == 0U ) return ;
	m_data_p.resize( 1U ) ;
	m_data_n.resize( 1U ) ;
	m_data_p[0] = data_p ;
	m_data_n[0] = data_n ;
	PublisherImp::publish( m_shmem_control , m_shmem_data , *m_info.get() , m_name , data_n , 1U , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::Publisher::publish( const std::vector<char> & data , const char * type )
{
	if( data.empty() ) return ;
	m_data_p.resize( 1U ) ;
	m_data_n.resize( 1U ) ;
	m_data_p[0] = &data[0] ;
	m_data_n[0] = data.size() ;
	PublisherImp::publish( m_shmem_control , m_shmem_data , *m_info.get() , m_name , data.size() , 1U , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::Publisher::publish( const std::vector<std::pair<const char *,size_t> > & data , const char * type )
{
	m_data_p.resize( data.size() ) ;
	m_data_n.resize( data.size() ) ;
	size_t data_total = 0U ;
	for( size_t i = 0U ; i < data.size() ; i++ )
	{
		m_data_p[i] = data[i].first ;
		m_data_n[i] = data[i].second ;
		data_total += data[i].second ;
	}
	if( data_total != 0U )
		PublisherImp::publish( m_shmem_control , m_shmem_data , *m_info.get() , m_name , data_total , data.size() , &m_data_p[0] , &m_data_n[0] , type ) ;
}

void G::Publisher::publish( const std::vector<std::vector<char> > & data , const char * type )
{
	m_data_p.resize( data.size() ) ;
	m_data_n.resize( data.size() ) ;
	size_t data_total = 0U ;
	size_t i_out = 0U ;
	for( size_t i_in = 0U ; i_in < data.size() ; i_in++ )
	{
		if( data[i_in].empty() ) continue ;
		m_data_p[i_out] = &(data[i_in])[0] ;
		m_data_n[i_out++] = data[i_in].size() ;
		data_total += data[i_in].size() ;
	}
	if( data_total != 0U )
		PublisherImp::publish( m_shmem_control , m_shmem_data , *m_info.get() , m_name , data_total , i_out , &m_data_p[0] , &m_data_n[0] , type ) ;
}

std::vector<std::string> G::Publisher::list( std::vector<std::string> * others )
{
	return PublisherImp::list( others ) ;
}

G::Item G::Publisher::info( const std::string & channel_name , bool all_slots )
{
	return PublisherImp::info( channel_name , true , all_slots ) ;
}

void G::Publisher::purge( const std::string & channel_name )
{
	PublisherImp::purge( channel_name ) ;
}

std::string G::Publisher::delete_( const std::string & channel_name )
{
	return PublisherImp::delete_( channel_name ) ;
}

// ==

G::PublisherChannel::PublisherChannel( const std::string & name , const std::string & path_prefix ) :
	m_name(PublisherImp::checkName(name)) ,
	m_shmem_control(name,SharedMemory::Control()) ,
	m_path_prefix(path_prefix)
{
	if( m_path_prefix.empty() )
		m_path_prefix = std::string("/tmp/") + SharedMemory::prefix() + name ; // "/tmp/xx-name"

	PublisherImp::check( m_shmem_control , name ) ;
	Cleanup::add( PublisherImp::cleanupProcess , 
		PublisherImp::strdup_ignore_leak(SharedMemory::osName(m_name).c_str()) ) ;
}

G::PublisherChannel::~PublisherChannel()
{
}

G::PublisherSubscriber * G::PublisherChannel::subscribe()
{
	PublisherImp::SocketHolder socket_holder ;
	PublisherImp::createSocket( socket_holder , m_path_prefix ) ;
	size_t slot_id = PublisherImp::subscribe( m_shmem_control , socket_holder , m_name ) ;
	return new PublisherSubscriber( *this , slot_id , socket_holder.release() ) ;
}

void G::PublisherChannel::releaseSlot( size_t slot_id )
{
	PublisherImp::releaseSlot( m_shmem_control , slot_id ) ;
}

bool G::PublisherChannel::receive( size_t slot_id , int socket_fd , std::vector<char> & buffer , 
	std::string * type_p , G::EpochTime * time_p )
{
	return PublisherImp::receive( m_shmem_control , m_shmem_data , m_name , slot_id , socket_fd , buffer , type_p , time_p ) ;
}

std::string G::PublisherChannel::name() const
{
	return m_name ;
}

// ==

G::PublisherSubscriber::PublisherSubscriber( PublisherChannel & channel , size_t slot_id , int socket_fd ) :
	m_channel(channel) ,
	m_slot_id(slot_id) ,
	m_socket_fd(socket_fd)
{
	G_ASSERT( slot_id < PublisherImp::SLOTS ) ;
	G_ASSERT( socket_fd >= 0 ) ;
}

G::PublisherSubscriber::~PublisherSubscriber()
{
	m_channel.releaseSlot( m_slot_id ) ;
	::close( m_socket_fd ) ;
}

size_t G::PublisherSubscriber::id() const
{
	return m_slot_id ;
}

int G::PublisherSubscriber::fd() const
{
	return m_socket_fd ;
}

bool G::PublisherSubscriber::receive( std::vector<char> & buffer , std::string * type_p , G::EpochTime * time_p )
{
	return m_channel.receive( m_slot_id , m_socket_fd , buffer , type_p , time_p ) ;
}

bool G::PublisherSubscriber::peek( std::vector<char> & buffer , std::string * type_p , G::EpochTime * time_p )
{
	try
	{
		bool ok = m_channel.receive( m_slot_id , -1 , buffer , type_p , time_p ) ;
		return ok && !buffer.empty() ;
	}
	catch( SharedMemory::Error & )
	{
		// eg. nothing ever published, so no data segment
		return false ;
	}
}

// ==

G::PublisherSubscription::PublisherSubscription( const std::string & channel_name , bool lazy ) :
	m_channel_name(channel_name) ,
	m_lazy(lazy) ,
	m_closed(false)
{
	m_channel_name = Path(channel_name).basename() ;
	m_path_prefix = Path(channel_name).dirname().str() ;

	// reject eg. "/tmp/channel" -- must be eg. "/tmp/xx-name/channel"
	if( !m_path_prefix.empty() && 
		( Path(m_path_prefix).dirname() == Path() || Path(m_path_prefix).dirname().dirname() == Path() ) )
			throw PublisherError( "not enough directory parts in channel path prefix: [" + m_path_prefix + "]" ) ;

	if( !lazy )
	{
		try
		{
			m_channel.reset( new PublisherChannel(m_channel_name,m_path_prefix) ) ;
			m_subscriber.reset( m_channel->subscribe() ) ;
			m_channel_info = PublisherImp::info(m_channel_name,false,false).str() ;
		}
		catch( SharedMemory::Error & e )
		{
			throw PublisherError( "cannot subscribe to channel [" + channel_name + "]: " + std::string(e.what()) ) ;
		}
	}
}

bool G::PublisherSubscription::open()
{
	bool opened = false ;
	if( !m_channel_name.empty() && m_channel.get() == nullptr )
	{
		try
		{
			m_channel.reset( new PublisherChannel(m_channel_name,m_path_prefix) ) ;
			m_subscriber.reset( m_channel->subscribe() ) ;
			m_channel_info = PublisherImp::info(m_channel_name,false,false).str() ;
			opened = true ;
		}
		catch( PublisherError & e )
		{
		}
		catch( SharedMemory::Error & e )
		{
		}
		if( !opened )
		{
			G_ASSERT( m_subscriber.get() == nullptr ) ;
			m_channel.reset() ;
		}
	}
	return opened ;
}

int G::PublisherSubscription::fd() const
{
	return m_subscriber ? m_subscriber->fd() : -1 ;
}

bool G::PublisherSubscription::receive( std::vector<char> & buffer , std::string * type_p , G::EpochTime * time_p )
{
	return m_subscriber ? m_subscriber->receive(buffer,type_p,time_p) : false ;
}

bool G::PublisherSubscription::peek( std::vector<char> & buffer , std::string * type_p , G::EpochTime * time_p )
{
	return m_subscriber ? m_subscriber->peek(buffer,type_p,time_p) : false ;
}

std::string G::PublisherSubscription::name() const
{
	return m_channel_name ;
}

std::string G::PublisherSubscription::info() const
{
	return m_channel_info ;
}

unsigned int G::PublisherSubscription::age() const
{
	std::vector<char> buffer ;
	G::EpochTime time( 0 ) ;
	G::EpochTime now = G::DateTime::now() ;
	if( const_cast<PublisherSubscription*>(this)->peek(buffer,nullptr,&time) && now > time )
		return static_cast<unsigned int>((now-time).s) ;
	else
		return 0U ;
}

void G::PublisherSubscription::close()
{
	m_closed = true ;
	m_subscriber.reset() ; // first
	m_channel.reset() ; // second
}

// ==

void G::PublisherInfo::clear( SignalSafe )
{
	for( size_t i = 0U ; i < PublisherImp::SLOTS ; i++ )
		slot[i] = PublisherImp::InfoSlot() ;
}

// ==

void G::PublisherImp::initialise( SharedMemory & shmem_control , const std::string & channel_name , 
	const G::Item & info , bool auto_cleanup )
{
	// initialise the shared memory (without locking)
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	{
		mem->publisher_pid = ::getpid() ;
		strset( mem->publisher_info , sizeof(mem->publisher_info) , info.str() ) ;
		mem->seq = 1 ;
		for( size_t i = 0U ; i < SLOTS ; i++ )
			clearSlot( SignalSafe() , mem->slot[i] ) ;
		{
			Root claim_root ;
			new (&mem->mutex) Semaphore ;
		}
		mem->magic = MAGIC ; // last
	}

	// add signal handlers for cleanup
	if( auto_cleanup )
	{
		Cleanup::add( cleanupControlMemory , strdup_ignore_leak(SharedMemory::osName(channel_name).c_str()) ) ;
		Cleanup::add( cleanupDataMemory , strdup_ignore_leak(SharedMemory::osName(channel_name+".d").c_str()) ) ;
	}
}

std::string G::PublisherImp::checkName( const std::string & name )
{
	if( G::Str::printable(name) != name || name.find_first_of("/\\*?") != std::string::npos || name.find('_') == 0U )
		throw PublisherError( "invalid channel name: special characters disallowed" , G::Str::printable(name) ) ;
	return name ;
}

void G::PublisherImp::check( SharedMemory & shmem_control , const std::string & name )
{
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	if( shmem_control.size() != sizeof(ControlMemory) || mem->magic != MAGIC )
	{
		G_DEBUG( "G::PublisherImp::check: not valid as a publisher control segment: [" << name << "] (" << shmem_control.size() << ")" ) ;
		throw PublisherError( "invalid shared memory segment: [" + SharedMemory::osName(name) + "]" ) ;
	}
}

void G::PublisherImp::cleanupControlMemory( SignalSafe signal_safe , const char * name )
{
	SharedMemory::cleanup( signal_safe , name , deactivate ) ;
}

void G::PublisherImp::cleanupDataMemory( SignalSafe signal_safe , const char * name )
{
	SharedMemory::cleanup( signal_safe , name , nop ) ;
}

void G::PublisherImp::cleanupProcess( SignalSafe signal_safe , const char * name )
{
	SharedMemory::trap( signal_safe , name , releaseSlots ) ;
}

void G::PublisherImp::nop( SignalSafe , void * )
{
}

void G::PublisherImp::releaseSlots( SignalSafe signal_safe , void * p )
{
	pid_t pid = ::getpid() ;
	ControlMemory * mem = static_cast<ControlMemory*>(p) ;
	{
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		for( size_t i = 0U ; i < SLOTS ; i++ )
		{
			if( mem->slot[i].in_use && mem->slot[i].pid == pid )
			{
				mem->slot[i].in_use = false ;
				mem->slot[i].pid = 0 ;
				mem->slot[i].socket_path.s[0] = '\0' ;
			}
		}
	}
}

void G::PublisherImp::deactivate( SignalSafe signal_safe , void * p )
{
	static PublisherInfo info ;
	ControlMemory * mem = static_cast<ControlMemory*>(p) ;
	{
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		mem->magic = 0 ; // mark as defunct
		notifyAll( signal_safe , mem , info ) ;
		closeAllSlots( signal_safe , mem ) ;
	}
}

void G::PublisherImp::closeAllSlots( SignalSafe signal_safe , ControlMemory * mem )
{
	if( mem->publisher_pid == ::getpid() )
	{
		for( size_t i = 0U ; i < SLOTS ; i++ )
			closeSlot( signal_safe , mem->slot[i] ) ;
	}
}

void G::PublisherImp::closeSlot( SignalSafe , Slot & slot )
{
	if( slot.socket_fd != -1 )
	{
		::close( slot.socket_fd ) ;
		slot.socket_fd = -1 ;
	}
}

void G::PublisherImp::publish( SharedMemory & shmem_control , unique_ptr<SharedMemory> & shmem_data ,
	PublisherInfo & info , const std::string & channel_name ,
	size_t data_total , size_t data_count , const char ** data_p_p , size_t * data_n_p , 
	const char * type )
{
	// lazy construction of the data segment once we know an appropriate size --
	// subscribers will not try to access the data segment until we have sent
	// out the first event
	//
	if( shmem_data.get() == nullptr )
	{
		const size_t size_limit = morethan( data_total ) ;
		shmem_data.reset( new SharedMemory(channel_name+".d",size_limit+sizeof(DataMemory)) ) ;
		DataMemory * dmem = static_cast<DataMemory*>(shmem_data->ptr()) ;
		dmem->size_limit = size_limit ;
		dmem->type[0] = '\0' ;
		dmem->data_size = 0U ;
		dmem->time_s = 0 ;
		dmem->time_us = 0U ;
	}

	// publish to all subscribers -- copy the payload into the data segment and notify-all
	//
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	{
		G::EpochTime time = G::DateTime::now() ;
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		DataMemory * dmem = static_cast<DataMemory*>(shmem_data->ptr()) ;
		if( data_total > dmem->size_limit )
		{
			const size_t new_size_limit = morethan( data_total ) ;
			shmem_data->remap( new_size_limit + sizeof(DataMemory) , true ) ;
			dmem = static_cast<DataMemory*>(shmem_data->ptr()) ;
			dmem->size_limit = new_size_limit ;
		}
		mem->seq++ ; if( mem->seq == 0U ) mem->seq = 1UL ;
		dmem->data_size = data_total ;
		dmem->time_s = time.s ;
		dmem->time_us = time.us ;
		::memset( dmem->type , 0 , sizeof(dmem->type) ) ;
		if( type != nullptr ) ::strncpy( dmem->type , type , sizeof(dmem->type)-1U ) ;
		for( char * out = dmem->data ; data_count ; out += *data_n_p , data_count-- , data_p_p++ , data_n_p++ )
			::memcpy( out , *data_p_p , *data_n_p ) ;
		notifyAll( SignalSafe() , mem , info ) ;
	}

	// report errors
	//
	for( size_t i = 0U ; i < SLOTS ; i++ )
	{
		if( info.slot[i].failed )
			info.slot[i].report() ;
	}
}

void G::PublisherImp::notifyAll( SignalSafe signal_safe , ControlMemory * mem , PublisherInfo & info )
{
	info.clear( signal_safe ) ;
	for( size_t i = 0U ; i < SLOTS ; i++ )
	{
		if( mem->slot[i].in_use && !mem->slot[i].failed )
		{
			notifyOne( signal_safe , mem->slot[i] , info.slot[i] ) ;
		}
		else if( mem->slot[i].socket_fd != -1 )
		{
			// tidy up after subscriber has gone away
			::close( mem->slot[i].socket_fd ) ;
			mem->slot[i].socket_fd = -1 ;
			clearSlot( signal_safe , mem->slot[i] ) ;
		}
	}
}

void G::PublisherImp::notifyOne( SignalSafe signal_safe , Slot & slot , InfoSlot & info_slot )
{
	// connect to the subscriber if necessary
	if( slot.socket_fd == -1 && !slot.failed ) // ie. newly in_use
	{
		// connect to the new subscriber
		slot.socket_fd = ::socket( AF_UNIX , SOCK_DGRAM , 0 ) ;
		slot.socket_path.s[sizeof(slot.socket_path.s)-1U] = '\0' ;
		LocalSocket::Address address( signal_safe , slot.socket_path.s ) ;
		Identity identity = Root::start( signal_safe ) ;

		int rc = ::connect( slot.socket_fd , address.p() , address.size() ) ; 
		if( rc < 0 ) 
		{
			int e = errno ;
			slot.error[e_connect] = info_slot.error[e_connect] = e ;
		}

		rc = ::unlink( slot.socket_path.s ) ;
		if( rc < 0 ) 
		{
			int e = errno ;
			slot.error[e_unlink] = info_slot.error[e_unlink] = e ;
		}

		Root::stop( signal_safe , identity ) ;
	}

	// poke the subscriber - dont use G::Msg::send() in case it is not signal-handler-safe
	char msg = '.' ;
	int rc = ::send( slot.socket_fd , &msg , 1 , MSG_NOSIGNAL | MSG_DONTWAIT ) ; 
	if( rc < 0 ) 
	{
		int e = errno ;
		slot.error[e_send] = info_slot.error[e_send] = e ;
	}

	// record fatal errors -- keep the slot in_use because it is the subscriber's 
	// responsibility to release it
	if( slot.error[e_connect] || G::Msg::fatal(slot.error[e_send]) )
	{
		slot.failed = info_slot.failed = true ;
		info_slot.socket_path = slot.socket_path ;
		::close( slot.socket_fd ) ;
		slot.socket_fd = -1 ;
	}
}

void G::PublisherImp::createSocket( SocketHolder & holder , const std::string & path_prefix )
{
	// build the socket path, including the pid
	std::stringstream ss ;
	ss << path_prefix << "." << ::getpid() ;
	holder.path = ss.str() ;
	if( holder.path.length() >= PublisherImp::socket_path_size )
		throw PublisherError( "socket path too long" ) ;

	// create the socket
	holder.fd = ::socket( AF_UNIX , SOCK_DGRAM , 0 ) ;
	if( holder.fd < 0 )
		throw PublisherError( "cannot create socket" ) ;

	// bind the socket path
	LocalSocket::Address address( holder.path ) ;
	int rc = 0 ;
	{
		Root claim_root ;
		rc = ::bind( holder.fd , address.p() , address.size() ) ;
	}
	if( rc != 0 )
		throw PublisherError( "cannot bind socket path" , holder.path ) ;

	G_DEBUG( "G::PublisherImp::createSocket: new socket [" << holder.path << "]" ) ;
}

size_t G::PublisherImp::subscribe( SharedMemory & shmem_control , SocketHolder & socket_holder , const std::string & name )
{
	// find a free slot and grab it
	//
	size_t slot_id = SLOTS ;
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	{
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		slot_id = findFreeSlot( *mem ) ;
		if( slot_id < SLOTS )
			claimSlot( mem->slot[slot_id] , socket_holder.path , ::getpid() ) ;
	}
	if( slot_id == SLOTS )
		throw PublisherError( "no free slots in channel [" + name + "]" ) ; // probably need to kill failed subscribers

	return slot_id ;
}

void G::PublisherImp::releaseSlot( SharedMemory & shmem_control , size_t slot_id )
{
	G_ASSERT( slot_id < SLOTS ) ;
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	{
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		releaseSlot( SignalSafe() , lock , mem->slot[slot_id] ) ;
	}
}

void G::PublisherImp::releaseSlot( SignalSafe signal_safe , Lock & , Slot & slot )
{
	slot.in_use = false ;
	// leave the rest of the slot alone so that the publisher can clean up too
}

size_t G::PublisherImp::flush( int socket_fd )
{
	char c ;
	size_t count = 0U ;
	while( 1 == ::recv(socket_fd,&c,1,MSG_DONTWAIT) )
		count++ ;
	if( count > 0U )
		G_DEBUG( "G::PublisherImp::flush: event backlog: " << count ) ;
	return count ;
}

bool G::PublisherImp::getch( int socket_fd )
{
	char c ;
	return 1 == ::recv( socket_fd , &c , 1 , 0 ) ;
}

bool G::PublisherImp::receive( SharedMemory & shmem_control , unique_ptr<SharedMemory> & shmem_data ,
	const std::string & channel_name , size_t slot_id , int socket_fd , std::vector<char> & buffer , 
	std::string * type_p , G::EpochTime * time_p )
{
	bool peek = socket_fd == -1 ;
	if( time_p != nullptr )
		time_p->s = 0 , time_p->us = 0U ;

	// check the publisher is there
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	if( mem->magic != MAGIC ) // no-mutex read of constant data
		return false ;

	// possibly-blocking wait for socket
	if( !peek )
	{
		size_t flushed = flush( socket_fd ) ;
		if( flushed == 0U && !getch(socket_fd) )
		{
			buffer.clear() ;
			if( type_p != nullptr ) type_p->clear() ;
			throw PublisherError( "socket receive error" ) ;
		}
		G_DEBUG( "G::PublisherImp::receive: got publication event" ) ;
	}

	// lazy attach to the data segment -- this can fail if peeking because
	// the data segment will not exist if nothing has been published
	if( shmem_data.get() == nullptr )
	{
		shmem_data.reset( new SharedMemory(channel_name+".d") ) ;
		G_DEBUG( "G::PublisherImp::receive: data segment size: " << shmem_data->size() ) ;
	}

	// check the publisher again
	if( mem->magic != MAGIC )
		return false ;

	// copy the data payload into the caller's buffer
	unsigned long seq = 0UL ;
	{
		if( type_p != nullptr )
			type_p->resize( sizeof(((DataMemory*)(nullptr))->type)+1U ) ;

		// lock the shared memory -- could do better wrt. serialising readers -- use two 
		// mutexes and a reader-count for a 'readers-writer' lock
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		G_ASSERT( shmem_data.get() != nullptr ) ;

		// if the publisher has grown the data segment then we follow suit
		DataMemory * dmem = static_cast<DataMemory*>(shmem_data->ptr()) ;
		G_ASSERT( dmem != nullptr ) ;
		if( (dmem->size_limit+sizeof(DataMemory)) > shmem_data->size() )
			shmem_data->remap( dmem->size_limit+sizeof(DataMemory) , true ) ;
		dmem = static_cast<DataMemory*>(shmem_data->ptr()) ;

		// check that we have not seen the current data already
		Slot & slot = mem->slot[slot_id] ;
		seq = slot.seq ;
		bool ok = slot.in_use && ( peek || slot.seq == 0UL || slot.seq != mem->seq ) ;
		if( ok )
		{
			// update the sequence number
			if( !peek )
				slot.seq = mem->seq ;

			// copy out the payload
			buffer.resize( dmem->data_size ) ;
			::memcpy( &buffer[0] , dmem->data , buffer.size() ) ;

			// copy out the type
			if( type_p != nullptr )
				strget( dmem->type , sizeof(dmem->type) , *type_p ) ;

			// copy out the publication time
			if( time_p != nullptr )
				time_p->s = dmem->time_s , time_p->us = dmem->time_us ;
		}
		else
		{
			buffer.clear() ;
			if( type_p != nullptr ) type_p->clear() ;
		}
	}
	G_DEBUG( "G::PublisherImp::receive: got message [" << seq << "]" ) ;

	return true ;
}

size_t G::PublisherImp::findFreeSlot( ControlMemory & mem )
{
	for( size_t i = 0U ; i < SLOTS ; i++ )
	{
		if( !mem.slot[i].in_use && mem.slot[i].socket_fd == -1 )
			return i ;
	}
	return SLOTS ;
}

void G::PublisherImp::claimSlot( Slot & slot , const std::string & socket_path , pid_t pid )
{
	clearSlot( SignalSafe() , slot ) ;
	slot.in_use = true ;
	slot.pid = pid ;
	::strncpy( slot.socket_path.s , socket_path.c_str() , sizeof(slot.socket_path.s)-1U ) ;
}

void G::PublisherImp::clearSlot( SignalSafe , Slot & slot )
{
	slot.in_use = false ;
	slot.failed = false ;
	slot.pid = 0 ;
	slot.seq = 0UL ;
	slot.socket_fd = -1 ;
	::memset( slot.error , 0 , sizeof(slot.error) ) ;
	::memset( slot.socket_path.s , 0 , sizeof(slot.socket_path.s) ) ;
}

G::PublisherImp::Snapshot G::PublisherImp::snapshot( const std::string & channel_name )
{
	Snapshot snapshot ;
	snapshot.has_data = false ;

	SharedMemory shmem_control( channel_name , SharedMemory::Control() ) ;
	check( shmem_control , channel_name ) ;

	try
	{
		SharedMemory shmem_data( channel_name+".d" ) ;
		ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
		{
			Lock lock( Semaphore::at(&mem->mutex) ) ;
			DataMemory * dmem = static_cast<DataMemory*>(shmem_data.ptr()) ;
			snapshot.control = *mem ;
			snapshot.has_data = true ;
			snapshot.data = *dmem ;
		}
	}
	catch( G::SharedMemory::Error & )
	{
		ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
		{
			Lock lock( Semaphore::at(&mem->mutex) ) ;
			snapshot.control = *mem ;
			snapshot.has_data = false ;
		}
	}

	return snapshot ;
}

std::string G::PublisherImp::check( const std::string & channel_name )
{
	std::string reason ;
	try
	{
		SharedMemory shmem_control( channel_name , SharedMemory::Control() ) ;
		check( shmem_control , channel_name ) ;
	}
	catch( G::SharedMemory::Error & e )
	{
		reason = e.what() ;
	}
	catch( G::PublisherError & e )
	{
		reason = e.what() ;
	}
	return reason ;
}

std::vector<std::string> G::PublisherImp::list( std::vector<std::string> * others )
{
	typedef std::vector<std::string> List ;
	List list = G::SharedMemory::list() ;
	for( List::iterator p = list.begin() ; p != list.end() ; )
	{
		std::string name = *p ;
		std::string reason = check( name ) ;
		G_DEBUG( "G::PublisherImp::list: name=[" << name << "] reason=[" << reason << "]" ) ;
		if( reason.empty() )
		{
			++p ;
		}
		else
		{
			p = list.erase( p ) ;

			// heuristics -- only report if permission denied -- could do better
			const bool report = 
				name.find('.') == std::string::npos &&
				reason.find("permission denied") != std::string::npos ;

			if( others != nullptr && report )
				others->push_back( name ) ;
		}
	}
	return list ;
}

G::Item G::PublisherImp::info( const std::string & channel_name , bool detail , bool all_slots )
{
	Snapshot s = snapshot( channel_name ) ;

	Item publisher = Item::map() ;
	std::string publisher_info_str = strget( s.control.publisher_info , sizeof(s.control.publisher_info) ) ;
	if( !G::Item::parse( publisher_info_str , publisher ) )
		publisher = Item::map() ;
	publisher.add( "pid" , Str::fromInt(s.control.publisher_pid) ) ;

	if( detail )
	{
		Item item = Item::map() ;
		item.add( "publisher" , publisher ) ;

		item.add( "seq" , Str::fromULong(s.control.seq) ) ;
		item.add( "data" , Item::map() ) ;
		if( s.has_data )
		{
			item["data"].add( "type" , Str::printable(strget(s.data.type,sizeof(s.data.type))) ) ;
			item["data"].add( "size" , Str::fromULong(s.data.data_size) ) ;
			item["data"].add( "limit" , Str::fromULong(s.data.size_limit) ) ;
			item["data"].add( "time" , Str::fromULong(s.data.time_s) ) ; // long vs. time_t
		}
		item.add( "slots" , Item::list() ) ;
		for( size_t i = 0U ; i < SLOTS ; i++ )
		{
			const Slot & slot = s.control.slot[i] ;
			if( all_slots || ( slot.in_use && !slot.failed ) )
			{
				int error = slot.error[0] ? slot.error[0] : ( slot.error[1] ? slot.error[1] : slot.error[2] ) ;
				std::string strerror = error ? G::Process::strerror(error) : std::string() ;
				Item slot_info = Item::map() ;
				slot_info.add( "index" , Str::fromUInt(i) ) ;
				slot_info.add( "in_use" , slot.in_use ? "1" : "0" ) ;
				slot_info.add( "failed" , slot.failed ? "1" : "0" ) ;
				slot_info.add( "pid" , Str::fromUInt(slot.pid) ) ;
				slot_info.add( "seq" , Str::fromUInt(slot.seq) ) ;
				slot_info.add( "socket_fd" , Str::fromInt(slot.socket_fd) ) ;
				slot_info.add( "error" , strerror ) ;
				slot_info.add( "socket" , Str::printable(slot.socket_path.s) ) ;
				item["slots"].add( slot_info ) ;
			}
		}
		return item ;
	}
	else
	{
		return publisher ;
	}
}

void G::PublisherImp::purge( const std::string & channel_name )
{
	SharedMemory shmem_control( channel_name , SharedMemory::Control() ) ;
	ControlMemory * mem = static_cast<ControlMemory*>(shmem_control.ptr()) ;
	{
		Lock lock( Semaphore::at(&mem->mutex) ) ;
		for( size_t i = 0U ; i < SLOTS ; i++ )
		{
			Slot & slot = mem->slot[i] ;
			if( slot.in_use && slot.failed && slot.seq < mem->seq && (mem->seq-slot.seq) > 10 )
			{
				clearSlot( SignalSafe() , slot ) ;
			}
		}
	}
}

std::string G::PublisherImp::delete_( const std::string & channel_name )
{
	std::string e1 = SharedMemory::delete_( channel_name + ".d" , false ) ;
	std::string e2 = SharedMemory::delete_( channel_name , true ) ;
	const char * sep = !e1.empty() && !e2.empty() ? ": " : "" ;
	return e1 == e2 ? e1 : ( e1 + sep + e2 ) ;
}

char * G::PublisherImp::strdup_ignore_leak( const char * p )
{
	return ::strdup( p ) ;
}

void G::PublisherImp::strset( char * p , size_t n , const std::string & s )
{
	if( p == nullptr || n == 0U ) return ;
	size_t x = std::min( s.size() , n-1U ) ;
	if( x ) ::memcpy( p , s.data() , x ) ;
	p[x] = '\0' ;
}

void G::PublisherImp::strget( const char * p , size_t n , std::string & s )
{
	s.assign( p , n ) ;
	if( s.find('\0') != std::string::npos )
		s.resize( s.find('\0') ) ;
}

std::string G::PublisherImp::strget( const char * p , size_t n )
{
	std::string result ;
	strget( p , n , result ) ;
	return result ;
}

size_t G::PublisherImp::morethan( size_t n )
{
	return n + n/2U + 10U ;
}

// ==

G::PublisherImp::Lock::Lock( Semaphore * s ) :
	m_s(s)
{
	s->decrement() ;
}

G::PublisherImp::Lock::~Lock()
{
	m_s->increment() ;
}

// ==

G::PublisherImp::SocketHolder::SocketHolder( int fd_ ) :
	fd(fd_)
{
}

int G::PublisherImp::SocketHolder::release()
{
	int fd_ = fd ;
	fd = -1 ;
	return fd_ ;
}

G::PublisherImp::SocketHolder::~SocketHolder()
{
	if( fd != -1 )
		::close(fd) ;
}

// ==

G::PublisherImp::InfoSlot::InfoSlot()
{
	failed = false ;
	for( size_t i = 0 ; i < ERRORS ; i++ )
		error[i] = 0 ;
	socket_path.s[0] = '\0' ;
}

std::string G::PublisherImp::InfoSlot::path() const
{
	std::string path( socket_path.s , sizeof(socket_path.s) ) ;
	return Str::printable( path.c_str() ) ;
}

int G::PublisherImp::InfoSlot::e() const
{
	for( size_t i = 0 ; i < ERRORS ; i++ )
	{
		if( error[i] )
			return error[i] ;
	}
	return 0 ;
}

void G::PublisherImp::InfoSlot::report()
{
	std::ostringstream ss ;
	ss 
		<< "cannot publish to subscriber: socket path [" << path() << "] "
		<< "error [" << G::Process::strerror(e()) << "]" ;

	G_ERROR( "G::Publisher::publish: " << ss.str() ) ;
}


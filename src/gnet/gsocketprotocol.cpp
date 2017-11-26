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
// gsocketprotocol.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gnet.h"
#include "gmonitor.h"
#include "gtimer.h"
#include "gssl.h"
#include "gsocketprotocol.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"

namespace
{
	const size_t c_buffer_size = G::limits::net_buffer ;
	const int Result_ok = GSsl::Protocol::Result_ok ;
	const int Result_more = GSsl::Protocol::Result_more ;
	const int Result_read = GSsl::Protocol::Result_read ;
	const int Result_write = GSsl::Protocol::Result_write ;
	const int Result_error = GSsl::Protocol::Result_error ;
}

/// \class GNet::SocketProtocolImp
/// A pimple-pattern implementation class used by GNet::SocketProtocol.
///
class GNet::SocketProtocolImp
{
public:
	typedef std::pair<const char *,size_t> Segment ;
	typedef std::vector<Segment> Segments ;
	struct Position /// A pointer into the scatter/gather payload of GNet::SocketProtocolImp::send().
	{ 
		size_t segment ; 
		size_t offset ; 
		Position( size_t segment_ , size_t offset_ ) : segment(segment_) , offset(offset_) {} 
		Position() : segment(0U) , offset(0U) {} 
	} ;

	SocketProtocolImp( EventHandler & , SocketProtocol::Sink & , StreamSocket & , 
		unsigned int secure_connection_timeout ) ;
	~SocketProtocolImp() ;
	void readEvent() ;
	bool writeEvent() ;
	bool send( const std::string & data , size_t offset ) ;
	bool send( const Segments & ) ;
	void sslConnect() ;
	void sslAccept() ;
	bool sslEnabled() const ;
	std::string peerCertificate() const ;

private:
	SocketProtocolImp( const SocketProtocolImp & ) ;
	void operator=( const SocketProtocolImp & ) ;
	static GSsl::Protocol * newProtocol( const std::string & ) ;
	static void log( int level , const std::string & line ) ;
	bool failed() const ;
	bool rawSendImp( const Segments & , Position , Position & ) ;
	void rawReadEvent() ;
	bool rawWriteEvent() ;
	bool rawSend( const Segments & , Position , bool ) ;
	void sslReadImp() ;
	bool sslSendImp() ;
	void sslConnectImp() ;
	void sslAcceptImp() ;
	void logSecure( const std::string & ) const ;
	void logCertificate( const std::string & , const std::string & ) const ;
	void logFlowControl( const char * what ) ;
	void onSecureConnectionTimeout() ;
	static size_t size( const Segments & ) ;
	static bool finished( const Segments & , Position ) ;
	static Position position( const Segments & , Position , size_t ) ;
	static const char * chunk_p( const Segments & , Position ) ;
	static size_t chunk_n( const Segments & , Position ) ;

private:
	enum State { State_raw , State_connecting , State_accepting , State_writing , State_idle } ;
	EventHandler & m_handler ;
	SocketProtocol::Sink & m_sink ;
	StreamSocket & m_socket ;
	unsigned int m_secure_connection_timeout ;
	Segments m_segments ;
	Position m_raw_pos ;
	std::string m_raw_copy ;
	std::string m_ssl_send_data ;
	bool m_failed ;
	GSsl::Protocol * m_ssl ;
	State m_state ;
	std::vector<char> m_read_buffer ;
	GSsl::Protocol::ssize_type m_read_buffer_n ;
	Timer<SocketProtocolImp> m_secure_connection_timer ;
	std::string m_peer_certificate ;
} ;

namespace GNet
{
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Position & pos )
	{
		return stream << "(" << pos.segment << "," << pos.offset << ")" ;
	}
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Segment & segment )
	{
		return stream << "(" << (const void*)(segment.first) << ":" << segment.second << ")" ;
	}
	std::ostream & operator<<( std::ostream & stream , const SocketProtocolImp::Segments & segments )
	{
		stream << "[" ;
		const char * sep = "" ;
		for( size_t i = 0U ; i < segments.size() ; i++ , sep = "," )
			stream << sep << segments.at(i) ;
		stream << "]" ;
		return stream ;
	}
}

GNet::SocketProtocolImp::SocketProtocolImp( EventHandler & handler , 
	SocketProtocol::Sink & sink , StreamSocket & socket , 
	unsigned int secure_connection_timeout ) :
		m_handler(handler) ,
		m_sink(sink) ,
		m_socket(socket) ,
		m_secure_connection_timeout(secure_connection_timeout) ,
		m_failed(false) ,
		m_ssl(nullptr) ,
		m_state(State_raw) ,
		m_read_buffer(c_buffer_size) ,
		m_read_buffer_n(0) ,
		m_secure_connection_timer(*this,&SocketProtocolImp::onSecureConnectionTimeout,handler)
{
	G_ASSERT( m_read_buffer.size() == c_buffer_size ) ;
}

GNet::SocketProtocolImp::~SocketProtocolImp()
{
	delete m_ssl ;
}

void GNet::SocketProtocolImp::onSecureConnectionTimeout()
{
	G_DEBUG( "GNet::SocketProtocolImp::onSecureConnectionTimeout: timed out" ) ;
	throw SocketProtocol::SecureConnectionTimeout() ;
}

void GNet::SocketProtocolImp::readEvent()
{
	G_DEBUG( "SocketProtocolImp::readEvent: read event in state=" << m_state ) ;
	if( m_state == State_raw )
		rawReadEvent() ;
	else if( m_state == State_connecting )
		sslConnectImp() ;
	else if( m_state == State_accepting )
		sslAcceptImp() ;
	else if( m_state == State_writing )
		sslSendImp() ;
	else // State_idle
		sslReadImp() ;
}

bool GNet::SocketProtocolImp::writeEvent()
{
	G_DEBUG( "GNet::SocketProtocolImp::writeEvent: write event in state=" << m_state ) ;
	bool rc = true ;
	if( m_state == State_raw )
		rc = rawWriteEvent() ;
	else if( m_state == State_connecting )
		sslConnectImp() ;
	else if( m_state == State_accepting )
		sslAcceptImp() ;
	else if( m_state == State_writing )
		sslSendImp() ;
	else // State_idle
		sslReadImp() ;
	return rc ;
}

size_t GNet::SocketProtocolImp::size( const Segments & segments )
{
	size_t n = 0U ;
	for( Segments::const_iterator p = segments.begin() ; p != segments.end() ; ++p )
	{
		G_ASSERT( (*p).first != nullptr ) ;
		G_ASSERT( (*p).second != 0U ) ; // for chunk_p()
		n += (*p).second ;
	}
	return n ;
}

GNet::SocketProtocolImp::Position GNet::SocketProtocolImp::position( const Segments & s , Position pos , size_t offset )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( (pos.offset+offset) <= s.at(pos.segment).second ) ; // because chunk_p()
	pos.offset += offset ;
	if( pos.offset >= s.at(pos.segment).second ) // in practice if==
	{
		pos.segment++ ;
		pos.offset = 0U ;
	}
	return pos ;
}

const char * GNet::SocketProtocolImp::chunk_p( const Segments & s , Position pos )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( pos.offset < s[pos.segment].second ) ;

	const Segment & segment = s.at( pos.segment ) ;
	return segment.first + pos.offset ;
}

size_t GNet::SocketProtocolImp::chunk_n( const Segments & s , Position pos )
{
	G_ASSERT( pos.segment < s.size() ) ;
	G_ASSERT( pos.offset < s[pos.segment].second ) ;

	const Segment & segment = s.at( pos.segment ) ;
	return segment.second - pos.offset ;
}

bool GNet::SocketProtocolImp::send( const Segments & segments )
{
	// for now only support scatter-gather segments in raw mode
	if( m_state != State_raw )
		throw std::runtime_error( "scatter/gather overload not implemented for tls" ) ; // TODO scatter-gather with tls library

	if( size(segments) == 0U )
		return true ;

	return rawSend( segments , Position() , false/*copy*/ ) ;
}

bool GNet::SocketProtocolImp::send( const std::string & data , size_t offset )
{
	if( data.empty() || offset >= data.length() )
		return true ;

	bool rc = true ;
	if( m_state == State_raw )
	{
		Segments segments( 1U ) ;
		segments[0].first = data.data() ;
		segments[0].second = data.size() ;
		rc = rawSend( segments , Position(0U,offset) , true/*copy*/ ) ;
	}
	else if( m_state == State_connecting || m_state == State_accepting )
	{
		throw SocketProtocol::SendError( "still busy negotiating" ) ;
	}
	else if( m_state == State_writing )
	{
		// we've been asked to send more stuff even though the previous send()
		// returned false - we could add to the pending buffer but it then 
		// gets complicated to retry the ssl->write(), so just throw an error
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;
	}
	else
	{
		m_state = State_writing ;
		m_ssl_send_data.append( data.substr(offset) ) ;
		rc = sslSendImp() ;
	}
	return rc ;
}

void GNet::SocketProtocolImp::log( int level , const std::string & log_line )
{
	if( level == 1 )
		G_DEBUG( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
	else if( level == 2 )
		G_LOG( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
	else
		G_WARNING( "GNet::SocketProtocolImp::log: tls: " << log_line ) ;
}

GSsl::Protocol * GNet::SocketProtocolImp::newProtocol( const std::string & profile_name )
{
	GSsl::Library * library = GSsl::Library::instance() ;
	if( library == nullptr )
		throw G::Exception( "SocketProtocolImp::newProtocol: internal error: no library instance" ) ;

	return new GSsl::Protocol( library->profile(profile_name) , log ) ;
}

void GNet::SocketProtocolImp::sslConnect()
{
	G_DEBUG( "SocketProtocolImp::sslConnect" ) ;
	G_ASSERT( m_ssl == nullptr ) ;

	m_ssl = newProtocol( "client" ) ;
	m_state = State_connecting ;
	if( m_secure_connection_timeout != 0U )
		m_secure_connection_timer.startTimer( m_secure_connection_timeout ) ;
	sslConnectImp() ;
}

void GNet::SocketProtocolImp::sslConnectImp()
{
	G_DEBUG( "SocketProtocolImp::sslConnectImp" ) ;
	G_ASSERT( m_ssl != nullptr ) ;
	G_ASSERT( m_state == State_connecting ) ;

	GSsl::Protocol::Result rc = m_ssl->connect( m_socket ) ;
	G_DEBUG( "SocketProtocolImp::sslConnectImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == Result_error )
	{
		m_socket.dropWriteHandler() ;
		m_state = State_raw ;
		throw SocketProtocol::ReadError( "ssl connect" ) ;
	}
	else if( rc == Result_read )
	{
		m_socket.dropWriteHandler() ;
	}
	else if( rc == Result_write )
	{
		m_socket.addWriteHandler( m_handler ) ;
	}
	else
	{
		m_socket.dropWriteHandler() ;
		m_state = State_idle ;
		if( m_secure_connection_timeout != 0U )
			m_secure_connection_timer.cancelTimer() ;
		m_peer_certificate = m_ssl->peerCertificate().first ;
		logSecure( m_peer_certificate ) ;
		G_DEBUG( "SocketProtocolImp::sslConnectImp: calling onSecure: " << G::Str::printable(m_peer_certificate) ) ;
		m_sink.onSecure( m_peer_certificate ) ;
	}
}

void GNet::SocketProtocolImp::sslAccept()
{
	G_DEBUG( "SocketProtocolImp::sslAccept" ) ;
	G_ASSERT( m_ssl == nullptr ) ;

	m_ssl = newProtocol( "server" ) ;
	m_state = State_accepting ;
	sslAcceptImp() ;
}

void GNet::SocketProtocolImp::sslAcceptImp()
{
	G_DEBUG( "SocketProtocolImp::sslAcceptImp" ) ;
	G_ASSERT( m_ssl != nullptr ) ;
	G_ASSERT( m_state == State_accepting ) ;

	GSsl::Protocol::Result rc = m_ssl->accept( m_socket ) ;
	G_DEBUG( "SocketProtocolImp::sslAcceptImp: result=" << GSsl::Protocol::str(rc) ) ;
	if( rc == Result_error )
	{
		m_socket.dropWriteHandler() ;
		m_state = State_raw ;
		throw SocketProtocol::ReadError( "ssl accept" ) ;
	}
	else if( rc == Result_read )
	{
		m_socket.dropWriteHandler() ;
	}
	else if( rc == Result_write )
	{
		m_socket.addWriteHandler( m_handler ) ;
	}
	else
	{
		m_socket.dropWriteHandler() ;
		m_state = State_idle ;
		m_peer_certificate = m_ssl->peerCertificate().first ;
		logSecure( m_peer_certificate ) ;
		G_DEBUG( "SocketProtocolImp::sslAcceptImp: calling onSecure: " << G::Str::printable(m_peer_certificate) ) ;
		m_sink.onSecure( m_peer_certificate ) ;
	}
}

bool GNet::SocketProtocolImp::sslEnabled() const
{
	return m_state == State_writing || m_state == State_idle ;
}

bool GNet::SocketProtocolImp::sslSendImp()
{
	G_ASSERT( m_state == State_writing ) ;

	bool rc = false ;
	GSsl::Protocol::ssize_type n = 0 ;
	GSsl::Protocol::Result result = m_ssl->write( m_ssl_send_data.data() , m_ssl_send_data.size() , n ) ;
	if( result == Result_error )
	{
		m_socket.dropWriteHandler() ;
		m_state = State_idle ;
		throw SocketProtocol::SendError( "ssl write" ) ;
	}
	else if( result == Result_read )
	{
		m_socket.dropWriteHandler() ;
	}
	else if( result == Result_write )
	{
		m_socket.addWriteHandler( m_handler ) ;
	}
	else
	{
		m_socket.dropWriteHandler() ;
		if( n < 0 ) throw SocketProtocol::SendError( "ssl arithmetic underflow" ) ;
		size_t un = static_cast<size_t>(n) ;
		rc = un == m_ssl_send_data.size() ;
		m_ssl_send_data.erase( 0U , un ) ;
		m_state = State_idle ;
	}
	return rc ;
}

void GNet::SocketProtocolImp::sslReadImp()
{
	G_DEBUG( "SocketProtocolImp::sslReadImp" ) ;
	G_ASSERT( m_state == State_idle ) ;
	G_ASSERT( m_ssl != nullptr ) ;

	std::vector<char> more_data ;
	GSsl::Protocol::Result rc = GSsl::Protocol::Result_more ;
	for( int sanity = 0 ; rc == Result_more && sanity < 1000 ; sanity++ )
	{
		rc = m_ssl->read( &m_read_buffer[0] , m_read_buffer.size() , m_read_buffer_n ) ;
		G_DEBUG( "SocketProtocolImp::sslReadImp: result=" << GSsl::Protocol::str(rc) ) ;
		if( rc == Result_error )
		{
			m_socket.dropWriteHandler() ;
			m_state = State_idle ;
			throw SocketProtocol::ReadError( "ssl read" ) ;
		}
		else if( rc == Result_read )
		{
			m_socket.dropWriteHandler() ;
		}
		else if( rc == Result_write )
		{
			m_socket.addWriteHandler( m_handler ) ;
		}
		else // Result_ok, Result_more
		{
			G_ASSERT( rc == Result_ok || rc == Result_more ) ;
			m_socket.dropWriteHandler() ;
			m_state = State_idle ;
			size_t n = static_cast<size_t>(m_read_buffer_n) ;
			m_read_buffer_n = 0 ;
			G_DEBUG( "SocketProtocolImp::sslReadImp: calling onData(): " << n ) ;
			m_sink.onData( &m_read_buffer[0] , n ) ;
		}
		if( rc == Result_more )
			G_DEBUG( "SocketProtocolImp::sslReadImp: more available to read" ) ;
	}
}

void GNet::SocketProtocolImp::rawReadEvent()
{
	const size_t read_buffer_size = G::Test::enabled("small-client-input-buffer") ? 3 : m_read_buffer.size() ;
	const ssize_t rc = m_socket.read( &m_read_buffer[0] , read_buffer_size ) ;

	if( rc == 0 || ( rc == -1 && !m_socket.eWouldBlock() ) )
	{
		throw SocketProtocol::ReadError() ;
	}
	else if( rc != -1 )
	{
		G_ASSERT( static_cast<size_t>(rc) <= read_buffer_size ) ;
		m_sink.onData( &m_read_buffer[0] , static_cast<size_t>(rc) ) ;
	}
	else
	{
		G_DEBUG( "GNet::SocketProtocolImp::rawReadEvent: read event read nothing" ) ;
		; // -1 && eWouldBlock() -- no-op (esp. for windows)
	}
}

bool GNet::SocketProtocolImp::finished( const Segments & segments , Position pos )
{
	return pos.segment == segments.size() ;
}

bool GNet::SocketProtocolImp::rawSend( const Segments & segments , Position pos , bool do_copy )
{
	G_ASSERT( !do_copy || segments.size() == 1U ) ; // copy => one segment

	if( !finished(m_segments,m_raw_pos) )
		throw SocketProtocol::SendError( "still busy sending the last packet" ) ;

	Position pos_out ;
	bool all_sent = rawSendImp( segments , pos , pos_out ) ;
	if( !all_sent && failed() )
	{
		m_segments.clear() ;
		m_raw_pos = Position() ;
		m_raw_copy.clear() ;
		throw SocketProtocol::SendError() ;
	}
	if( all_sent )
	{
		m_segments.clear() ;
		m_raw_pos = Position() ;
		m_raw_copy.clear() ;
	}
	else
	{
		// keep the residue in m_segments/m_raw_pos/m_raw_copy
		m_segments = segments ;
		m_raw_pos = pos_out ; 
		G_ASSERT( m_segments.empty() || m_segments[0].first != m_raw_copy.data() ) ;
		m_raw_copy.clear() ;

		// if the caller's data is temporary then keep our own copy
		if( do_copy ) 
		{
			G_ASSERT( segments.size() == 1U ) ; // precondition
			G_ASSERT( pos_out.offset < segments[0].second ) ; // since not all sent
			m_raw_copy.assign( segments[0].first+pos_out.offset , segments[0].second-pos_out.offset ) ;
			m_segments[0].first = m_raw_copy.data() ; 
			m_segments[0].second = m_raw_copy.size() ;
			m_raw_pos = Position() ;
		}

		m_socket.addWriteHandler( m_handler ) ;
		logFlowControl( "asserted" ) ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::rawWriteEvent()
{
	m_socket.dropWriteHandler() ;
	logFlowControl( "released" ) ;

	bool all_sent = rawSendImp( m_segments , m_raw_pos , m_raw_pos ) ;
	if( !all_sent && failed() )
	{
		m_segments.clear() ;
		m_raw_pos = Position() ;
		m_raw_copy.clear() ;
		throw SocketProtocol::SendError() ;
	}
	if( all_sent )
	{
		m_segments.clear() ;
		m_raw_pos = Position() ;
		m_raw_copy.clear() ;
	}
	else
	{
		m_socket.addWriteHandler( m_handler ) ;
		logFlowControl( "reasserted" ) ;
	}
	return all_sent ;
}

bool GNet::SocketProtocolImp::rawSendImp( const Segments & segments , Position pos , Position & pos_out )
{
	while( !finished(segments,pos) )
	{
		const char * chunk_data = chunk_p( segments , pos ) ;
		size_t chunk_size = chunk_n( segments , pos ) ;

		ssize_t rc = m_socket.write( chunk_data , chunk_size ) ;
		if( rc < 0 && ! m_socket.eWouldBlock() )
		{
			// fatal error, eg. disconnection
			m_failed = true ;
			pos_out = Position() ;
			return false ; // failed()
		}
		else if( rc < 0 || static_cast<size_t>(rc) < chunk_size )
		{
			// flow control asserted -- return the position where we stopped
			size_t nsent = rc > 0 ? static_cast<size_t>(rc) : 0U ;
			pos_out = position( segments , pos , nsent ) ;
			G_ASSERT( !finished(segments,pos_out) ) ;
			return false ; // not all sent
		}
		else
		{
			pos = position( segments , pos , static_cast<size_t>(rc) ) ;
		}
	}
	return true ; // all sent
}

bool GNet::SocketProtocolImp::failed() const
{
	return m_failed ;
}

void GNet::SocketProtocolImp::logSecure( const std::string & certificate ) const
{
	std::pair<std::string,bool> rc( std::string() , false ) ;
	if( GNet::Monitor::instance() )
		rc = GNet::Monitor::instance()->findCertificate( certificate ) ;
	if( rc.second ) // is new
		logCertificate( rc.first , certificate ) ;

	G_LOG( "GNet::SocketProtocolImp: tls/ssl protocol established with " 
		<< m_socket.getPeerAddress().second.displayString() 
		<< (rc.first.empty()?"":" certificate ") << rc.first ) ;
}

void GNet::SocketProtocolImp::logCertificate( const std::string & certid , const std::string & certificate ) const
{
	G::StringArray lines ;
	lines.reserve( 30U ) ;
	G::Str::splitIntoFields( certificate , lines , "\n" ) ;
	for( G::StringArray::iterator line_p = lines.begin() ; line_p != lines.end() ; ++line_p )
	{
		if( !(*line_p).empty() )
		{
			G_LOG( "GNet::SocketProtocolImp: certificate " << certid << ": " << *line_p ) ;
		}
	}
}

void GNet::SocketProtocolImp::logFlowControl( const char * what )
{
	const bool log = G::Test::enabled("log-flow-control") ;
	if( log )
		G_LOG( "GNet::SocketProtocolImp::send: @" << m_socket.asString() << ": flow control " << what ) ;
}

std::string GNet::SocketProtocolImp::peerCertificate() const
{
	return m_peer_certificate ;
}

// 

GNet::SocketProtocol::SocketProtocol( EventHandler & handler , Sink & sink , StreamSocket & socket ,
	unsigned int secure_connection_timeout ) :
		m_imp( new SocketProtocolImp(handler,sink,socket,secure_connection_timeout) )
{
}

GNet::SocketProtocol::~SocketProtocol()
{
	delete m_imp ;
}

void GNet::SocketProtocol::readEvent()
{
	m_imp->readEvent() ;
}

bool GNet::SocketProtocol::writeEvent()
{
	return m_imp->writeEvent() ;
}

bool GNet::SocketProtocol::send( const std::string & data , size_t offset )
{
	return m_imp->send( data , offset ) ;
}

bool GNet::SocketProtocol::send( const std::vector<std::pair<const char *,size_t> > & data )
{
	return m_imp->send( data ) ;
}

bool GNet::SocketProtocol::sslCapable()
{
	return GSsl::Library::instance() != nullptr && GSsl::Library::instance()->enabled() ;
}

void GNet::SocketProtocol::sslConnect()
{
	m_imp->sslConnect() ;
}

void GNet::SocketProtocol::sslAccept()
{
	m_imp->sslAccept() ;
}

bool GNet::SocketProtocol::sslEnabled() const
{
	return m_imp->sslEnabled() ;
}

std::string GNet::SocketProtocol::peerCertificate() const
{
	return m_imp->peerCertificate() ;
}

//

GNet::SocketProtocolSink::~SocketProtocolSink()
{
}


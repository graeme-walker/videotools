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
// gfiletree.cpp
// 

#include "gdef.h"
#include "gfiletree.h"
#include "gstrings.h"
#include "gdebug.h"
#include <algorithm>

G::FileTree::FileTree() :
	m_moved(false) ,
	m_callback(nullptr)
{
}

G::FileTree::FileTree( const G::Path & root , DirectoryTreeCallback * callback ) :
	m_moved(true) ,
	m_root(root.collapsed()) ,
	m_p(m_root,false,callback) ,
	m_callback(callback)
{
	const DirectoryTree end ;
	while( m_p != end && (*m_p).m_is_dir )
		++m_p ;
}

void G::FileTree::swap( FileTree & other )
{
	using std::swap ;
	swap( m_moved , other.m_moved ) ;
	swap( m_root , other.m_root ) ;
	swap( m_p , other.m_p ) ;
	swap( m_callback , other.m_callback ) ;
}

void G::FileTree::reroot( const G::Path & root )
{
	m_root = root.collapsed() ;
	m_moved = true ;
	m_p = DirectoryTree( m_root , false , m_callback ) ;
	const DirectoryTree end ;
	while( m_p != end && (*m_p).m_is_dir )
		++m_p ;
}

G::Path G::FileTree::root() const
{
	return m_root ;
}

G::Path G::FileTree::current() const
{
	return m_p == DirectoryTree() ? G::Path() : (*m_p).m_path ;
}

G::Path G::FileTree::next()
{
	if( m_moved )
	{
		m_moved = false ;
	}
	else
	{
		const DirectoryTree end ;
		if( m_p != end )
		{
			++m_p ;
			while( m_p != end && (*m_p).m_is_dir )
				++m_p ;
		}
	}
	return current() ;
}

bool G::FileTree::reposition( const G::Path & path )
{
	int rc = reposition( path , 0 ) ;
	return rc == 0 ;
}

int G::FileTree::reposition( const G::Path & path , int )
{
	G::Path relative_path = G::Path::difference( m_root , path ) ;
	if( relative_path == G::Path() )
		return 1 ; // out-of-tree

	G::StringArray parts = relative_path.split() ;
	if( parts.size() == 1U && parts.at(0U) == "." )
		parts.clear() ;

	DirectoryTree p( m_root , m_p.reversed() , m_callback ) ;
	const DirectoryTree end ;
	size_t i = 0U ; for( ; i < parts.size() ; i++ )
	{
		const std::string & part = parts.at(i) ;
		G_DEBUG( "G::FileTree::reposition: part: [" << part << "]" ) ;
		if( p != end && (*p).m_is_dir && p.down( part , true ) )
			;
		else
			break ;
	}

	if( p != end && i < parts.size() ) // over-specified
		++p ;

	while( p != end && (*p).m_is_dir )
		++p ;

	if( p == end )
		return 2 ; // off-the-end

	m_p = p ;
	m_moved = true ;
	return 0 ;
}

bool G::FileTree::last()
{
	// use a reversed iterator to get the last position
	const DirectoryTree end ;
	bool is_reversed = m_p.reversed() ;
	DirectoryTree p( m_root , !is_reversed , m_callback ) ;
	while( p != end && (*p).m_is_dir )
		++p ;

	bool changed = m_p != p ;
	m_p.swap( p ) ;
	m_p.reverse( is_reversed ) ;
	m_moved = true ;
	return changed ;
}

bool G::FileTree::first()
{
	// use a brand new iterator to get the first position
	const DirectoryTree end ;
	DirectoryTree p( m_root , m_p.reversed() , m_callback ) ;
	while( p != end && (*p).m_is_dir )
		++p ;

	bool changed = m_p != p ;
	m_p.swap( p ) ;
	m_moved = true ;
	return changed ;
}

void G::FileTree::reverse( bool in_reverse )
{
	m_p.reverse( in_reverse ) ;
}

bool G::FileTree::reversed() const
{
	return m_p.reversed() ;
}

bool G::FileTree::moved() const
{
	return m_moved ;
}

/// \file gfiletree.cpp

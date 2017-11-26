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
// gdirectorytree.cpp
//

#include "gdef.h"
#include "gdirectorytree.h"
#include "gdirectory.h"
#include "groot.h"
#include "gfile.h"
#include "gpath.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>
#include <functional>
#include <iterator>

namespace
{
	bool compare_names( const G::DirectoryList::Item & a , const G::DirectoryList::Item & b )
	{
		return a.m_name < b.m_name ;
	}
	struct CompareNames
	{
		explicit CompareNames( bool reverse ) : m_reverse(reverse) {}
		bool operator()( const G::DirectoryList::Item & a , const G::DirectoryList::Item & b ) const
		{
			return m_reverse ? compare_names( b , a ) : compare_names( a , b ) ;
		}
		bool m_reverse ;
	} ;
	template <typename T, typename C>
	bool is__sorted( T p , T end , C cmp )
	{
		T prev = p ;
		for( bool first = true ; p != end ; ++p , first = false )
		{
			if( !first && !cmp(*prev,*p) )
				return false ;
			prev = p ;
		}
		return true ;
	}
}

G::DirectoryTree::DirectoryTree() :
	m_reverse(false) ,
	m_callback(0)
{
}

G::DirectoryTree::DirectoryTree( const Path & path , bool reverse , DirectoryTreeCallback * callback ) :
	m_reverse(reverse) ,
	m_callback(callback)
{
	bool exists = false ;
	{
		G::Root claim_root ;
		exists = G::File::exists(path) ;
	}
	if( exists )
	{
		FileItem item ;
		item.m_is_dir = isDir(path) ;
		item.m_path = path ;
		item.m_name = path.basename() ;

		if( !ignore(item,0) )
		{
			m_stack.push_back( Context() ) ;
			Context & context = m_stack.back() ;

			context.m_list.push_back( item ) ;
			context.m_p = context.m_list.begin() ;
			context.m_end = context.m_list.end() ;
		}
	}
}

G::DirectoryTree::FileList G::DirectoryTree::readFileList() const
{
	FileList file_list ;
	{
		G::Root claim_root ;
		G::DirectoryList::readAll( (*m_stack.back().m_p).m_path , file_list , true ) ;
	}
	G_ASSERT( is__sorted(file_list.begin(),file_list.end(),compare_names) ) ;
	filter( file_list , m_stack.size() ) ;
	return file_list ;
}

G::DirectoryTree & G::DirectoryTree::operator++()
{
	G_ASSERT( !m_stack.empty() ) ;

	FileList file_list ;
	if( (*m_stack.back().m_p).m_is_dir )
	{
		file_list = readFileList() ;
		if( m_reverse )
			std::reverse( file_list.begin() , file_list.end() ) ;
	}

	if( file_list.empty() )
	{
		while( !m_stack.empty() )
		{
			++m_stack.back().m_p ;
			if( m_stack.back().m_p == m_stack.back().m_end )
				m_stack.pop_back() ;
			else
				break ;
		}
	}
	else
	{
		m_stack.push_back( Context() ) ;
		Context & context = m_stack.back() ;
		context.m_list.swap( file_list ) ;
		context.m_p = context.m_list.begin() ;
		context.m_end = context.m_list.end() ;
	}
	return *this ;
}

bool G::DirectoryTree::operator==( const DirectoryTree & other ) const
{
	if( m_stack.size() != other.m_stack.size() )
		return false ;
	else if( m_stack.empty() )
		return true ;
	return 
		(*m_stack.back().m_p).m_path == (*other.m_stack.back().m_p).m_path ;
}

bool G::DirectoryTree::operator!=( const DirectoryTree & other ) const
{
	return !( *this == other ) ;
}

const G::DirectoryList::Item & G::DirectoryTree::operator*() const
{
	G_ASSERT( !m_stack.empty() ) ;
	return *m_stack.back().m_p ;
}

bool G::DirectoryTree::isDir( const Path & path )
{
	G::Root claim_root ;
	return Directory(path).valid() ;
}

size_t G::DirectoryTree::depth() const
{
	G_ASSERT( !m_stack.empty() ) ;
	return m_stack.size() - 1U ;
}

G::DirectoryTree::DirectoryTree( const G::DirectoryTree & other ) :
	m_stack(other.m_stack) ,
	m_reverse(other.m_reverse) ,
	m_callback(other.m_callback)
{
	Stack::iterator other_p = (const_cast<DirectoryTree&>(other)).m_stack.begin() ;
	for( Stack::iterator this_p = m_stack.begin() ; this_p != m_stack.end() ; ++this_p , ++other_p )
	{
		(*this_p).m_p = (*this_p).m_list.begin() ;
		(*this_p).m_end = (*this_p).m_list.end() ;
		std::advance( (*this_p).m_p , std::distance((*other_p).m_list.begin(),(*other_p).m_p) ) ;
	}
}

void G::DirectoryTree::swap( G::DirectoryTree & other )
{
	m_stack.swap( other.m_stack ) ;
	std::swap( m_reverse , other.m_reverse ) ;
	std::swap( m_callback , other.m_callback ) ;
}

G::DirectoryTree & G::DirectoryTree::operator=( const G::DirectoryTree & other )
{
	DirectoryTree tmp( other ) ;
	swap( tmp ) ;
	return *this ;
}

bool G::DirectoryTree::down( const std::string & name , bool at_or_after )
{
	typedef std::pair<FileList::iterator,FileList::iterator> Range ;
	G_ASSERT( !m_stack.empty() ) ;
	G_ASSERT( (*m_stack.back().m_p).m_is_dir ) ;

	FileList file_list = readFileList() ;

	if( m_reverse )
		std::reverse( file_list.begin() , file_list.end() ) ;

	FileItem item ; 
	item.m_name = name ;
	G_ASSERT( is__sorted(file_list.begin(),file_list.end(),CompareNames(m_reverse)) ) ;
	Range range = std::equal_range( file_list.begin() , file_list.end() , item , CompareNames(m_reverse) ) ;
	if( at_or_after && range.first == file_list.end() )
	{
		while( !m_stack.empty() )
		{
			++m_stack.back().m_p ;
			if( m_stack.back().m_p == m_stack.back().m_end )
				m_stack.pop_back() ;
			else
				break ;
		}
		return false ;
	}
	else if( at_or_after || range.first != range.second )
	{
		m_stack.push_back( Context() ) ;
		Context & context = m_stack.back() ;
		context.m_list.swap( file_list ) ;
		context.m_p = range.first ;
		context.m_end = context.m_list.end() ;
		return true ;
	}
	else
	{
		return false ;
	}
}

void G::DirectoryTree::up()
{
	G_ASSERT( !m_stack.empty() ) ;
	m_stack.pop_back() ;
}

void G::DirectoryTree::reverse( bool in_reverse )
{
	if( m_reverse != in_reverse )
	{
		m_reverse = in_reverse ;
		for( Stack::iterator stack_p = m_stack.begin() ; stack_p != m_stack.end() ; ++stack_p )
		{
			Context & context = *stack_p ;
			size_t pos = std::distance( context.m_list.begin() , context.m_p ) ;
			std::reverse( context.m_list.begin() , context.m_list.end() ) ;
			context.m_p = context.m_list.begin() ;
			context.m_end = context.m_list.end() ;
			std::advance( context.m_p , context.m_list.size()-pos-1 ) ;
		}
	}
}

void G::DirectoryTree::filter( FileList & list , size_t depth ) const
{
	for( FileList::iterator p = list.begin() ; p != list.end() ; )
	{
		if( ignore(*p,depth) )
			p = list.erase( p ) ;
		else
			++p ;
	}
}

bool G::DirectoryTree::ignore( const FileItem & item , size_t depth ) const
{
	if( m_callback )
		return m_callback->directoryTreeIgnore( item , depth ) ;
	else
		return false ;
}

bool G::DirectoryTree::reversed() const
{
	return m_reverse ;
}

// ==

G::DirectoryTreeCallback::~DirectoryTreeCallback()
{
}

/// \file gdirectorytree.cpp

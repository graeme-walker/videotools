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
/// \file gdirectorytree.h
///

#ifndef G_DIRECTORY_TREE__H
#define G_DIRECTORY_TREE__H

#include "gdef.h"
#include "gdirectory.h"
#include "gpath.h"
#include <list>
#include <vector>

namespace G
{
	class DirectoryTree ;
	class DirectoryTreeCallback ;
}

/// \class G::DirectoryTree
/// A directory tree iterator for sorted, depth-first traversal of files and
/// directories.
/// 
class G::DirectoryTree
{
public:
	DirectoryTree() ;
		///< Default contructor for an off-the-end iterator.

	explicit DirectoryTree( const G::Path & root , bool reverse = false , 
		DirectoryTreeCallback *ignore = 0 ) ;
			///< Contructor for a tree iterator rooted at the given path.
			///< 
			///< If reverse-d the iterator moves through each depth level 
			///< in reverse; this is not the same a reversal of the whole 
			///< sequence because the sequence includes non-leaf nodes.
			///< It follows that reversal is more useful when directories
			///< are filtered out by client code (see G::FileTree).
			///< 
			///< An optional callback function, taking a path and a depth,
			///< can be used to filter out files and directories that
			///< should be ignored. Directories that are ignored are not
			///< traversed at all.

	DirectoryTree & operator++() ;
		///< Pre-increment operator. Moves to the next file or directory.

	const DirectoryList::Item & operator*() const ;
		///< Dereference operator yielding a G::DirectoryList::Item.

	bool operator==( const DirectoryTree & ) const ;
		///< Comparison operator.

	bool operator!=( const DirectoryTree & ) const ;
		///< Comparison operator.

	size_t depth() const ;
		///< Returns the nesting depth compared to the root.

	bool down( const std::string & name , bool at_or_after = false ) ;
		///< Moves deeper into the directory tree, with a precondition 
		///< that the iterator currently points to a directory.
		///< 
		///< If 'at_or_after' is false the iterator is moved into the 
		///< directory to a position that matches the requested name. 
		///< Returns false if not found, with the iterator position 
		///< unchanged.
		///< 
		///< If 'at_or_after' is true the iterator is moved into the
		///< directory to a position that is at or after the requested 
		///< name and then returns true. However, if the requested name 
		///< is lexicographically later than the last name in the
		///< directory, then the iterator might advance to another 
		///< directory altogether or completely off-the-end of the tree, 
		///< and in this case false is returned.
		///< 
		///< Precondition: operator*().m_is_dir

	void up() ;
		///< Moves up to the parent directory.
		///< Precondition: depth() >= 1

	void swap( DirectoryTree & other ) ;
		///< Swaps this and other.

	DirectoryTree( const DirectoryTree & ) ;
		///< Copy constructor.

	DirectoryTree & operator=( const DirectoryTree & ) ;
		///< Assignment operator.

	void reverse( bool in_reverse = true ) ;
		///< Changes the reversal state, returning the previous value.

	bool reversed() const ;
		///< Returns the current reversal state.

private:
	typedef DirectoryList::Item FileItem ;
	typedef std::vector<FileItem> FileList ;
	struct Context
	{
		FileList m_list ;
		FileList::iterator m_p ;
		FileList::iterator m_end ;
	} ;
	typedef std::list<Context> Stack ;

private:
	static bool isDir( const Path & ) ;
	FileList readFileList() const ;
	void filter( FileList & , size_t ) const ;
	bool ignore( const FileItem & , size_t ) const ;

private:
	Stack m_stack ;
	bool m_reverse ;
	DirectoryTreeCallback * m_callback ;
} ;

/// \class G::DirectoryTreeCallback
/// A callback interface to allow G::DirectoryTree to ignore paths.
/// 
class G::DirectoryTreeCallback
{
public:
	virtual bool directoryTreeIgnore( const G::DirectoryList::Item & , size_t depth ) = 0 ;
		///< Returns true if the file should be ignored.

protected:
	virtual ~DirectoryTreeCallback() ;
} ;

namespace G
{
	inline
	void swap( DirectoryTree & a , DirectoryTree & b )
	{
		a.swap( b ) ;
	}
}

#endif

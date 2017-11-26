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
/// \file gfiletree.h
///

#ifndef G_FILE_TREE__H
#define G_FILE_TREE__H

#include "gdef.h"
#include "gpath.h"
#include "gdirectorytree.h"
#include "gexception.h"
#include <list>
#include <vector>

namespace G
{
	class FileTree ;
}

/// \class G::FileTree
/// A class for walking files in a directory tree, with repositioning. The 
/// implementation uses G::DirectoryTree, transparently stepping over 
/// directories. 
/// 
/// The iteration model is:
/// \code
/// while( tree.next() != G::Path() )
/// {
///    if( fn( tree.current() ) )
///       tree.reposition(...) ;
/// }
/// \endcode
/// 
class G::FileTree
{
public:

	FileTree() ;
		///< Default contructor for an empty tree.

	explicit FileTree( const G::Path & root , DirectoryTreeCallback * ignore = 0 ) ;
		///< Constructor for the tree rooted at the given path.
		///< (The optional ignore callback does not need to ignore
		///< directories; the FileTree class does that anyways.)

	void reroot( const G::Path & root ) ;
		///< Resets the root, as if newly constructed.

	G::Path root() const ;
		///< Returns the current root, as passed to the constructor
		///< or reroot(). Returns the empty string if default 
		///< constructed.

	bool moved() const ;
		///< Returns true after construction, reroot(), first(), last()
		///< or a successful reposition(). The moved() state is
		///< reset by next().

	G::Path current() const ;
		///< Returns the current path.

	G::Path next() ;
		///< Moves to the next file in the tree, depth first, and returns
		///< the path. Returns the empty path if off the end or if 
		///< default constructed. However, in the moved() state the 
		///< first call to next() just returns current() and resets 
		///< the moved() state. It is okay to call next() if currently 
		///< off the end.

	bool reposition( const G::Path & path ) ;
		///< Repositions the iterator within the current tree, at or after
		///< the given position. The path does not have to represent an 
		///< existing file or directory. 
		///< 
		///< The given path should normally start with the root(). If the 
		///< root is a relative path then the reposition() path should
		///< also be relative.
		///< 
		///< Returns false if there are no files at or after the required 
		///< position or if the position is outside the tree, without
		///< changing the current iterator position.

	int reposition( const G::Path & path , int ) ;
		///< An overload that distinguishes the repositioning failure reason
		///< as (1) out-of-tree or (2) off-the-end. Returns zero on success.

	bool last() ;
		///< Repositions to the last position (so that next() will soon go off 
		///< the end), or to the root() if currently reversed. Returns true 
		///< if the current path changes. This re-examines the file-system, 
		///< so the new position may be unexpectedly 'less than' the previous 
		///< position.

	bool first() ;
		///< Repositions to the root() position, or to the last position if 
		///< currently reversed(). Returns true if the current path changes. 
		///< This re-examines the file-system, so the new position 
		///< may be unexpectedly 'greater than' the previous position.

	void reverse( bool = true ) ;
		///< Iterate in reverse, and reposition() to at-or-before rather than
		///< at-or-after.

	bool reversed() const ;
		///< Returns true if currently reverse()d.

	void swap( FileTree & other ) ;
		///< Swaps this and other.

private:
	bool m_moved ;
	G::Path m_root ;
	DirectoryTree m_p ;
	DirectoryTreeCallback * m_callback ;
} ;

namespace G
{
	inline
	void swap( FileTree & a , FileTree & b )
	{
		a.swap( b ) ;
	}
}

#endif

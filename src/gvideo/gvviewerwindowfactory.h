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
/// \file gvviewerwindowfactory.h
///

#ifndef GV_VIEWERWINDOWFACTORY__H
#define GV_VIEWERWINDOWFACTORY__H

#include "gdef.h"
#include "gvviewerwindow.h"

namespace Gv
{
	class ViewerWindowFactory ;
}

/// \class Gv::ViewerWindowFactory
/// A factory class for Gv::ViewerWindow.
/// 
class Gv::ViewerWindowFactory
{
public:
	static ViewerWindow * create( ViewerWindow::Handler & , ViewerWindowConfig , int dx , int dy ) ;
		///< A factory function that returns a new'ed ViewerWindow.
		///< As soon as it has been stored in a smart-pointer it should
		///< have its init() method called.
} ;

#endif

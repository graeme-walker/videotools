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
/// \file gvviewerwindow_x.h
///

#ifndef GV_VIEWERWINDOW_X__H
#define GV_VIEWERWINDOW_X__H

#include "gdef.h"
#include "gvviewerwindow.h"
#include "gvmask.h"
#include "gxeventloop.h"
#include "geventhandler.h"
#include "gxdisplay.h"
#include "gxwindow.h"
#include "gxcanvas.h"

namespace Gv
{
	class ViewerWindowX ;
	class ViewerWindowXBase ;
}

/// \class Gv::ViewerWindowXBase
/// A private base class for ViewerWindowX that initialises the display before 
/// the window is created by the GX::Window base class.
/// 
class Gv::ViewerWindowXBase
{
public:
	ViewerWindowXBase() ;
	GX::Display m_base_display ;
	GX::EventLoop m_base_xloop ;
} ;

/// \class Gv::ViewerWindowX
/// A pimple-pattern implementation class for ViewerWindow that uses X11.
/// 
class Gv::ViewerWindowX : private Gv::ViewerWindowXBase , private GX::Window , private GNet::EventHandler, public Gv::ViewerWindow
{
public:
	ViewerWindowX( ViewerWindowHandler & , ViewerWindowConfig , int dx , int dy ) ;
		///< Constructor.

	virtual ~ViewerWindowX() ;
		///< Destructor.

	virtual void init() override ;
		///< Override from ViewerWindow.

	virtual void display( int , int , int , const char * , size_t ) override ;
		///< Override from ViewerWindow.

	virtual int dx() const override ;
		///< Override from ViewerWindow.

	virtual int dy() const override ;
		///< Override from ViewerWindow.

private:
	virtual void onChar( char ) override ; // GX::Window
	virtual void onLeftMouseButtonDown( int x , int y , bool shift , bool control ) override ; // GX::Window
	virtual void onLeftMouseButtonUp( int x , int y , bool shift , bool control ) override ; // GX::Window
	virtual void onMouseMove( int x , int y ) override ; // GX::Window
	virtual void onExpose( XExposeEvent & ) override ; // GX::Window
	virtual void readEvent() override ; // GNet::EventHandler
	virtual void onException( std::exception & ) override ; // GNet::EventHandler
	void update( int , int , int , int , const unsigned char * , size_t ) ;
	void update( int , int , int , int , int , const unsigned char * , size_t ) ;

private:
	GX::EventLoop & m_xloop ;
	ViewerWindowHandler & m_window_handler ;
	GX::Canvas m_canvas ;
	ViewerWindowConfig m_config ;
	int m_dx ;
	int m_dy ;
	Gv::Mask m_mask ;
} ;


inline
Gv::ViewerWindowXBase::ViewerWindowXBase() : 
	m_base_xloop(m_base_display)
{
}

#endif

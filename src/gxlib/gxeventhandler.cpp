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
// gxeventhandler.cpp
//

#include "gdef.h"
#include "gxdef.h"
#include "gxeventhandler.h"
#include "glog.h"
#include <iostream>

GX::EventHandler::~EventHandler()
{
}

bool GX::EventHandler::onIdle()
{
	return false ; // stop
}

void GX::EventHandler::onLeftMouseButtonDown( int x , int y , bool shift , bool control ) 
{
	G_DEBUG( "GX::EventHandler::onLeftMouseButtonDown: " << x << "," << y << " " << (shift?"S":"") << (control?"C":"") ) ;
}

void GX::EventHandler::onLeftMouseButtonUp( int x , int y , bool shift , bool control )
{
	G_DEBUG( "GX::EventHandler::onLeftMouseButtonUp: " << x << "," << y << " " << (shift?"S":"") << (control?"C":"") ) ;
}

void GX::EventHandler::onExposure( int x , int y , int dx , int dy )
{
	G_DEBUG( "GX::EventHandler::onExposure: " << x << "," << y << "," << dx << "," << dy ) ;
}

void GX::EventHandler::onExpose( XExposeEvent & )
{
	G_DEBUG( "GX::EventHandler::onExpose" ) ;
}

void GX::EventHandler::onPaint()
{
	G_DEBUG( "GX::EventHandler::onPaint" ) ;
}

void GX::EventHandler::onKeyPress( XKeyEvent & )
{
	G_DEBUG( "GX::EventHandler::onKeyPress" ) ;
}

void GX::EventHandler::onKeyRelease( XKeyEvent & )
{
	G_DEBUG( "GX::EventHandler::onKeyRelease" ) ;
}

void GX::EventHandler::onMap( XMapEvent & )
{
	G_DEBUG( "GX::EventHandler::onMap" ) ;
}

void GX::EventHandler::onShow()
{
	G_DEBUG( "GX::EventHandler::onShow" ) ;
}

bool GX::EventHandler::onCreate()
{
	G_DEBUG( "GX::EventHandler::onCreate" ) ;
	return true ;
}

void GX::EventHandler::onUser()
{
	G_DEBUG( "GX::EventHandler::onUser" ) ;
}

void GX::EventHandler::onMouseMove( int x , int y )
{
	G_DEBUG( "GX::EventHandler::onMouseMove: " << x << "," << y ) ;
}

/// \file gxeventhandler.cpp

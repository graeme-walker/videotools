#
## Copyright (C) 2017 Graeme Walker
## 
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# gxlib/Makefile.am

noinst_LIBRARIES = libgxlib.a

AM_CPPFLAGS = -I$(top_srcdir)/src/grlib -I$(top_srcdir)/src/glib

sources = \
	gxcanvas.cpp \
	gxcanvas.h \
	gxwindow.cpp \
	gxwindow.h \
	gxcapture.h \
	gxcolourmap.cpp \
	gxcolourmap.h \
	gxcontext.cpp \
	gxcontext.h \
	gxdef.h \
	gxdisplay.cpp \
	gxdisplay.h \
	gxdrawable.cpp \
	gxdrawable.h \
	gxerror.cpp \
	gxerror.h \
	gxeventhandler.cpp \
	gxeventhandler.h \
	gxeventloop.cpp \
	gxeventloop.h \
	gxeventqueue.h \
	gximage.cpp \
	gximage.h \
	gxpixmap.cpp \
	gxpixmap.h \
	gxvisual.cpp \
	gxvisual.h \
	gxwindow.cpp \
	gxwindow.h \
	gxwindowmap.cpp \
	gxwindowmap.h

if GCONFIG_X11
libgxlib_a_SOURCES = $(sources)
EXTRA_DIST = gxempty.cpp
else
libgxlib_a_SOURCES = gxempty.cpp
EXTRA_DIST = $(sources)
endif


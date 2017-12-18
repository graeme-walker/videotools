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
# gvideo/Makefile.am

noinst_LIBRARIES = libgvideo.a

EXTRA_DIST = \
	doxygen.h

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gxlib \
	-I$(top_srcdir)/src/grlib

libgvideo_a_SOURCES = \
	gvavcreader.h \
	gvbars.cpp \
	gvbars.h \
	gvcache.cpp \
	gvcache.h \
	gvcamera.cpp \
	gvcamera.h \
	gvcapturebuffer.cpp \
	gvcapturebuffer.h \
	gvcapture.cpp \
	gvcapture_test.cpp \
	gvcapture_test.h \
	gvcapturefactory.h \
	gvcapture.h \
	gvcapture_v4l.h \
	gvclientholder.h \
	gvcommandsocket.h \
	gvcommandsocket.cpp \
	gvdatabase.cpp \
	gvdatabase.h \
	gvdemo.cpp \
	gvdemo.h \
	gvdemodata.h \
	gvexit.h \
	gvhttpserver.cpp \
	gvhttpserver.h \
	gvhttpserverpeer.cpp \
	gvhttpserverpeer.h \
	gvimagegenerator.cpp \
	gvimagegenerator.h \
	gvimageinput.cpp \
	gvimageinput.h \
	gvimageoutput.cpp \
	gvimageoutput.h \
	gvmask.cpp \
	gvmask.h \
	gvmulticast.cpp \
	gvmulticast.h \
	gvoverlay.cpp \
	gvoverlay.h \
	gvribbon.cpp \
	gvribbon.h \
	gvrtpavcpacket.cpp \
	gvrtpavcpacket.h \
	gvrtpjpegpacket.cpp \
	gvrtpjpegpacket.h \
	gvrtppacket.cpp \
	gvrtppacket.h \
	gvrtppacketstream.cpp \
	gvrtppacketstream.h \
	gvrtpserver.cpp \
	gvrtpserver.h \
	gvsdp.cpp \
	gvsdp.h \
	gvstartup.cpp \
	gvstartup.h \
	gvtimezone.cpp \
	gvtimezone.h \
	gvviewerevent.cpp \
	gvviewerevent.h \
	gvviewerinput.cpp \
	gvviewerinput.h \
	gvviewerwindow.cpp \
	gvviewerwindow_ansi.cpp \
	gvviewerwindow_ansi.h \
	gvviewerwindowfactory.h \
	gvviewerwindowfactory.cpp \
	gvviewerwindow.h

if GCONFIG_LIBAV
libgvideo_a_SOURCES += gvavcreader_libav.cpp
EXTRA_DIST += gvavcreader_none.cpp
else
libgvideo_a_SOURCES += gvavcreader_none.cpp
EXTRA_DIST += gvavcreader_libav.cpp
endif

if GCONFIG_V4L
libgvideo_a_SOURCES += gvcapture_v4l.cpp gvcapturefactory_v4l.cpp
EXTRA_DIST += gvcapturefactory_test.cpp
else
libgvideo_a_SOURCES += gvcapturefactory_test.cpp
EXTRA_DIST += gvcapture_v4l.cpp gvcapturefactory_v4l.cpp
endif

if GCONFIG_X11
libgvideo_a_SOURCES += \
	gvviewerwindow_x.h \
	gvviewerwindow_x.cpp
else
EXTRA_DIST += \
	gvviewerwindow_x.h \
	gvviewerwindow_x.cpp
endif

if GCONFIG_CURSES
libgvideo_a_SOURCES += \
	gvviewerwindow_curses.cpp \
	gvviewerwindow_curses.h
else
EXTRA_DIST += \
	gvviewerwindow_curses.cpp \
	gvviewerwindow_curses.h
endif

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
# main/Makefile.am

bin_PROGRAMS = \
	vt-alarm \
	vt-webcamserver \
	vt-webcamplayer \
	vt-channel \
	vt-fileplayer \
	vt-httpclient \
	vt-httpserver \
	vt-maskeditor \
	vt-recorder \
	vt-rtpserver \
	vt-rtspclient \
	vt-socket \
	vt-viewer \
	vt-watcher

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gxlib \
	-I$(top_srcdir)/src/grlib \
	-I$(top_srcdir)/src/gvideo

local_libraries = \
	$(top_builddir)/src/gvideo/libgvideo.a \
	$(top_builddir)/src/gxlib/libgxlib.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/grlib/libgrlib.a \
	$(top_builddir)/src/glib/libglib.a

vt_alarm_SOURCES = alarm.cpp
vt_alarm_LDADD = $(local_libraries)
vt_channel_SOURCES = channel.cpp
vt_channel_LDADD = $(local_libraries)
vt_maskeditor_SOURCES = maskeditor.cpp
vt_maskeditor_LDADD = $(local_libraries)
vt_fileplayer_SOURCES = fileplayer.cpp
vt_fileplayer_LDADD = $(local_libraries)
vt_httpclient_SOURCES = httpclient.cpp
vt_httpclient_LDADD = $(local_libraries)
vt_httpserver_SOURCES = httpserver.cpp
vt_httpserver_LDADD = $(local_libraries)
vt_recorder_SOURCES = recorder.cpp
vt_recorder_LDADD = $(local_libraries)
vt_rtpserver_SOURCES = rtpserver.cpp
vt_rtpserver_LDADD = $(local_libraries)
vt_rtspclient_SOURCES = rtspclient.cpp
vt_rtspclient_LDADD = $(local_libraries)
vt_socket_SOURCES = socket.cpp
vt_socket_LDADD = $(local_libraries)
vt_viewer_SOURCES = viewer.cpp
vt_viewer_LDADD = $(local_libraries)
vt_watcher_SOURCES = watcher.cpp
vt_watcher_LDADD = $(local_libraries)
vt_webcamplayer_SOURCES = webcamplayer.cpp
vt_webcamplayer_LDADD = $(local_libraries)
vt_webcamserver_SOURCES = webcamserver.cpp
vt_webcamserver_LDADD = $(local_libraries)


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
# glib/Makefile.am

noinst_LIBRARIES = libglib.a

EXTRA_DIST =

libglib_a_SOURCES = \
	garg.cpp \
	garg.h \
	gassert.h \
	gbase64.cpp \
	gbase64.h \
	gbititerator.h \
	gbitstream.h \
	gcleanup.h \
	gcleanup_unix.cpp \
	gconvert.cpp \
	gconvert.h \
	gdaemon.h \
	gdaemon_unix.cpp \
	gdate.cpp \
	gdate.h \
	gdatetime.cpp \
	gdatetime.h \
	gdatetime_unix.cpp \
	gdebug.h \
	gdef.h \
	gdirectory.cpp \
	gdirectory.h \
	gdirectorytree.cpp \
	gdirectorytree.h \
	gdirectory_unix.cpp \
	genvironment.h \
	genvironment_unix.cpp \
	gexception.cpp \
	gexception.h \
	gexecutable.cpp \
	gexecutable.h \
	gexecutable_unix.cpp \
	gexpgolomb.h \
	gfatpipe.cpp \
	gfatpipe.h \
	gfile.cpp \
	gfile.h \
	gfile_unix.cpp \
	gfiletree.cpp \
	gfiletree.h \
	ggetopt.cpp \
	ggetopt.h \
	ghexdump.h \
	ghostname.h \
	ghostname_unix.cpp \
	gidentity.h \
	gidentity_unix.cpp \
	gitem.h \
	gitem.cpp \
	glimits.h \
	glocalsocket.cpp \
	glocalsocket.h \
	glog.cpp \
	glog.h \
	glogoutput.cpp \
	glogoutput.h \
	glogoutput_unix.cpp \
	gmapfile.h \
	gmapfile.cpp \
	gmd5_native.cpp \
	gmd5.h \
	gmemory.h \
	gmsg.cpp \
	gmsg.h \
	gnewprocess.h \
	gnewprocess_unix.cpp \
	gnoncopyable.h \
	goptionmap.h \
	goptionparser.cpp \
	goptionparser.h \
	goptions.cpp \
	goptions.h \
	goptionvalue.h \
	gpath.cpp \
	gpath.h \
	gpidfile.cpp \
	gpidfile.h \
	gprocess.h \
	gprocess_unix.cpp \
	gpublisher.cpp \
	gpublisher.h \
	greadwrite.cpp \
	greadwrite.h \
	groot.cpp \
	groot.h \
	gsemaphore.h \
	gsharedmemory.cpp \
	gsharedmemory.h \
	gsignalsafe.h \
	gsleep.h \
	gslot.h \
	gslot.cpp \
	gstaticassert.h \
	gstr.cpp \
	gstr.h \
	gstrings.cpp \
	gstrings.h \
	gtest.cpp \
	gtest.h \
	gthread.cpp \
	gtime.cpp \
	gtime.h \
	gurl.cpp \
	gurl.h \
	md5.cpp \
	md5.h

if GCONFIG_SEMINIT
libglib_a_SOURCES += gsemaphore_posix.cpp
EXTRA_DIST += gsemaphore_sysv.cpp
else
libglib_a_SOURCES += gsemaphore_sysv.cpp
EXTRA_DIST += gsemaphore_posix.cpp
endif

if GCONFIG_ICONV
libglib_a_SOURCES += gconvert_unix.cpp
EXTRA_DIST += gconvert_none.cpp
else
libglib_a_SOURCES += gconvert_none.cpp
EXTRA_DIST += gconvert_unix.cpp
endif


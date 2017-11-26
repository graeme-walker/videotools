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
# grlib/Makefile.am

noinst_LIBRARIES = libgrlib.a

AM_CPPFLAGS = -I$(top_srcdir)/src/glib

EXTRA_DIST =

libgrlib_a_SOURCES = \
	gravc.cpp \
	gravc.h \
	grcolour.h \
	grcolour16.h \
	grcolourspace.cpp \
	grcolourspace.h \
	grcolourspacemap.h \
	grcolourspacematrix.h \
	grcolourspaceranges.h \
	grcolourspacetables.h \
	grcolourspacetypes.h \
	grdef.h \
	grglyph.cpp \
	grglyph.h \
	grhistogram.h \
	grimage.cpp \
	grimage.h \
	grimagebuffer.h \
	grimageconverter.cpp \
	grimageconverter.h \
	grimagedata.cpp \
	grimagedata.h \
	grimagedecoder.cpp \
	grimagedecoder.h \
	grimagetype.cpp \
	grimagetype.h \
	grjpeg.cpp \
	grjpeg.h \
	grlinedraw.h \
	grpng.h \
	grpnm.cpp \
	grpnm.h \
	grscaler.h \
	grtraits.h \
	grvectors.cpp \
	grvectors.h

if GCONFIG_LIBPNG
libgrlib_a_SOURCES += grpng_png.cpp
EXTRA_DIST += grpng_none.cpp
else
libgrlib_a_SOURCES += grpng_none.cpp
EXTRA_DIST += grpng_png.cpp
endif

if GCONFIG_LIBJPEG
libgrlib_a_SOURCES += grjpeg_jpeg.cpp
EXTRA_DIST += grjpeg_none.cpp
else
libgrlib_a_SOURCES += grjpeg_none.cpp
EXTRA_DIST += grjpeg_jpeg.cpp
endif


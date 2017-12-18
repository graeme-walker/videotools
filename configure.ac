dnl Copyright (C) 2017 Graeme Walker
dnl 
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl ===
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

AC_INIT([videotools],[1.0.1],[],[videotools])
AC_CONFIG_SRCDIR([src/glib/gdef.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([no-define])
AC_CONFIG_HEADERS([gconfig_defs.h])
AM_MAINTAINER_MODE
AC_DISABLE_OPTION_CHECKING

dnl ===
dnl check for programs...
dnl
AC_PROG_CC([cc gcc clang])
AC_PROG_CXX([c++ g++ clang++])
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_SED
AC_PROG_GREP
AC_CHECK_PROGS(MAKE,make gmake)
AC_CHECK_PROGS(AR,ar gar)
AC_CHECK_PROGS(GZIP,gzip)
AC_CHECK_PROG(GCONFIG_HAVE_DOXYGEN,doxygen,yes)

dnl ===
dnl check for libraries...
dnl
AC_LANG([C])
GCONFIG_FN_SEARCHLIBS_POSIX
GCONFIG_FN_SEARCHLIBS_JPEG
GCONFIG_FN_SEARCHLIBS_PNG
GCONFIG_FN_SEARCHLIBS_LIBAV
GCONFIG_FN_SEARCHLIBS_CURSES
GCONFIG_FN_SEARCHLIBS_X11

dnl ===
dnl check for header files, functions and typedefs...
dnl
AC_LANG([C++])
GCONFIG_FN_TYPE_SOCKLEN_T
GCONFIG_FN_TYPE_ERRNO_T
GCONFIG_FN_TYPE_SSIZE_T
GCONFIG_FN_SEMAPHORES
GCONFIG_FN_CXX_NULLPTR
GCONFIG_FN_CXX_CONSTEXPR
GCONFIG_FN_CXX_NOEXCEPT
GCONFIG_FN_CXX_OVERRIDE
GCONFIG_FN_CXX_FINAL
GCONFIG_FN_CXX_TYPE_TRAITS
GCONFIG_FN_CXX_EMPLACE
GCONFIG_FN_CXX_SHARED_PTR
GCONFIG_FN_CXX_STD_THREAD
GCONFIG_FN_IPV6
GCONFIG_FN_SIN6_LEN
GCONFIG_FN_STATBUF_NSEC
GCONFIG_FN_IF_NAMETOINDEX
GCONFIG_FN_INET_NTOP
GCONFIG_FN_INET_PTON
GCONFIG_FN_IP_MREQN
GCONFIG_FN_FMEMOPEN
GCONFIG_FN_GETPWNAM
GCONFIG_FN_GETPWNAM_R
GCONFIG_FN_GMTIME_R
GCONFIG_FN_GMTIME_S
GCONFIG_FN_LOCALTIME_R
GCONFIG_FN_LOCALTIME_S
GCONFIG_FN_STRNCPY_S
GCONFIG_FN_SETGROUPS
GCONFIG_FN_GETENV_S
GCONFIG_FN_READLINK
GCONFIG_FN_PROC_PIDPATH
GCONFIG_FN_ICONV
GCONFIG_FN_CURSES
GCONFIG_FN_V4L
GCONFIG_FN_PNG
GCONFIG_FN_JPEG
GCONFIG_FN_LIBAV
GCONFIG_FN_LIBAV_NEW_FRAME_ALLOC_FN
GCONFIG_FN_LIBAV_NEW_DECODE_FN
GCONFIG_FN_LIBAV_NEW_DESCRIPTOR
GCONFIG_FN_X11

dnl ===
dnl initialise aclocal/pkg.m4...
dnl
# if the configure script fails in this area then you are probably missing pkg.m4
PKG_PROG_PKG_CONFIG(0.9.0)

dnl ===
dnl "--disable-install-hook"
dnl
GCONFIG_FN_ENABLE_INSTALL_HOOK

dnl ===
dnl "--enable-std-thread"
dnl
AC_ARG_ENABLE(std-thread,AS_HELP_STRING([--enable-std-thread],[use std::thread (default auto)]))
GCONFIG_FN_ENABLE_STD_THREAD

dnl ===
dnl "--enable-debug"
dnl
AC_ARG_ENABLE(debug,AS_HELP_STRING([--enable-debug],[enable extra debug messages at compile-time (default no)]))
GCONFIG_FN_ENABLE_DEBUG

dnl ===
dnl "--with-doxygen"
dnl
AC_ARG_WITH(doxygen,AS_HELP_STRING([--with-doxygen],[generate source code documentation with doxygen (default auto)]))
GCONFIG_FN_WITH_DOXYGEN

dnl ===
dnl "--with-libv4l"
dnl
AC_ARG_WITH(libv4l,AS_HELP_STRING([--with-libv4l],[use video-for-linux libv4l2 helper library (default no)]))
GCONFIG_FN_WITH_LIBV4L

dnl ===
dnl "--enable-install-hook"
dnl
AC_ARG_ENABLE([install-hook],AS_HELP_STRING([--enable-install-hook],[enable extra make-install steps (default yes)]))
GCONFIG_FN_ENABLE_INSTALL_HOOK

dnl ===
dnl install directory tweaks
dnl
GCONFIG_FN_SET_DIRECTORIES_X

dnl ===
dnl generate files...
dnl
AC_CONFIG_FILES([Makefile src/Makefile src/glib/Makefile src/gnet/Makefile src/gxlib/Makefile src/grlib/Makefile src/gvideo/Makefile src/main/Makefile bin/Makefile doc/Makefile m4/Makefile man/Makefile])
AC_OUTPUT

dnl ===
dnl final warnings...
dnl
GCONFIG_FN_WARNINGS

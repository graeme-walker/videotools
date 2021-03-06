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

dnl GCONFIG_FN_SEARCHLIBS_POSIX
dnl ---------------------------
dnl Does AC_SEARCH_LIBS for various posix functions.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_POSIX],[
	AC_SEARCH_LIBS([gethostbyname],[nsl])
	AC_SEARCH_LIBS([connect],[socket])
	AC_SEARCH_LIBS([shm_open],[rt])
	AC_SEARCH_LIBS([dlopen],[dl])
])

dnl GCONFIG_FN_SEARCHLIBS_CURSES
dnl ----------------------------
dnl Does AC_SEARCH_LIBS for curses.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_CURSES],[
	AC_SEARCH_LIBS([initscr],[curses],[gconfig_have_libcurses=yes],[gconfig_have_libcurses=no])
])

dnl GCONFIG_FN_SEARCHLIBS_PAM
dnl -------------------------
dnl Does AC_SEARCH_LIBS for pam.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_PAM],[
	AC_SEARCH_LIBS([pam_end],[pam],[gconfig_have_libpam=yes],[gconfig_have_libpam=no])
])

dnl GCONFIG_FN_SEARCHLIBS_JPEG
dnl --------------------------
dnl Does AC_SEARCH_LIBS for jpeg.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_JPEG],[
	AC_SEARCH_LIBS([jpeg_abort],[turbojpeg jpeg],[gconfig_have_libjpeg=yes],[gconfig_have_libjpeg=no])
])

dnl GCONFIG_FN_SEARCHLIBS_PNG
dnl -------------------------
dnl Does AC_SEARCH_LIBS for png.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_PNG],[
	AC_SEARCH_LIBS([png_create_read_struct],[png],[gconfig_have_libpng=yes],[gconfig_have_libpng=no])
])

dnl GCONFIG_FN_SEARCHLIBS_LIBAV
dnl ---------------------------
dnl Does AC_SEARCH_LIBS for libav.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_LIBAV],[
	AC_SEARCH_LIBS([avcodec_register_all],[avcodec],[gconfig_have_libav=yes],[gconfig_have_libav=no])
	AC_SEARCH_LIBS([av_log_set_level],[avutil])
])

dnl GCONFIG_FN_SEARCHLIBS_ZLIB
dnl --------------------------
dnl Does AC_SEARCH_LIBS for zlib.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_ZLIB],[
	AC_SEARCH_LIBS([zlibVersion],[z],[gconfig_have_zlib=yes],[gconfig_have_zlib=no])
])

dnl GCONFIG_FN_SEARCHLIBS_X11
dnl -------------------------
dnl Does AC_SEARCH_LIBS for x11.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_X11],[
	AC_SEARCH_LIBS([XOpenDisplay],[X11],[gconfig_have_xlib=yes],[gconfig_have_xlib=no])
])

dnl GCONFIG_FN_PROG_WINDRES
dnl -----------------------
dnl Sets GCONFIG_WINDRES=windres in makefiles as the windows resource compiler,
dnl overridable from the configure command-line.
dnl
AC_DEFUN([GCONFIG_FN_PROG_WINDRES],[
	if test "$GCONFIG_WINDRES" = ""
	then
		GCONFIG_WINDRES="`echo \"$CC\" | grep mingw32 | sed 's/gcc$/windres/'`"
		if test "$GCONFIG_WINDRES" = ""
		then
			GCONFIG_WINDRES="windres"
		fi
	fi
	AC_MSG_CHECKING([for resource compiler])
	AC_MSG_RESULT([$GCONFIG_WINDRES])
	AC_SUBST([GCONFIG_WINDRES])
])

dnl GCONFIG_FN_PROG_WINDMC
dnl ----------------------
dnl Sets GCONFIG_WINDMC=... in makefiles as the windows message compiler,
dnl overridable from the configure command-line.
dnl
AC_DEFUN([GCONFIG_FN_PROG_WINDMC],[
	if test "$GCONFIG_WINDMC" = ""
	then
		GCONFIG_WINDMC="`echo \"$CC\" | grep mingw32 | sed 's/gcc$/windmc/'`"
		if test "$GCONFIG_WINDMC" = ""
		then
			GCONFIG_WINDMC="./fakemc.exe"
		fi
	fi
	AC_MSG_CHECKING([message compiler])
	AC_MSG_RESULT([$GCONFIG_WINDMC])
	AC_SUBST([GCONFIG_WINDMC])
])

dnl GCONFIG_FN_SEMAPHORES
dnl ---------------------
dnl Checks for semaphores. Defines GCONFIG_HAVE_SEM_INIT and
dnl adds "-pthread" as necessary. This should be used before
dnl std::thread checks.
dnl
dnl Unfortunately some BSDs have a disfunctional sem_init() 
dnl that fails at run-time.
dnl
AC_DEFUN([GCONFIG_FN_SEMAPHORES],[

	AC_MSG_CHECKING([posix semaphores])
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <semaphore.h>]
		],
		[
			[sem_t sem ;]
			[int rc = sem_init(&sem,1,1) ;]
		])],
		gconfig_seminit=yes ,
		gconfig_seminit=no )
	AC_MSG_RESULT([$gconfig_seminit])

	gconfig_save_CXXFLAGS="$CXXFLAGS"
	gconfig_save_LDFLAGS="$LDFLAGS"
	CXXFLAGS="$CXXFLAGS -pthread"
	LDFLAGS="$LDFLAGS -pthread"
	AC_MSG_CHECKING([posix semaphores with -pthread])
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <semaphore.h>]
		],
		[
			[sem_t sem ;]
			[int rc = sem_init(&sem,1,1) ;]
		])],
		gconfig_seminit_pthread=yes ,
		gconfig_seminit_pthread=no )
	AC_MSG_RESULT([$gconfig_seminit_pthread])
	if test "$gconfig_seminit_pthread" = "no" ; then
		CXXFLAGS="$gconfig_save_CXXFLAGS"
		LDFLAGS="$gconfig_save_LDFLAGS"
	fi

	gconfig_have_seminit=0
	if test "$gconfig_seminit" = "yes" -o "$gconfig_seminit_pthread" = "yes"
	then
		gconfig_have_seminit=1
	fi

	if test "`uname`" = "Darwin"
	then
		gconfig_have_seminit=0
	fi

	if test "$gconfig_have_seminit" -eq 1 ; then
		AC_DEFINE(GCONFIG_HAVE_SEM_INIT,1,[Define true for posix semaphores])
	else
		AC_DEFINE(GCONFIG_HAVE_SEM_INIT,0,[Define true for posix semaphores])
	fi
	AM_CONDITIONAL([GCONFIG_SEMINIT],[test "$gconfig_have_seminit" -eq 1])
])

dnl GCONFIG_FN_CXX_NULLPTR
dnl ----------------------
dnl Tests for c++ nullptr keyword.
dnl
AC_DEFUN([GCONFIG_FN_CXX_NULLPTR],
[AC_CACHE_CHECK([for c++ nullptr],[gconfig_cv_cxx_nullptr],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[],
		[[void * p = nullptr]])],
		gconfig_cv_cxx_nullptr=yes ,
		gconfig_cv_cxx_nullptr=no )
])
	if test "$gconfig_cv_cxx_nullptr" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_NULLPTR,1,[Define true if compiler supports c++ nullptr])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_NULLPTR,0,[Define true if compiler supports c++ nullptr])
	fi
])

dnl GCONFIG_FN_CXX_CONSTEXPR
dnl ------------------------
dnl Tests for c++ constexpr support.
dnl
AC_DEFUN([GCONFIG_FN_CXX_CONSTEXPR],
[AC_CACHE_CHECK([for c++ constexpr],[gconfig_cv_cxx_constexpr],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[template <typename T> struct Foo {static constexpr int foo = 1;};]],
		[[int i = 1 ;]])],
		gconfig_cv_cxx_constexpr=yes ,
		gconfig_cv_cxx_constexpr=no )
])
	if test "$gconfig_cv_cxx_constexpr" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_CONSTEXPR,1,[Define true if compiler supports c++ constexpr])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_CONSTEXPR,0,[Define true if compiler supports c++ constexpr])
	fi
])

dnl GCONFIG_FN_CXX_NOEXCEPT
dnl -----------------------
dnl Tests for c++ noexcept support.
dnl
AC_DEFUN([GCONFIG_FN_CXX_NOEXCEPT],
[AC_CACHE_CHECK([for c++ noexcept],[gconfig_cv_cxx_noexcept],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[void fn() noexcept;]],
		[[int i = 1 ;]])],
		gconfig_cv_cxx_noexcept=yes ,
		gconfig_cv_cxx_noexcept=no )
])
	if test "$gconfig_cv_cxx_noexcept" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_NOEXCEPT,1,[Define true if compiler supports c++ noexcept])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_NOEXCEPT,0,[Define true if compiler supports c++ noexcept])
	fi
])

dnl GCONFIG_FN_CXX_OVERRIDE
dnl -----------------------
dnl Tests for c++ override support.
dnl
AC_DEFUN([GCONFIG_FN_CXX_OVERRIDE],
[AC_CACHE_CHECK([for c++ override],[gconfig_cv_cxx_override],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#if defined(__GNUC__) __cplusplus < 200000L]
			[#error gcc is too noisy when using override/final without std=c++11]
			[#endif]
			[struct base { virtual void fn() {} } ;]
			[struct derived : public base { virtual void fn() override {} } ;]
		],
		[
			[derived d ;]
		])],
		gconfig_cv_cxx_override=yes ,
		gconfig_cv_cxx_override=no )
])
	if test "$gconfig_cv_cxx_override" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_OVERRIDE,1,[Define true if compiler supports c++ override])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_OVERRIDE,0,[Define true if compiler supports c++ override])
	fi
])

dnl GCONFIG_FN_CXX_FINAL
dnl --------------------
dnl Tests for c++ final keyword.
dnl
AC_DEFUN([GCONFIG_FN_CXX_FINAL],
[AC_CACHE_CHECK([for c++ final keyword],[gconfig_cv_cxx_final],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#if defined(__GNUC__) && __cplusplus < 200000L]
			[#error gcc is too noisy when using override/final without std=c++11]
			[#endif]
			[struct base { virtual void fn() {} } ;]
			[struct derived : public base { virtual void fn() final {} } ;]
		],
		[
			[derived d ;]
		])],
		gconfig_cv_cxx_final=yes ,
		gconfig_cv_cxx_final=no )
])
	if test "$gconfig_cv_cxx_final" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_FINAL,1,[Define true if compiler supports c++ final keyword])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_FINAL,0,[Define true if compiler supports c++ final keyword])
	fi
])

dnl GCONFIG_FN_CXX_TYPE_TRAITS
dnl --------------------------
dnl Tests for c++ <type_traits> std::make_unsigned.
dnl
AC_DEFUN([GCONFIG_FN_CXX_TYPE_TRAITS],
[AC_CACHE_CHECK([for c++ type_traits],[gconfig_cv_cxx_type_traits_make_unsigned],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <type_traits>]],
		[[std::make_unsigned<int>::type i = 0U ;]])],
		gconfig_cv_cxx_type_traits_make_unsigned=yes ,
		gconfig_cv_cxx_type_traits_make_unsigned=no )
])
	if test "$gconfig_cv_cxx_type_traits_make_unsigned" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED,1,[Define true if compiler has <type_traits> make_unsigned])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED,0,[Define true if compiler has <type_traits> make_unsigned])
	fi
])

dnl GCONFIG_FN_CXX_EMPLACE
dnl ----------------------
dnl Tests for c++ std::vector::emplace_back() etc.
dnl
AC_DEFUN([GCONFIG_FN_CXX_EMPLACE],
[AC_CACHE_CHECK([for c++ emplace_back and friends],[gconfig_cv_cxx_emplace],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <vector>]],
		[[std::vector<int> v; v.emplace_back(1);]])],
		gconfig_cv_cxx_emplace=yes ,
		gconfig_cv_cxx_emplace=no )
])
	if test "$gconfig_cv_cxx_emplace" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_EMPLACE,1,[Define true if compiler has std::vector::emplace_back()])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_EMPLACE,0,[Define true if compiler has std::vector::emplace_back()])
	fi
])

dnl GCONFIG_FN_CXX_SHARED_PTR
dnl -------------------------
dnl Tests for c++ std::shared_ptr.
dnl
AC_DEFUN([GCONFIG_FN_CXX_SHARED_PTR],
[AC_CACHE_CHECK([for c++ std::shared_ptr and friends],[gconfig_cv_cxx_std_shared_ptr],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <memory>]],
		[[typedef std::shared_ptr<int> ptr;]])],
		gconfig_cv_cxx_std_shared_ptr=yes ,
		gconfig_cv_cxx_std_shared_ptr=no )
])
	if test "$gconfig_cv_cxx_std_shared_ptr" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_SHARED_PTR,1,[Define true if compiler has std::shared_ptr])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_SHARED_PTR,0,[Define true if compiler has std::shared_ptr])
	fi
])

dnl GCONFIG_FN_CXX_STD_THREAD
dnl -------------------------
dnl Tests for a viable c++ std::thread class under the current compile and link options.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STD_THREAD],
[AC_CACHE_CHECK([for c++ std::thread],[gconfig_cv_cxx_std_thread],
[
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <thread>]
			[#include <cstdio>]
		],
		[
			[std::thread t(std::fopen,"/dev/null","r");]
			[t.join();]
		])],
		gconfig_cv_cxx_std_thread=yes ,
		gconfig_cv_cxx_std_thread=no )
])
	if test "$gconfig_cv_cxx_std_thread" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,1,[Define true if compiler has std::thread])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,0,[Define true if compiler has std::thread])
	fi
])

dnl GCONFIG_FN_STATBUF_NSEC
dnl -----------------------
dnl Tests whether stat provides nanosecond file times.
dnl
AC_DEFUN([GCONFIG_FN_STATBUF_NSEC],
[AC_CACHE_CHECK([for statbuf nanoseconds],[gconfig_cv_statbuf_nsec],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/stat.h>]
				[#include <unistd.h>]
			[#endif]
		],
		[[struct stat statbuf; statbuf.st_atim.tv_nsec = 0;]])],
		gconfig_cv_statbuf_nsec=yes ,
		gconfig_cv_statbuf_nsec=no )
])
	if test "$gconfig_cv_statbuf_nsec" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_STATBUF_NSEC,1,[Define true if statbuf has a st_atim.tv_nsec member])
	else
		AC_DEFINE(GCONFIG_HAVE_STATBUF_NSEC,0,[Define true if statbuf has a st_atim.tv_nsec member])
	fi
])

dnl GCONFIG_FN_TYPE_SOCKETLEN_T
dnl ---------------------------
dnl Tests for socklen_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_SOCKLEN_T],
[AC_CACHE_CHECK([for socklen_t],[gconfig_cv_type_socklen_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#ifdef _WIN32_WINNT]
					[#undef _WIN32_WINNT]
				[#endif]
				[#define _WIN32_WINNT _WIN32_WINNT_WINXP]
				[#include <winsock2.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
			[#endif]
		],
		[[socklen_t len = 42; return (int)len;]])],
		gconfig_cv_type_socklen_t=yes,
		gconfig_cv_type_socklen_t=no )
])
	if test "$gconfig_cv_type_socklen_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SOCKLEN_T,1,[Define true if socklen_t type definition in sys/socket.h])
	else
		AC_DEFINE(GCONFIG_HAVE_SOCKLEN_T,0,[Define true if socklen_t type definition in sys/socket.h])
	fi
])

dnl GCONFIG_FN_TYPE_ERRNO_T
dnl -----------------------
dnl Tests for errno_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_ERRNO_T],
[AC_CACHE_CHECK([for errno_t],[gconfig_cv_type_errno_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <error.h>]],
		[[errno_t e = 42; return (int)e;]])],
		gconfig_cv_type_errno_t=yes,
		gconfig_cv_type_errno_t=no )
])
	if test "$gconfig_cv_type_errno_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_ERRNO_T,1,[Define true if errno_t type definition in error.h])
	else
		AC_DEFINE(GCONFIG_HAVE_ERRNO_T,0,[Define true if errno_t type definition in error.h])
	fi
])

dnl GCONFIG_FN_TYPE_SSIZE_T
dnl -----------------------
dnl Tests for ssize_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_SSIZE_T],
[AC_CACHE_CHECK([for ssize_t],[gconfig_cv_type_ssize_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <stdio.h>]
		],
		[[ssize_t e = 42; return (int)e;]])],
		gconfig_cv_type_ssize_t=yes,
		gconfig_cv_type_ssize_t=no )
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <unistd.h>]
		],
		[[ssize_t e = 42; return (int)e;]])],
		gconfig_cv_type_ssize_t=yes )
])
	if test "$gconfig_cv_type_ssize_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SSIZE_T,1,[Define true if ssize_t type is defined])
	else
		AC_DEFINE(GCONFIG_HAVE_SSIZE_T,0,[Define true if ssize_t type is defined])
	fi
])

dnl GCONFIG_FN_IPV6
dnl ---------------
dnl Tests for a minimum set of IPv6 features available.
dnl
AC_DEFUN([GCONFIG_FN_IPV6],
[AC_CACHE_CHECK([for ipv6],[gconfig_cv_ipv6],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#ifdef _WIN32_WINNT]
					[#undef _WIN32_WINNT]
				[#endif]
				[#define _WIN32_WINNT _WIN32_WINNT_WINXP]
				[#include <winsock2.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/socket.h>]
				[#include <netinet/in.h>]
				[#include <arpa/inet.h>]
				[#include <netdb.h>]
			[#endif]
		],
		[
			[struct sockaddr_in6 * p = 0;]
			[int f = AF_INET6;]
			[struct addrinfo * ai = 0;]
			[getaddrinfo("","",ai,&ai);]
		])],
		gconfig_cv_ipv6=yes ,
		gconfig_cv_ipv6=no )
])
	if test "$gconfig_cv_ipv6" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IPV6,1,[Define true if ipv6 is available])
	else
		AC_DEFINE(GCONFIG_HAVE_IPV6,0,[Define true if ipv6 is available])
	fi
])

dnl GCONFIG_FN_INET_PTON
dnl --------------------
dnl Tests for inet_pton().
dnl
AC_DEFUN([GCONFIG_FN_INET_PTON],
[AC_CACHE_CHECK([for inet_pton()],[gconfig_cv_inet_pton],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#ifdef _WIN32_WINNT]
					[#undef _WIN32_WINNT]
				[#endif]
				[#define _WIN32_WINNT _WIN32_WINNT_WINXP]
				[#include <winsock2.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <arpa/inet.h>]
			[#endif]
		],
		[[inet_pton(0,"",(void*)0);]])],
		gconfig_cv_inet_pton=yes ,
		gconfig_cv_inet_pton=no )
])
	if test "$gconfig_cv_inet_pton" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INET_PTON,1,[Define true if inet_pton() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_INET_PTON,0,[Define true if inet_pton() is available])
	fi
])

dnl GCONFIG_FN_IF_NAMETOINDEX
dnl -------------------------
dnl Tests for if_nametoindex().
dnl
AC_DEFUN([GCONFIG_FN_IF_NAMETOINDEX],
[AC_CACHE_CHECK([for if_nametoindex()],[gconfig_cv_if_nametoindex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#ifdef _WIN32_WINNT]
					[#undef _WIN32_WINNT]
				[#endif]
				[#define _WIN32_WINNT _WIN32_WINNT_WINXP]
				[#include <winsock2.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <arpa/inet.h>]
				[#include <net/if.h>]
			[#endif]
		],
		[[unsigned int i = if_nametoindex("");]])],
		gconfig_cv_if_nametoindex=yes ,
		gconfig_cv_if_nametoindex=no )
])
	if test "$gconfig_cv_if_nametoindex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IF_NAMETOINDEX,1,[Define true if if_nametoindex() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_IF_NAMETOINDEX,0,[Define true if if_nametoindex() is available])
	fi
])

dnl GCONFIG_FN_INET_NTOP
dnl --------------------
dnl Tests for inet_ntop().
dnl
AC_DEFUN([GCONFIG_FN_INET_NTOP],
[AC_CACHE_CHECK([for inet_ntop()],[gconfig_cv_inet_ntop],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#ifdef _WIN32_WINNT]
					[#undef _WIN32_WINNT]
				[#endif]
				[#define _WIN32_WINNT _WIN32_WINNT_WINXP]
				[#include <winsock2.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <arpa/inet.h>]
			[#endif]
		],
		[[inet_ntop(0,"",0,0);]])],
		gconfig_cv_inet_ntop=yes ,
		gconfig_cv_inet_ntop=no )
])
	if test "$gconfig_cv_inet_ntop" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INET_NTOP,1,[Define true if inet_ntop() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_INET_NTOP,0,[Define true if inet_ntop() is available])
	fi
])

dnl GCONFIG_FN_SIN6_LEN
dnl -------------------
dnl Tests whether sin6_len is in sockaddr_in6.
dnl
AC_DEFUN([GCONFIG_FN_SIN6_LEN],
[AC_CACHE_CHECK([for sin6_len],[gconfig_cv_sin6_len],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#ifdef _WIN32_WINNT]
					[#undef _WIN32_WINNT]
				[#endif]
				[#define _WIN32_WINNT _WIN32_WINNT_WINXP]
				[#include <winsock2.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <netinet/in.h>]
			[#endif]
		],
		[[struct sockaddr_in6 s; s.sin6_len = 1;]])],
		gconfig_cv_sin6_len=yes ,
		gconfig_cv_sin6_len=no )
])
	if test "$gconfig_cv_sin6_len" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SIN6_LEN,1,[Define true if sockaddr_in6 has a sin6_len member])
	else
		AC_DEFINE(GCONFIG_HAVE_SIN6_LEN,0,[Define true if sockaddr_in6 has a sin6_len member])
	fi
])

dnl GCONFIG_FN_IP_MREQN
dnl -------------------
dnl Tests for multicast structure ip_mreqn.
dnl
AC_DEFUN([GCONFIG_FN_IP_MREQN],
[AC_CACHE_CHECK([for struct ip_mreqn],[gconfig_cv_ip_mreqn],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/socket.h>]
			[#include <netinet/in.h>]
			[#include <netinet/ip.h>]
		],
		[[struct ip_mreqn m ; m.imr_ifindex = 0]])],
		gconfig_cv_ip_mreqn=yes ,
		gconfig_cv_ip_mreqn=no )
])
	if test "$gconfig_cv_ip_mreqn" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IP_MREQN,1,[Define true if struct ip_mreqn is available])
	else
		AC_DEFINE(GCONFIG_HAVE_IP_MREQN,0,[Define true if struct ip_mreqn is available])
	fi
])

dnl GCONFIG_FN_FMEMOPEN
dnl -------------------
dnl Tests for fmemopen().
dnl
AC_DEFUN([GCONFIG_FN_FMEMOPEN],
[AC_CACHE_CHECK([for fmemopen],[gconfig_cv_fmemopen],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdio.h>]
		],
		[[FILE * fp = fmemopen((void*)"",1U,"r")]])],
		gconfig_cv_fmemopen=yes ,
		gconfig_cv_fmemopen=no )
])
	if test "$gconfig_cv_fmemopen" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_FMEMOPEN,1,[Define true if fmemopen is available])
	else
		AC_DEFINE(GCONFIG_HAVE_FMEMOPEN,0,[Define true if fmemopen is available])
	fi
])

dnl GCONFIG_FN_SETGROUPS
dnl --------------------
dnl Tests for setgroups().
dnl
AC_DEFUN([GCONFIG_FN_SETGROUPS],
[AC_CACHE_CHECK([for setgroups],[gconfig_cv_setgroups],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <unistd.h>]
			[#include <grp.h>]
		],
		[[setgroups(0,0)]])],
		gconfig_cv_setgroups=yes ,
		gconfig_cv_setgroups=no )
])
	if test "$gconfig_cv_setgroups" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SETGROUPS,1,[Define true if setgroups is available])
	else
		AC_DEFINE(GCONFIG_HAVE_SETGROUPS,0,[Define true if setgroups is available])
	fi
])

dnl GCONFIG_FN_GETPWNAM
dnl -------------------
dnl Tests for getpwnam().
dnl
AC_DEFUN([GCONFIG_FN_GETPWNAM],
[AC_CACHE_CHECK([for getpwnam],[gconfig_cv_getpwnam],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <pwd.h>]
		],
		[[struct passwd *r = getpwnam("")]])],
		gconfig_cv_getpwnam=yes ,
		gconfig_cv_getpwnam=no )
])
	if test "$gconfig_cv_getpwnam" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM,1,[Define true if getpwnam in pwd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM,0,[Define true if getpwnam in pwd.h])
	fi
])

dnl GCONFIG_FN_GETPWNAM_R
dnl ---------------------
dnl Tests for getpwnam_r().
dnl
AC_DEFUN([GCONFIG_FN_GETPWNAM_R],
[AC_CACHE_CHECK([for getpwnam_r],[gconfig_cv_getpwnam_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <pwd.h>]
		],
		[
			[char c;]
			[struct passwd *r;]
			[getpwnam_r("",r,&c,0,&r) ;]
		])],
		gconfig_cv_getpwnam_r=yes ,
		gconfig_cv_getpwnam_r=no )
])
	if test "$gconfig_cv_getpwnam_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM_R,1,[Define true if getpwnam_r in pwd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM_R,0,[Define true if getpwnam_r in pwd.h])
	fi
])

dnl GCONFIG_FN_GMTIME_R
dnl -------------------
dnl Tests for gmtime_r().
dnl
AC_DEFUN([GCONFIG_FN_GMTIME_R],
[AC_CACHE_CHECK([for gmtime_r],[gconfig_cv_gmtime_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <time.h>]],
		[[gmtime_r((time_t*)0,(struct tm*)0) ;]])],
		gconfig_cv_gmtime_r=yes ,
		gconfig_cv_gmtime_r=no )
])
	if test "$gconfig_cv_gmtime_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GMTIME_R,1,[Define true if gmtime_r in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GMTIME_R,0,[Define true if gmtime_r in time.h])
	fi
])

dnl GCONFIG_FN_GMTIME_S
dnl -------------------
dnl Tests for gmtime_s().
dnl
AC_DEFUN([GCONFIG_FN_GMTIME_S],
[AC_CACHE_CHECK([for gmtime_s],[gconfig_cv_gmtime_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <windows.h>]
			[#endif]
			[#include <time.h>]
		],
		[[gmtime_s((struct tm*)0,(time_t*)0) ;]])],
		gconfig_cv_gmtime_s=yes ,
		gconfig_cv_gmtime_s=no )
])
	if test "$gconfig_cv_gmtime_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GMTIME_S,1,[Define true if gmtime_s in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GMTIME_S,0,[Define true if gmtime_s in time.h])
	fi
])

dnl GCONFIG_FN_LOCALTIME_R
dnl ----------------------
dnl Tests for localtime_r().
dnl
AC_DEFUN([GCONFIG_FN_LOCALTIME_R],
[AC_CACHE_CHECK([for localtime_r],[gconfig_cv_localtime_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <time.h>]],
		[[localtime_r((time_t*)0,(struct tm*)0) ;]])],
		gconfig_cv_localtime_r=yes ,
		gconfig_cv_localtime_r=no )
])
	if test "$gconfig_cv_localtime_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_R,1,[Define true if localtime_r in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_R,0,[Define true if localtime_r in time.h])
	fi
])

dnl GCONFIG_FN_LOCALTIME_S
dnl ----------------------
dnl Tests for localtime_s().
dnl
AC_DEFUN([GCONFIG_FN_LOCALTIME_S],
[AC_CACHE_CHECK([for localtime_s],[gconfig_cv_localtime_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <windows.h>]
			[#endif]
			[#include <time.h>]
		],
		[[localtime_s((struct tm*)0,(time_t*)0) ;]])],
		gconfig_cv_localtime_s=yes ,
		gconfig_cv_localtime_s=no )
])
	if test "$gconfig_cv_localtime_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_S,1,[Define true if localtime_s in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_S,0,[Define true if localtime_s in time.h])
	fi
])

dnl GCONFIG_FN_GETENV_S
dnl -------------------
dnl Tests for getenv_s().
dnl
AC_DEFUN([GCONFIG_FN_GETENV_S],
[AC_CACHE_CHECK([for getenv_s],[gconfig_cv_getenv_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <windows.h>]
			[#endif]
			[#include <stdlib.h>]
		],
		[[getenv_s((size_t*)0,(char*)0,0U,(const char*)0) ;]])],
		gconfig_cv_getenv_s=yes ,
		gconfig_cv_getenv_s=no )
])
	if test "$gconfig_cv_getenv_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETENV_S,1,[Define true if getenv_s in stdlib.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETENV_S,0,[Define true if getenv_s in stdlib.h])
	fi
])

dnl GCONFIG_FN_PROC_PIDPATH
dnl -----------------------
dnl Tests for proc_pidpath() (osx).
dnl
AC_DEFUN([GCONFIG_FN_PROC_PIDPATH],
[AC_CACHE_CHECK([for proc_pidpath],[gconfig_cv_proc_pidpath],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdlib.h>]
			[#include <libproc.h>]
		],
		[
			[int rc = proc_pidpath(getpid(),(char*)0,(size_t)0);]
		])],
		gconfig_cv_proc_pidpath=yes ,
		gconfig_cv_proc_pidpath=no )
])
	if test "$gconfig_cv_proc_pidpath" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PROC_PIDPATH,1,[Define true if have OSX proc_pidpath()])
	else
		AC_DEFINE(GCONFIG_HAVE_PROC_PIDPATH,0,[Define true if have OSX proc_pidpath()])
	fi
])

dnl GCONFIG_FN_READLINK
dnl -------------------
dnl Tests for readlink().
dnl
AC_DEFUN([GCONFIG_FN_READLINK],
[AC_CACHE_CHECK([for readlink],[gconfig_cv_readlink],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <unistd.h>]],
		[[readlink((const char*)0,(char*)0,0U);]])],
		gconfig_cv_readlink=yes ,
		gconfig_cv_readlink=no )
])
	if test "$gconfig_cv_readlink" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_READLINK,1,[Define true if have readlink() in unistd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_READLINK,0,[Define true if have readlink() in unistd.h])
	fi
])

dnl GCONFIG_FN_STRNCPY_S
dnl --------------------
dnl Tests for strncpy_s().
dnl
AC_DEFUN([GCONFIG_FN_STRNCPY_S],
[AC_CACHE_CHECK([for strncpy_s],[gconfig_cv_strncpy_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <windows.h>]
			[#endif]
			[#include <string.h>]
		],
		[[strncpy_s((char*)0,0U,"",0U) ;]])],
		gconfig_cv_strncpy_s=yes ,
		gconfig_cv_strncpy_s=no )
])
	if test "$gconfig_cv_strncpy_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_STRNCPY_S,1,[Define true if strncpy_s in string.h])
	else
		AC_DEFINE(GCONFIG_HAVE_STRNCPY_S,0,[Define true if strncpy_s in string.h])
	fi
])

dnl GCONFIG_FN_CONFIGURATION
dnl ------------------------
dnl Sets GCONFIG_CONFIGURATION in makefiles to provide information about the build.
dnl
AC_DEFUN([GCONFIG_FN_CONFIGURATION],
[
changequote(<<,>>)
	GCONFIG_CONFIGURATION="`echo \"$ac_configure_args\" | tr '\n' ' ' | base64 2>/dev/null | tr -d '\n' | tr -d ' '`"
changequote([,])
	AC_SUBST([GCONFIG_CONFIGURATION])
])

dnl GCONFIG_FN_ZLIB
dnl ---------------
dnl Tests for zlib, with AC_SEARCH_LIBS. This is simpler than the macro below 
dnl (GCONFIG_FN_WITH_ZLIB), which is intended to be used with a "--with-zlib" 
dnl configure option.
dnl
AC_DEFUN([GCONFIG_FN_ZLIB],
[AC_CACHE_CHECK([for zlib],[gconfig_cv_zlib],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <zlib.h>]])],
		gconfig_cv_zlib=yes,
		gconfig_cv_zlib=no )
])
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_ZLIB])
    if test "$gconfig_cv_zlib" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_ZLIB,1,[Define true to enable use of zlib])
	else
		AC_DEFINE(GCONFIG_HAVE_ZLIB,0,[Define true to enable use of zlib])
	fi
])

dnl GCONFIG_FN_WITH_ZLIB
dnl --------------------
dnl Tests for zlib, used after AC_ARG_WITH(zlib).
dnl
AC_DEFUN([GCONFIG_FN_WITH_ZLIB],
[AC_CACHE_CHECK([for zlib],[gconfig_cv_zlib],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <zlib.h>]])],
		gconfig_cv_zlib=yes,
		gconfig_cv_zlib=no )
])
	if test "$with_zlib" = "no"
	then
		GCONFIG_ZLIB_LIBS=""
	else
    		if test "$gconfig_cv_zlib" = "yes"
		then
			GCONFIG_ZLIB_LIBS="-lz"
			AC_DEFINE(GCONFIG_HAVE_ZLIB,1,[Define true to enable use of zlib])
		else
			AC_DEFINE(GCONFIG_HAVE_ZLIB,0,[Define true to enable use of zlib])
			if test "$with_zlib" = "yes"
			then
				AC_MSG_WARN([ignoring --with-zlib: check config.log and try setting CFLAGS])
			fi
			GCONFIG_ZLIB_LIBS=""
		fi
	fi
	AC_SUBST([GCONFIG_ZLIB_LIBS])
])

dnl GCONFIG_FN_QT
dnl -------------
dnl Tests for Qt. Sets QT_MOC, QT_LIBS and QT_CFLAGS according to what pkg-config says.
dnl A fallback copy of "pkg.m4" should be included in the distribution.
dnl
AC_DEFUN([GCONFIG_FN_QT],
[
	PKG_CHECK_MODULES([QT],[Qt5Widgets > 5],
		[gconfig_qt=qt5],
		[PKG_CHECK_MODULES([QT],[QtGui > 4],[gconfig_qt=qt4],[gconfig_qt=""])]
	)

	AC_ARG_VAR([QT_MOC],[moc command for QT])

	# find moc from pkg-config
	if test "$QT_MOC" = "" -a "$PKG_CONFIG" != ""
	then
		if test "$gconfig_qt" = "qt5"
		then
			QT_MOC="`$PKG_CONFIG --variable=exec_prefix Qt5Gui`/bin/moc"
			QT_CHOOSER="`$PKG_CONFIG --variable=exec_prefix Qt5Gui`/bin/qtchooser"
			if test -x "$QT_MOC" ; then : ; else QT_MOC="" ; fi
			if test -x "$QT_CHOOSER" ; then : ; else QT_CHOOSER="" ; fi
			if test "$QT_MOC" != "" -a "$QT_CHOOSER" != ""
			then
				QT_MOC="$QT_CHOOSER -run-tool=moc -qt=qt5"
			fi
			if echo "$QT_CFLAGS" | grep -q fPI ; then : ; else
				QT_CFLAGS="$QT_CFLAGS -fPIC"
			fi
		:
		elif test "$gconfig_qt" = "qt4"
		then
			QT_MOC="`$PKG_CONFIG --variable=exec_prefix QtGui`/bin/moc"
			QT_CHOOSER="`$PKG_CONFIG --variable=exec_prefix QtGui`/bin/qtchooser"
			if test -x "$QT_MOC" ; then : ; else QT_MOC="" ; fi
			if test -x "$QT_CHOOSER" ; then : ; else QT_CHOOSER="" ; fi
			if test "$QT_MOC" != "" -a "$QT_CHOOSER" != ""
			then
				QT_MOC="$QT_CHOOSER -run-tool=moc -qt=qt4"
			fi
		fi
	fi

	# if no pkg-config find moc on the path
	if test "$QT_MOC" = ""
	then
		AC_PATH_PROG([QT_MOC],[moc])
	fi

	# special help for mac (frameworks, no pkg-config)
	if test "`uname`" = "Darwin"
	then
		if test "$QT_MOC" != "" -a "$QT_CFLAGS" = "" -a "$QT_LIBS" = ""
		then
			QT_DIR="`dirname $QT_MOC`/.."
			if $QT_MOC -v | grep -q "Qt 4"
			then
				gconfig_qt="qt4"
				QT_CFLAGS="-F $QT_DIR/lib"
				QT_LIBS="-F $QT_DIR/lib -framework QtGui -framework QtCore"
			fi
			if $QT_MOC -v | grep -q "moc 5"
			then
				gconfig_qt="qt5"
				QT_CFLAGS="-F $QT_DIR/lib"
				QT_LIBS="-F $QT_DIR/lib -framework QtWidgets -framework QtGui -framework QtCore"
			fi
		fi
	fi
])

dnl GCONFIG_FN_ENABLE_GUI
dnl ---------------------
dnl Allows for "if GUI" conditionals in makefiles, based on "--enable-gui" or QT_MOC.
dnl Typically used after GCONFIG_FN_QT and AC_ARG_ENABLE(gui).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_GUI],
[
	if test "$enable_gui" = "no"
	then
		gconfig_qt=""
		QT_MOC=""
		QT_LIBS=""
		QT_CFLAGS=""
	fi

	if test "$enable_gui" = "yes"
	then
		if test "$QT_MOC" = "" -o "$QT_LIBS" = "" -o "$QT_CFLAGS" = ""
		then
			AC_MSG_WARN([ignoring --enable-gui: set QT_MOC, QT_LIBS and QT_CFLAGS to override])
			gconfig_qt=""
			QT_MOC=""
			QT_LIBS=""
			QT_CFLAGS=""
		fi
	fi

	if test "$QT_MOC" != ""
	then
		AC_MSG_NOTICE([QT version: $gconfig_qt])
		AC_MSG_NOTICE([QT moc command: $QT_MOC])
	fi

	AC_SUBST([GCONFIG_QT_LIBS],[$QT_LIBS])
	AC_SUBST([GCONFIG_QT_CFLAGS],[$QT_CFLAGS])
	AC_SUBST([GCONFIG_QT_MOC],[$QT_MOC])

	AM_CONDITIONAL([GCONFIG_GUI],[test "$QT_MOC" != ""])
])

dnl GCONFIG_FN_ENABLE_DEBUG
dnl -----------------------
dnl Defines _DEBUG if "--enable-debug". Defaults to "no" but allows 
dnl "--enable-debug=full" as per kdevelop. Typically used after 
dnl AC_ARG_ENABLE(debug).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_DEBUG],
[
	if test "$enable_debug" = "no" -o -z "$enable_debug"
	then
		:
	else
		AC_DEFINE(_DEBUG,1,[Define to enable debug messages at compile-time])
	fi
])

dnl GCONFIG_FN_ENABLE_VERBOSE
dnl -------------------------
dnl Defines "GCONFIG_NO_LOG" if "--disable-verbose". Typically used after 
dnl AC_ARG_ENABLE(verbose).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_VERBOSE],
[
	if test "$enable_verbose" != "no"
	then
		:
	else
		AC_DEFINE(GCONFIG_NO_LOG,1,[Define to disable the G_LOG macro])
	fi
])

dnl GCONFIG_FN_ENABLE_STD_THREAD
dnl ----------------------------
dnl Defines GCONFIG_ENABLE_STD_THREAD based on the GCONFIG_FN_CXX_STD_THREAD
dnl result, unless "--disable-std-thread" has been disabled it. Using
dnl "--disable-std-thread" is useful for current versions of mingw32-w64.
dnl
dnl Typically used after GCONFIG_FN_CXX_STD_THREAD and AC_ARG_ENABLE(std-thread).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_STD_THREAD],
[
	if test "$enable_std_thread" = "no"
	then
		AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,0,[Define true to use std::thread])
	else
		if test "$enable_std_thread" = "yes"
		then
			if test "$gconfig_cv_cxx_std_thread" = "no"
			then
				AC_MSG_WARN([std::thread test compilation failed - see config.log])
				AC_MSG_WARN([try setting CXXFLAGS etc to enable the compiler's c++11 threading support])
				AC_MSG_ERROR([cannot enable std::thread because the feature test failed])
			fi
			AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,1,[Define true to use std::thread])
		else
			if test "$gconfig_cv_cxx_std_thread" = "yes"
			then
				AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,1,[Define true to use std::thread])
			else
				AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,0,[Define true to use std::thread])
			fi
		fi
	fi
])

dnl GCONFIG_FN_ENABLE_IPV6
dnl ----------------------
dnl Enables ipv6 if "--enable-ipv6" is used and ipv6 is available.
dnl Typically used after GCONFIG_FN_IPV6 and AC_ARG_ENABLE(ipv6).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_IPV6],
[
	if test "$enable_ipv6" = "no"
	then
		gconfig_use_ipv6="no"
	else
		if test "$gconfig_cv_ipv6" = "no"
		then
			if test "$enable_ipv6" = "yes"
			then
				AC_MSG_WARN([ignoring --enable-ipv6])
			fi
			gconfig_use_ipv6="no"
			AC_DEFINE(GCONFIG_ENABLE_IPV6,0,[Define true to use IPv6])
		else
			AC_DEFINE(GCONFIG_ENABLE_IPV6,1,[Define true to use IPv6])
			gconfig_use_ipv6="yes"
		fi
	fi
	AM_CONDITIONAL([GCONFIG_IPV6],test "$gconfig_use_ipv6" = "yes")
])

dnl GCONFIG_FN_ENABLE_MAC
dnl ---------------------
dnl Enables mac tweaks if "--enable-mac" is used. Typically used after 
dnl AC_ARG_ENABLE(mac).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_MAC],
[
	AM_CONDITIONAL([GCONFIG_MAC],test "$enable_mac" = "yes" -o "`uname`" = "Darwin")
])

dnl GCONFIG_FN_ENABLE_WINDOWS
dnl -------------------------
dnl Enables windows tweaks if "--enable-windows" is used. This is normally only 
dnl required for doing a cross-compilation from linux. Typically used after 
dnl AC_ARG_ENABLE(windows).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_WINDOWS],
[
	if test "$enable_windows" = "yes" -o "`uname -o 2>/dev/null`" = "Msys"
	then
		AC_DEFINE(GCONFIG_WINDOWS,1,[Define true for a windows build])
		AC_DEFINE(GCONFIG_MINGW,1,[Define true for a windows build using the mingw tool chain])
	else
		AC_DEFINE(GCONFIG_WINDOWS,0,[Define true for a windows build])
		AC_DEFINE(GCONFIG_MINGW,0,[Define true for a windows build using the mingw tool chain])
	fi
	AM_CONDITIONAL([GCONFIG_WINDOWS],test "$enable_windows" = "yes" -o "`uname -o 2>/dev/null`" = "Msys")
])

dnl GCONFIG_FN_ENABLE_TESTING
dnl -------------------------
dnl Disables make-check tests if "--disable-testing" is used.
dnl Eg. "make distcheck DISTCHECK_CONFIGURE_FLAGS=--disable-testing".
dnl Typically used after AC_ARG_ENABLE(testing).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_TESTING],
[
	AM_CONDITIONAL([GCONFIG_TESTING],test "$enable_testing" != "no")
])

dnl GCONFIG_FN_SSL_OPENSSL
dnl ----------------------
dnl Tests for OpenSSL. This is used in other GCONFIG_FN macros.
dnl
AC_DEFUN([GCONFIG_FN_SSL_OPENSSL],
[AC_CACHE_CHECK([for openssl],[gconfig_cv_ssl_openssl],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[SSL_CTX * p = 0 ; return 1;]])],
		gconfig_cv_ssl_openssl=yes,
		gconfig_cv_ssl_openssl=no )
])
])

dnl GCONFIG_FN_SSL_POLARSSL
dnl -----------------------
dnl Tests for PolarSSL (now renamed). This is used in other GCONFIG_FN macros.
dnl
AC_DEFUN([GCONFIG_FN_SSL_POLARSSL],
[AC_CACHE_CHECK([for polarssl],[gconfig_cv_ssl_polarssl],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <polarssl/ssl.h>]],
		[[ssl_context * p = 0 ; return 1;]])],
		gconfig_cv_ssl_polarssl=yes,
		gconfig_cv_ssl_polarssl=no )
])
])

dnl GCONFIG_FN_SSL
dnl --------------
dnl Tests for SSL libraries. Typically used after GCONFIG_FN_SSL_OPENSSL, 
dnl GCONFIG_FN_SSL_POLARSSL, AC_ARG_WITH(openssl) and AC_ARG_WITH(polarssl).
dnl
AC_DEFUN([GCONFIG_FN_SSL],
[
	if test "$with_openssl" = "yes" -a "$with_polarssl" = "yes"
	then
		AC_MSG_ERROR([cannot use --with-openssl and --with-polarssl])
	:
	elif test "$with_openssl" = "yes"
	then
		if test "$gconfig_cv_ssl_openssl" = "no"
		then
			AC_MSG_ERROR([cannot use --with-openssl: openssl is not available: check config.log])
		fi
		GCONFIG_SSL_LIBS="-lssl -lcrypto"
		gconfig_ssl="openssl"
	:
	elif test "$with_polarssl" = "yes"
	then
		if test "$gconfig_cv_ssl_polarssl" = "no"
		then
			AC_MSG_ERROR([cannot use --with-polarssl: polarssl is not available: check config.log])
		fi
		GCONFIG_SSL_LIBS="-lpolarssl"
		gconfig_ssl="polarssl"
	:
	elif test "$with_openssl" != "no" -a "$gconfig_cv_ssl_openssl" = "yes"
	then
		GCONFIG_SSL_LIBS="-lssl -lcrypto"
		gconfig_ssl="openssl"
	:
	else
		GCONFIG_SSL_LIBS=""
		gconfig_ssl="none"
	fi

	if test "$gconfig_ssl" = "openssl"
	then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL,1,[Define true to use openssl])
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL,0,[Define true to use openssl])
	fi

	AC_SUBST([GCONFIG_SSL_LIBS])
	AM_CONDITIONAL([GCONFIG_SSL_IS_OPENSSL],test "$gconfig_ssl" = "openssl")
	AM_CONDITIONAL([GCONFIG_SSL_IS_POLARSSL],test "$gconfig_ssl" = "polarssl")
	AM_CONDITIONAL([GCONFIG_SSL_IS_NONE],test "$gconfig_ssl" = "none")
	AC_MSG_NOTICE([SSL library: $gconfig_ssl])

	if test "$gconfig_ssl" = "openssl"
	then
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[SSLv3_method(); return 1;]])],
		gconfig_ssl_openssl_sslv3=yes,
		gconfig_ssl_openssl_sslv3=no )
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[TLSv1_1_method(); return 1;]])],
		gconfig_ssl_openssl_tlsv1_1=yes,
		gconfig_ssl_openssl_tlsv1_1=no )
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[TLSv1_2_method(); return 1;]])],
		gconfig_ssl_openssl_tlsv1_2=yes,
		gconfig_ssl_openssl_tlsv1_2=no )
	else
		gconfig_ssl_openssl_sslv3=no
		gconfig_ssl_openssl_tlsv1_1=no
		gconfig_ssl_openssl_tlsv1_2=no
	fi

	if test "$gconfig_ssl_openssl_sslv3" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_SSLv3,1,[Define true if openssl has SSLv3_method])
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_SSLv3,0,[Define true if openssl has SSLv3_method])
	fi
	if test "$gconfig_ssl_openssl_tlsv1_1" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_TLSv1_1,1,[Define true if openssl has TLSv1_1_method])
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_TLSv1_1,0,[Define true if openssl has TLSv1_1_method])
	fi
	if test "$gconfig_ssl_openssl_tlsv1_2" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_TLSv1_2,1,[Define true if openssl has TLSv1_2_method])
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_TLSv1_2,0,[Define true if openssl has TLSv1_2_method])
	fi
])

dnl GCONFIG_FN_ENABLE_STATIC_LINKING
dnl --------------------------------
dnl The "--enable-static-linking" makes a half-hearted attempt at static
dnl linking without using "libtool". Only applicable to gcc. Note that statically
dnl linked openssl may require a statically linked zlib so try using
dnl "GCONFIG_SSL_LIBS=-lssl -lcrypto -lz".
dnl
dnl Typically used after AC_ARG_ENABLE(static-linking) and GCONFIG_FN_WITH_ZLIB.
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_STATIC_LINKING],
[
	if test "$enable_static_linking" = "yes"
	then
		GCONFIG_STATIC_START="-Xlinker -Bstatic"
		GCONFIG_STATIC_END="${GCONFIG_ZLIB_LIBS} -Xlinker -Bdynamic -ldl"
	else
		GCONFIG_STATIC_START=""
		GCONFIG_STATIC_END=""
	fi
	AC_SUBST([GCONFIG_STATIC_START])
	AC_SUBST([GCONFIG_STATIC_END])
])

dnl GCONFIG_FN_ENABLE_INSTALL_HOOK
dnl ------------------------------
dnl The "--disable-install-hook" option can be used to disable tricksy install 
dnl steps when building a package for distribution.
dnl
dnl Typically used after AC_ARG_ENABLE(install-hook).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_INSTALL_HOOK],
[
	AM_CONDITIONAL([GCONFIG_INSTALL_HOOK],test "$enable_install_hook" != "no")
])

dnl GCONFIG_FN_WITH_DOXYGEN
dnl -----------------------
dnl Tests for doxygen. Typically used after AC_CHECK_PROG(doxygen) and 
dnl AC_ARG_WITH(doxygen).
dnl
AC_DEFUN([GCONFIG_FN_WITH_DOXYGEN],
[
	if test "$with_doxygen" != ""
	then
		if test "$with_doxygen" = "yes" -a "$GCONFIG_HAVE_DOXYGEN" != "yes"
		then
			AC_MSG_WARN([forcing use of doxygen even though not found])
		fi
		HAVE_DOXYGEN="$with_doxygen"
	fi
	AC_SUBST([GCONFIG_HAVE_DOXYGEN])
])

dnl GCONFIG_FN_WITH_MAN2HTML
dnl ------------------------
dnl Tests for man2html. Typically used after AC_CHECK_PROG(man2html) and 
dnl AC_ARG_WITH(man2html).
dnl
AC_DEFUN([GCONFIG_FN_WITH_MAN2HTML],
[
	if test "$with_man2html" != ""
	then
		if test "$with_man2html" = "yes" -a "$GCONFIG_HAVE_MAN2HTML" != "yes"
		then
			AC_MSG_WARN([forcing use of man2html even though not found])
		fi
		GCONFIG_HAVE_MAN2HTML="$with_man2html"
	fi
	AC_SUBST([GCONFIG_HAVE_MAN2HTML])
])

dnl GCONFIG_FN_BOOST
dnl ----------------
dnl Tests for boost.
dnl
AC_DEFUN([GCONFIG_FN_BOOST],
[AC_CACHE_CHECK([for boost],[gconfig_cv_boost],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <boost/exception/all.hpp>]],
		[[int i = 1 ;]])],
		gconfig_cv_boost=yes ,
		gconfig_cv_boost=no )
])
	if test "$gconfig_cv_boost" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_BOOST,1,[Define true to enable use of boost])
	else
		AC_DEFINE(GCONFIG_HAVE_BOOST,0,[Define true to enable use of boost])
	fi
])

dnl GCONFIG_FN_ICONV_LIBC
dnl ---------------------
dnl Tests for iconv in libc.
dnl
dnl Note that using AC_SEARCH_LIBS is no good because the test does not
dnl include the header, and including the header can modify the library
dnl requirements.
dnl
dnl See also "/usr/share/aclocal/iconv.m4" for a more complete solution.
dnl
AC_DEFUN([GCONFIG_FN_ICONV_LIBC],
[AC_CACHE_CHECK([for iconv in libc],[gconfig_cv_iconv_libc],
[
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdlib.h>]
			[#include <iconv.h>]
		],
		[
			[iconv_t i = iconv_open("","") ;]
			[iconv(i,NULL,NULL,NULL,NULL) ;]
			[iconv_close(i) ;]
		])],
		gconfig_cv_iconv_libc=yes ,
		gconfig_cv_iconv_libc=no )
])
])

dnl GCONFIG_FN_ICONV_LIBICONV
dnl -------------------------
dnl Tests for iconv in libiconv. Adds to LIBS if required.
dnl
AC_DEFUN([GCONFIG_FN_ICONV_LIBICONV],
[AC_CACHE_CHECK([for iconv in libiconv],[gconfig_cv_iconv_libiconv],
[
	gconfig_save_LIBS="$LIBS"
	LIBS="$LIBS -liconv"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdlib.h>]
			[#include <iconv.h>]
		],
		[
			[iconv_t i = iconv_open("","") ;]
			[iconv(i,NULL,NULL,NULL,NULL) ;]
			[iconv_close(i) ;]
		])],
		gconfig_cv_iconv_libiconv=yes ,
		gconfig_cv_iconv_libiconv=no )
	])
	if test "$gconfig_cv_iconv_libiconv" = "no" ; then
		LIBS="$gconfig_save_LIBS"
	fi
])
])

dnl GCONFIG_FN_ICONV
dnl ----------------
dnl Tests for iconv.
dnl
AC_DEFUN([GCONFIG_FN_ICONV],
[
	AC_REQUIRE([GCONFIG_FN_ICONV_LIBC])
	AC_REQUIRE([GCONFIG_FN_ICONV_LIBICONV])
	if test "$gconfig_cv_iconv_libc" = "yes" -o "$gconfig_cv_iconv_libiconv" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_ICONV,1,[Define true to enable use of iconv])
	else
		AC_DEFINE(GCONFIG_HAVE_ICONV,0,[Define true to enable use of iconv])
	fi
	AM_CONDITIONAL([GCONFIG_ICONV],[test "$gconfig_cv_iconv_libc" = "yes" -o "$gconfig_cv_iconv_libiconv" = "yes"])
])

dnl GCONFIG_FN_ALSA
dnl ---------------
dnl Tests for alsa.
dnl
AC_DEFUN([GCONFIG_FN_ALSA],
[AC_CACHE_CHECK([for alsa],[gconfig_cv_alsa],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <alsa/asoundlib.h>]],
		[
			[snd_pcm_t * x ;]
			[int rc = snd_pcm_open( &x , "" , SND_PCM_STREAM_PLAYBACK , 0 ) ;]
		])],
		gconfig_cv_alsa=yes ,
		gconfig_cv_alsa=no )
])
	if test "$gconfig_cv_alsa" = "yes" ; then
		LIBS="$LIBS -lasound"
		AC_DEFINE(GCONFIG_HAVE_ALSA,1,[Define true to enable use of alsa])
	else
		AC_DEFINE(GCONFIG_HAVE_ALSA,0,[Define true to enable use of alsa])
	fi
])

dnl GCONFIG_FN_PAM_IN_WHATEVER
dnl --------------------------
dnl Tests a pam header file location. Sets local gconfig_cv_pam_in_whatever 
dnl depeding on whether the header is included from the whatever directory.
dnl
AC_DEFUN([GCONFIG_FN_PAM_IN_SECURITY],
[AC_CACHE_CHECK([for security/pam_appl.h],[gconfig_cv_pam_in_security],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <security/pam_appl.h>]],
		[[int rc = pam_start("","",(const struct pam_conv*)0,(pam_handle_t**)0)]])] ,
		[gconfig_cv_pam_in_security=yes],
		[gconfig_cv_pam_in_security=no])
])
])
AC_DEFUN([GCONFIG_FN_PAM_IN_PAM],
[AC_CACHE_CHECK([for pam/pam_appl.h],[gconfig_cv_pam_in_pam],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <pam/pam_appl.h>]],
		[[int rc = pam_start("","",(const struct pam_conv*)0,(pam_handle_t**)0)]])] ,
		[gconfig_cv_pam_in_pam=yes],
		[gconfig_cv_pam_in_pam=no])
])
])
AC_DEFUN([GCONFIG_FN_PAM_IN_INCLUDE],
[AC_CACHE_CHECK([for include/pam_appl.h],[gconfig_cv_pam_in_include],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <pam_appl.h>]],
		[[int rc = pam_start("","",(const struct pam_conv*)0,(pam_handle_t**)0)]])] ,
		[gconfig_cv_pam_in_include=yes],
		[gconfig_cv_pam_in_include=no])
])
])

dnl GCONFIG_FN_PAM
dnl --------------
dnl Tests for pam headers.
dnl
AC_DEFUN([GCONFIG_FN_PAM],
[
	AC_REQUIRE([GCONFIG_FN_PAM_IN_SECURITY])
	AC_REQUIRE([GCONFIG_FN_PAM_IN_PAM])
	AC_REQUIRE([GCONFIG_FN_PAM_IN_INCLUDE])

	if test "$gconfig_cv_pam_in_security" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_SECURITY,1,[Define true to include pam_appl.h from security])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_SECURITY,0,[Define true to include pam_appl.h from security])
	fi

	if test "$gconfig_cv_pam_in_pam" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_PAM,1,[Define true to include pam_appl.h from pam])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_PAM,0,[Define true to include pam_appl.h from pam])
	fi

	if test "$gconfig_cv_pam_in_include" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_INCLUDE,1,[Define true to include pam_appl.h as-is])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_INCLUDE,0,[Define true to include pam_appl.h as-is])
	fi
])

dnl GCONFIG_FN_WITH_PAM
dnl -------------------
dnl Tests for pam. Typically used after AC_ARG_WITH(pam).
dnl
AC_DEFUN([GCONFIG_FN_WITH_PAM],
[
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_PAM])
	AC_REQUIRE([GCONFIG_FN_PAM])

	gconfig_pam_compiles="no"
	if test "$gconfig_cv_pam_in_security" = "yes" -o "$gconfig_cv_pam_in_pam" = "yes" -o "$gconfig_cv_pam_in_include" = "yes"
	then
		gconfig_pam_compiles="yes"
	fi

	if test "$with_pam" = "yes" -a "$gconfig_pam_compiles" = "no"
	then
		AC_MSG_WARN([forcing use of pam even though it does not seem to compile])
	fi

	gconfig_use_pam="$with_pam"
	if test "$with_pam" = "" -a "$gconfig_pam_compiles" = "yes"
	then
		gconfig_use_pam="yes"
	fi

	if test "$gconfig_use_pam" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_PAM,1,[Define true to use pam])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM,0,[Define true to use pam])
	fi
	AM_CONDITIONAL([GCONFIG_PAM],[test "$gconfig_use_pam" = "yes"])
])

dnl GCONFIG_FN_JPEG
dnl ---------------
dnl Tests for libjpeg.
dnl
AC_DEFUN([GCONFIG_FN_JPEG],
[AC_CACHE_CHECK([for libjpeg],[gconfig_cv_libjpeg],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <stdio.h>]
			[#include <jpeglib.h>]
		],
		[[jpeg_decompress_struct m ; jpeg_create_decompress( &m )]])] ,
		[gconfig_cv_libjpeg=yes],
		[gconfig_cv_libjpeg=no])
])
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_JPEG])
	if test "$gconfig_cv_libjpeg" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LIBJPEG,1,[Define true to enable use of libjpeg])
	else
		AC_DEFINE(GCONFIG_HAVE_LIBJPEG,0,[Define true to enable use of libjpeg])
		gconfig_warnings="$gconfig_warnings libjpeg_jpeg_images"
	fi
	AM_CONDITIONAL([GCONFIG_LIBJPEG],[test "$gconfig_cv_libjpeg" = "yes"])
])

dnl GCONFIG_FN_PNG
dnl --------------
dnl Tests for libpng.
dnl
AC_DEFUN([GCONFIG_FN_PNG],
[AC_CACHE_CHECK([for libpng],[gconfig_cv_libpng],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <png.h>]],
		[[png_struct * p ; png_create_info_struct( p )]])] ,
		[gconfig_cv_libpng=yes],
		[gconfig_cv_libpng=no])
])
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_PNG])
	if test "$gconfig_cv_libpng" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LIBPNG,1,[Define true to enable use of libpng])
	else
		AC_DEFINE(GCONFIG_HAVE_LIBPNG,0,[Define true to enable use of libpng])
		gconfig_warnings="$gconfig_warnings libpng_png_images"
	fi
	AM_CONDITIONAL([GCONFIG_LIBPNG],[test "$gconfig_cv_libpng" = "yes"])
])

dnl GCONFIG_FN_LIBEXIV
dnl ------------------
dnl Tests for libexiv. Adds to LIBS as required.
dnl
AC_DEFUN([GCONFIG_FN_LIBEXIV],
[AC_CACHE_CHECK([for libexiv],[gconfig_cv_libexiv],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <exiv2/exiv2.hpp>]],
		[[long n = Exiv2::TypeInfo::typeSize(Exiv2::date)]])] ,
		[gconfig_cv_libexiv=yes],
		[gconfig_cv_libexiv=no])
])
	if test "$gconfig_cv_libexiv" = "yes" ; then
		LIBS="$LIBS -lexiv2"
		AC_DEFINE(GCONFIG_HAVE_LIBEXIV,1,[Define true to enable use of libexiv2])
	else
		AC_DEFINE(GCONFIG_HAVE_LIBEXIV,0,[Define true to enable use of libexiv2])
	fi
	AM_CONDITIONAL([GCONFIG_LIBEXIV],[test "$gconfig_cv_libexiv" = "yes"])
])

dnl GCONFIG_FN_LIBAV
dnl ----------------
dnl Tests for libav.
dnl
AC_DEFUN([GCONFIG_FN_LIBAV],
[AC_CACHE_CHECK([for libav],[gconfig_cv_libav],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <libavcodec/avcodec.h>]],
		[[avcodec_find_decoder(AV_CODEC_ID_H264)]])] ,
		[gconfig_cv_libav=yes],
		[gconfig_cv_libav=no])
])
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_LIBAV])
	if test "$gconfig_cv_libav" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LIBAV,1,[Define true to enable use of libav])
	else
		AC_DEFINE(GCONFIG_HAVE_LIBAV,0,[Define true to enable use of libav])
		gconfig_warnings="$gconfig_warnings libav_h.264_video"
	fi
	AM_CONDITIONAL([GCONFIG_LIBAV],[test "$gconfig_cv_libav" = "yes"])
])

dnl GCONFIG_FN_LIBAV_NEW_FRAME_ALLOC_FN
dnl -----------------------------------
dnl Checks for new libav function av_frame_alloc(), replacing avcodec_alloc_frame().
dnl
AC_DEFUN([GCONFIG_FN_LIBAV_NEW_FRAME_ALLOC_FN],
[AC_CACHE_CHECK([for libav new frame alloc function],[gconfig_cv_libav_new_frame_alloc_fn],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <libavcodec/avcodec.h>]],
		[[av_frame_alloc()]])] ,
		[gconfig_cv_libav_new_frame_alloc_fn=yes],
		[gconfig_cv_libav_new_frame_alloc_fn=no])
])
	if test "$gconfig_cv_libav_new_frame_alloc_fn" = "yes" ; then
		AC_DEFINE(GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN,1,[Define true to enable libav new frame alloc function])
	else
		AC_DEFINE(GCONFIG_LIBAV_NEW_FRAME_ALLOC_FN,0,[Define true to enable libav new frame alloc function])
	fi
])

dnl GCONFIG_FN_LIBAV_NEW_DECODE_FN
dnl ------------------------------
dnl Checks for new libav functions avcodec_send_packet()/avcodec_receive_frame(), replacing avcodec_decode_video2().
dnl
AC_DEFUN([GCONFIG_FN_LIBAV_NEW_DECODE_FN],
[AC_CACHE_CHECK([for libav new decode function],[gconfig_cv_libav_new_decode_fn],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <libavcodec/avcodec.h>]],
		[
			[avcodec_send_packet( (AVCodecContext*)0 , (AVPacket*)0 ) ;]
			[avcodec_receive_frame( (AVCodecContext*)0 , (AVFrame*)0 )]
		])] ,
		[gconfig_cv_libav_new_decode_fn=yes],
		[gconfig_cv_libav_new_decode_fn=no])
])
	if test "$gconfig_cv_libav_new_decode_fn" = "yes" ; then
		AC_DEFINE(GCONFIG_LIBAV_NEW_DECODE_FN,1,[Define true to enable libav new decode functions])
	else
		AC_DEFINE(GCONFIG_LIBAV_NEW_DECODE_FN,0,[Define true to enable libav new decode functions])
	fi
])

dnl GCONFIG_FN_LIBAV_NEW_DESCRIPTOR
dnl -------------------------------
dnl Checks for new pixdel descriptor fields.
dnl
AC_DEFUN([GCONFIG_FN_LIBAV_NEW_DESCRIPTOR],
[AC_CACHE_CHECK([for libav new pixel descriptor fields],[gconfig_cv_libav_new_descriptor],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <libavcodec/avcodec.h>]
			[#include <libavutil/pixdesc.h>]
		],
		[
			[struct AVComponentDescriptor x;]
			[x.step = 0]
		])] ,
		[gconfig_cv_libav_new_descriptor=yes],
		[gconfig_cv_libav_new_descriptor=no])
])
	if test "$gconfig_cv_libav_new_descriptor" = "yes" ; then
		AC_DEFINE(GCONFIG_LIBAV_NEW_DESCRIPTOR,1,[Define true to enable libav new pixel descriptor])
	else
		AC_DEFINE(GCONFIG_LIBAV_NEW_DESCRIPTOR,0,[Define true to enable libav new pixel descriptor])
	fi
])

dnl GCONFIG_FN_CURSES
dnl -----------------
dnl Tests for curses.
dnl
AC_DEFUN([GCONFIG_FN_CURSES],
[AC_CACHE_CHECK([for curses],[gconfig_cv_curses],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <curses.h>]],
		[[initscr()]])] ,
		[gconfig_cv_curses=yes],
		[gconfig_cv_curses=no])
])
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_CURSES])
	if test "$gconfig_cv_curses" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CURSES,1,[Define true to enable use of curses])
	else
		AC_DEFINE(GCONFIG_HAVE_CURSES,0,[Define true to enable use of curses])
		gconfig_warnings="$gconfig_warnings curses_coloured_text"
	fi
	AM_CONDITIONAL([GCONFIG_CURSES],[test "$gconfig_cv_curses" = "yes"])
])

dnl GCONFIG_FN_V4L
dnl --------------
dnl Tests for v4l.
dnl
AC_DEFUN([GCONFIG_FN_V4L],
[AC_CACHE_CHECK([for v4l],[gconfig_cv_v4l],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <linux/videodev2.h>]],
		[[struct v4l2_format fmt ; fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE]])] ,
		[gconfig_cv_v4l=yes],
		[gconfig_cv_v4l=no])
])
	if test "$gconfig_cv_v4l" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_V4L,1,[Define true to enable use of video-for-linux])
	else
		AC_DEFINE(GCONFIG_HAVE_V4L,0,[Define true to enable use of video-for-linux])
		gconfig_warnings="$gconfig_warnings v4l_webcam_capture"
	fi
	AM_CONDITIONAL([GCONFIG_V4L],[test "$gconfig_cv_v4l" = "yes"])
])

dnl GCONFIG_FN_X11
dnl --------------
dnl Tests for x11 xlib.
dnl
AC_DEFUN([GCONFIG_FN_X11],
[AC_CACHE_CHECK([for x11],[gconfig_cv_x11],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <X11/Xlib.h>]],
		[[Display * d = XOpenDisplay(0)]])] ,
		[gconfig_cv_x11=yes],
		[gconfig_cv_x11=no])
])
	AC_REQUIRE([GCONFIG_FN_SEARCHLIBS_X11])
	if test "$gconfig_cv_x11" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_X11,1,[Define true to enable use of x11 xlib])
	else
		AC_DEFINE(GCONFIG_HAVE_X11,0,[Define true to enable use of x11 xlib])
		gconfig_warnings="$gconfig_warnings xlib_x_windows"
	fi
	AM_CONDITIONAL([GCONFIG_X11],[test "$gconfig_cv_x11" = "yes"])
])

dnl GCONFIG_FN_WITH_LIBV4L
dnl ----------------------
dnl Enables libv4l (ie. the userland shim library for v4l) if using 
dnl "--with-libv4l". Used after AC_ARG_WITH(libv4l). Note that libv4l
dnl is not the same as v4l in the kernel, so HAVE_V4L and HAVE_LIBV4L
dnl serve different purposes.
dnl
AC_DEFUN([GCONFIG_FN_WITH_LIBV4L],
[
	if test "$with_libv4l" = "yes" ; then
		LIBS="-lv4l2 $LIBS"
		AC_DEFINE(GCONFIG_HAVE_LIBV4L,1,[Define true to enable use of libv4l2])
	else
		AC_DEFINE(GCONFIG_HAVE_LIBV4L,0,[Define true to enable use of libv4l2])
	fi
])

dnl GCONFIG_FN_WARNINGS
dnl -------------------
dnl Displays a summary warning.
dnl
AC_DEFUN([GCONFIG_FN_WARNINGS],
[
	for gconfig_w in $gconfig_warnings ""
	do
		if test "$gconfig_w" != ""
		then
			echo "$gconfig_w" | sed 's/_/ /g' | while read gconfig_what gconfig_stuff
			do
				AC_MSG_WARN([missing $gconfig_what - no support for $gconfig_stuff])
			done
		fi
	done
])

dnl GCONFIG_FN_SET_DIRECTORIES
dnl --------------------------
dnl Sets makefile variables for install directory paths, usually incorporating
dnl the package name. These should be used in conjunction with DESTDIR when
dnl writing install rules in makefiles. Standard extensions of these variables, 
dnl such as e_sysconf_DATA, are also magically meaningful.
dnl
AC_DEFUN([GCONFIG_FN_SET_DIRECTORIES],
[
	if test "$e_libexecdir" = ""
	then
		e_libexecdir="$libexecdir/$PACKAGE"
	fi
	if test "$e_examplesdir" = ""
	then
		e_examplesdir="$libexecdir/$PACKAGE/examples"
	fi
	if test "$e_sysconfdir" = ""
	then
		e_sysconfdir="$sysconfdir"
	fi
	if test "$e_docdir" = ""
	then
		e_docdir="$docdir"
		if test "$e_docdir" = ""
		then
			e_docdir="$datadir/$PACKAGE/doc"
		fi
	fi
	if test "$e_spooldir" = ""
	then
		e_spooldir="$localstatedir/spool/$PACKAGE"
	fi
	if test "$e_pamdir" = ""
	then
		e_pamdir="$sysconfdir/pam.d"
	fi
	if test "$e_initdir" = ""
	then
		e_initdir="$libexecdir/$PACKAGE/init"
	fi
	if test "$e_icondir" = ""
	then
		e_icondir="$datadir/$PACKAGE"
	fi

	AC_SUBST([e_docdir])
	AC_SUBST([e_initdir])
	AC_SUBST([e_icondir])
	AC_SUBST([e_spooldir])
	AC_SUBST([e_examplesdir])
	AC_SUBST([e_libexecdir])
	AC_SUBST([e_pamdir])
	AC_SUBST([e_sysconfdir])
])

dnl GCONFIG_FN_SET_DIRECTORIES_X
dnl ----------------------------
dnl Sets makefile variables for install directory paths, typically incorporating
dnl the package name. These should be used in conjunction with DESTDIR when
dnl writing install rules in makefiles. Standard extensions of these variables, 
dnl such as x_data_DATA, are also magically meaningful.
dnl
AC_DEFUN([GCONFIG_FN_SET_DIRECTORIES_X],
[
	if test "$x_libexecdir" = ""
	then
		if test "`eval basename \"$libexecdir\"`" = "$PACKAGE"
		then
			x_libexecdir="$libexecdir"
		else
			x_libexecdir="$libexecdir/$PACKAGE"
		fi
	fi
	if test "$x_docdir" = ""
	then
		if test "`eval basename \"$docdir\"`" = "$PACKAGE"
		then
			x_docdir="$docdir"
		else
			x_docdir="$docdir/$PACKAGE"
		fi
	fi
	if test "$x_datadir" = ""
	then
		if test "`eval basename \"$datadir\"`" = "$PACKAGE"
		then
			x_datadir="$datadir"
		else
			x_datadir="$datadir/$PACKAGE"
		fi
	fi

	AC_SUBST([x_libexecdir])
	AC_SUBST([x_docdir])
	AC_SUBST([x_datadir])
])

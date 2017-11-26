#!/bin/sh
#
# configure.sh
#
# A thin wrapper for the autotools configure script that fiddles with
# the toolchain environment variables and the 'make install' directory
# settings according to the host o/s.
#
thisdir="`cd \`dirname $0\` && pwd`"
dirs="none"

if expr "$*" : '.*enable.debug' >/dev/null
then
	if test "$CFLAGS$CXXFLAGS" = ""
	then
		export CFLAGS="-O0 -g"
		export CXXFLAGS="-O0 -g"
	fi
fi

if test "`uname`" = "NetBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib"
	dirs="--bindir=/usr/bin --libexecdir=/usr/lib --docdir=/usr/share/doc --mandir=/usr/share/man"
fi

if test "`uname`" = "FreeBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/local/include -I/usr/local/include/libav"
	export LDFLAGS="$LDFLAGS -L/usr/local/lib -L/usr/local/lib/libav"
	dirs="--mandir=/usr/local/man"
fi

if test "`uname`" = "OpenBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
	dirs="--mandir=/usr/local/man"
fi

if test "`uname`" = "Darwin"
then
	export CPPFLAGS="$CPPFLAGS -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/opt/local/lib -L/opt/X11/lib"
	dirs=""
fi

if test "`uname`" = "Linux"
then
	dirs="--bindir=/usr/bin --libexecdir=/usr/lib --datarootdir=/usr/share --sysconfdir=/etc"
fi

if test "$dirs" = "none"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include -I/usr/X11R6/include -I/usr/local/include -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib -L/usr/X11R6/lib -L/usr/local/lib -L/opt/local/lib -L/opt/X11/lib"
	dirs=""
fi

$thisdir/configure $dirs "$@"


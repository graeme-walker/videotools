#!/bin/sh
#
# make_uninstall.sh
#
# Called by "make uninstall" to remove boot-time hooks.
#
# usage: make_uninstall.sh <active> <srcdir> <DESTDIR> <x_libexecdir> <x_datadir> <sysconfdir> <bindir>
#

cfg_active="$1"
cfg_srcdir="$2"
cfg_destdir="$3"
cfg_xlibdir="$4"
cfg_xdatadir="$5"
cfg_etcdir="$6"
cfg_bindir="$7"

do_rm()
{
	if test -e "$1" then ; rm "$1" ; fi
}

if test "$cfg_active" -eq 1 -a "$cfg_destdir" = ""
then
	if test "`uname`" = "Linux" -a -f /etc/init.d/videotools
	then
		if test -x /usr/lib/lsb/remove_initd
		then
			/usr/lib/lsb/remove_initd videotools
		else
			if text -x /usr/sbin/update-rc.d
			then
				/usr/sbin/update-rc.d -f videotools remove
			fi
		fi
	fi
	do_rm /etc/init.d/videotools
	do_rm "$cfg_etcdir/rc.d/videotools"
fi

true

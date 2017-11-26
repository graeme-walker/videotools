#!/bin/sh
#
# vt-startstop.sh
#
# Starts and stops the programs listed as "run ..." lines in /etc/videotools.
#
# Installed on linux as "/etc/init.d/videotools", with symlinks like 
# "/etc/rc2.d/S99videotools".
#
# Directories etc. can be overridden by an optional file "/etc/default/videotools"
# or "/etc/rc.conf.d/videotools".
#
# The "run" directives can have substitution variables for RUNDIR (eg. local
# domain control sockets) and SHAREDIR (eg. recordings and mask files).
#
# See also: install_initd, remove_initd
#
### BEGIN INIT INFO
# Provides: videotools
# Required-Start: $local_fs $network $syslog
# Required-Stop: $local_fs $network $syslog
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: Start videotools daemons
### END INIT INFO
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin
NAME=videotools
DESC=$NAME
CONFIG=/etc/$NAME
IDLEUSER=daemon
GROUP=root
RUNDIR=/var/run/$NAME
PIDDIR=$RUNDIR
SHAREDIR=/usr/share/$NAME
BINDIR=/usr/bin
UMASK=0007
test -f /etc/default/$NAME && . /etc/default/$NAME
test -f /etc/rc.conf.d/$NAME && . /etc/rc.conf.d/$NAME
test -f /etc/default/rcS && . /etc/default/rcS
log_success_msg() {
	echo "$@"
}
log_failure_msg() {
	echo "$@"
}
log_warning_msg() {
	echo "$@"
}
start_daemon() {
	if test "`cat \"$2\" 2>/dev/null`" -gt 0 2>/dev/null && kill -0 "`cat \"$2\"`"
	then
		: # running
	else
		shift ; shift ; shift
		"$@"
	fi
}
killproc() {
	shift
	kill `cat "$1" 2>/dev/null` 2>/dev/null
}
pidofproc() {
	shift
	kill -0 `cat "$1" 2>/dev/null` 2>/dev/null
}
log_daemon_msg() {
	log_success_msg "$@"
}
log_progress_msg() {
	:;
}
log_end_msg() {
	if test "$1" -eq 0 ; then log_success_msg "...ok" ; else log_failure_msg "...failed!" ; fi
}
test -f /lib/lsb/init-functions && . /lib/lsb/init-functions

case $1 in

	restart|force-reload)
		$0 stop
		$0 start
	;;

	*start)
		log_daemon_msg "Starting $DESC services"

		if test ! -d "$RUNDIR"
		then
			if mkdir "$RUNDIR" 2>/dev/null 
			then
				test -z "$GROUP" || chgrp "$GROUP" "$RUNDIR" 2>/dev/null
				chmod 770 "$RUNDIR"
			fi
		fi

		if test -n "$IDLEUSER" ; then
			opt_user="--user $IDLEUSER"
		fi

		id=0
		final_status=0
		while read run program args
		do
			if expr "$run" : '^run' >/dev/null
			then
				id="`expr $id + 1`"
				args="`echo \"$args\" | sed 's:\$RUNDIR:'\"$RUNDIR\"':g'`"
				args="`echo \"$args\" | sed 's:\$SHAREDIR:'\"$SHAREDIR\"':g'`"

				pidfile="$PIDDIR/$program-$id.pid"
				log_progress_msg "$program-$id"
				umask="`umask`"
				umask "$UMASK"
				start_daemon -p "$pidfile" -- "$BINDIR/$program" --daemon --syslog $opt_user --pid-file "$pidfile" $args
				status=$?
				umask "$umask" 2>/dev/null
				if test "$status" -ne 0 ; then final_status=$status ; fi
			fi
		done < "$CONFIG"

		if test "$id" = ""
		then
			logger -- "$0: nothing configured: see $CONFIG" 2>/dev/null
		fi

		log_end_msg $final_status
	;;

	stop)
		log_daemon_msg "Stopping $DESC services"
		final_status=0
		id=0
		while read run program args
		do
			if expr "$run" : '^run' >/dev/null
			then
				id="`expr $id + 1`"
				pidfile="$PIDDIR/$program-$id.pid"
				log_progress_msg "$program-$id"
				killproc -p "$pidfile" "$BINDIR/$program"
				status=$?
				if test "$status" -ne 0 ; then final_status=$status ; fi
			fi
		done < "$CONFIG"
		log_end_msg $final_status
	;;

	status)
		list=""
		id=0
		while read run program args
		do
			if expr "$run" : '^run' >/dev/null
			then
				id="`expr $id + 1`"
				pidfile="$PIDDIR/$program-$id.pid"
				pidofproc -p "$pidfile" "$BINDIR/$program" >/dev/null
				status=$?
				if test "$status" -eq 0 ; then
					list="$list $program-$id"
				fi
			fi
		done < "$CONFIG"
		if test "$list" != ""
		then
			log_success_msg "$NAME is running$list"
			true
		else
			log_failure_msg "$NAME is not running"
			false
		fi
	;;

	abort)
		for name in alarm webcamserver webcamplayer fileplayer httpclient httpserver maskeditor recorder rtpserver rtspclient socket viewer watcher
		do
			killall $2 "vt-$name" 2>/dev/null
		done
	;;

	setup)

		test -d "$RUNDIR" || mkdir -p "$RUNDIR" 2>/dev/null 
		chgrp "$GROUP" "$RUNDIR"
		chmod 770 "$RUNDIR"

		test -d "$SHAREDIR" || mkdir -p "$SHAREDIR" 2>/dev/null 
		chgrp "$GROUP" "$SHAREDIR"
		chmod 770 "$SHAREDIR"

		chmod 550 $BINDIR/vt-*
		chgrp "$GROUP" $BINDIR/vt-*
		chmod g+s $BINDIR/vt-*

	;;

	*)
		echo usage: `basename $0` '{start|stop|restart|force-reload|status}' >&2
		exit 2
	;;
esac
exit 0

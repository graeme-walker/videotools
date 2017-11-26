#!/bin/sh
#
# videotools
#
# Start/stop wrapper for videotools on FreeBSD, installed as 
# "/etc/rc.d/videotools" or "/usr/local/etc/rc.d/videotools".
#
# Requires the following line to be added to "/etc/rc.conf":
#
#    videotools_enable="YES"
#
# Delegates to the linux start/stop script, which reads default directories 
# etc. from "/etc/rc.conf.d/videotools" and its list of what to run from 
# "/etc/videotools".
#
# See also man rc(8).
#
# PROVIDE: videotools
# REQUIRE: DAEMON
#

. /etc/rc.subr
videotools_enable=${videotools_enable:-"NO"}
name=videotools
rcvar=videotools_enable
command=/usr/local/bin/vt-startstop.sh
command_args="$1"
load_rc_config $name
run_rc_command "$1"


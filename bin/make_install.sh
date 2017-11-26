#!/bin/sh
#
# make_install.sh
#
# Called by "make install" to hook into the boot system, etc. Disable
# with "configure" option "--disable-install-hook".
#
# This might also be done by the packaging system, but doing it in
# "make install" is also worthwhile.
#
# usage: make_install.sh <srcdir> <DESTDIR> <x_libexecdir> <x_datadir> <sysconfdir> <bindir>
#

cfg_srcdir="$1" # eg. "../bin"
cfg_destdir="$2" # eg. ""
cfg_xlibdir="$3" # eg. "/usr/lib/foo"
cfg_xdatadir="$4" # eg. "/usr/share/foo"
cfg_etcdir="$5" # eg. "/etc"
cfg_bindir="$6" # eg. "/usr/bin"

PATH="/bin:/sbin:/usr/bin:/usr/sbin:$PATH"
logger -- "$0" "`echo $* | tr ' ' ','`"

do_mkdir()
{
	dir="$1"
	if test ! -d "$dir"
	then
		mkdir -p "$dir"
		chmod 770 "$dir" 2>/dev/null
		chgrp daemon "$dir" 2>/dev/null
	fi
	test -d "$dir"
}

do_var()
{
	# use the '/etc/default' mechanism to force the 'make install' directories
	varfile="$1"
	key="$2"
	value="$3"
	if test -w "$varfile"
	then
		# (dont overwite existing entries)
		grep -q "^$key=" "$varfile" || echo "$key=$value" >> "$varfile"
	fi
}

do_symlink()
{
	symlink_dir="$1" 
	symlink_src="$2" 
	symlink_dst="$3" 
	if test -e "$symlink_dir/$symlink_dst"
	then
		:
	else
		cd "$symlink_dir" && ln -s "$symlink_src" "$symlink_dst"
	fi
}

do_setup()
{
	varfile="$1"
	rundir="$2"
	sharedir="$3"
	etcdir="$4"
	bindir="$5"
	do_mkdir "$rundir" 
	do_mkdir "$sharedir" 
	touch "$varfile" 2>/dev/null
	do_var "$varfile" RUNDIR "$rundir"
	do_var "$varfile" SHAREDIR "$sharedir"
	do_var "$varfile" CONFIG "$etcdir/videotools"
	do_var "$varfile" BINDIR "$bindir"
	do_symlink "$sharedir" example.html index.html
}

do_startstop_linux()
{
	srcdir="$1"
	ss_src="$srcdir/vt-startstop.sh"
	ss_dst="/etc/init.d/videotools"
	cp "$ss_src" "$ss_dst" && chmod 555 "$ss_dst"
	if test -x "$ss_dst" 
	then 
		# (the lsb script is defective on ubuntu 14.04 because benign python 
		# exceptions trigger the apport gui)
		if test -x /usr/lib/lsb/install_initd -a "`cat /proc/sys/kernel/core_pattern | grep apport`" = ""
		then
			/usr/lib/lsb/install_initd videotools 
		:
		elif test -x /lib/lsb/install_initd
		then
			/lib/lsb/install_initd videotools 
		:
		elif test -x /usr/sbin/update-rc.d
		then
			/usr/sbin/update-rc.d videotools defaults
		:
		elif test -x /bin/systemctl -a `id -u` = 0
		then
			systemctl enable videotools
		fi
	fi
}

do_startstop_bsd()
{
	srcdir="$1"
	xlibdir="$2"
	etcdir="$3"
	ss_src="$srcdir/vt-startstop-bsd.sh"
	ss_imp="$xlibdir/vt-startstop.sh"
	ss_dst="$etcdir/rc.d/videotools"
	cp "$ss_src" "$ss_dst" && chmod 755 "$ss_dst"
	sed -i "" 's:^command=.*:command='"$ss_imp"':g' "$ss_dst"
	chmod 555 "$ss_dst"
}

if test "$cfg_destdir" = ""
then
	if test "`uname`" = "Linux"
	then
		do_startstop_linux "$cfg_srcdir"
		do_setup "/etc/default/videotools" "/var/run/videotools" "$cfg_xdatadir" "$cfg_etcdir" "$cfg_bindir"
	else
		do_startstop_bsd "$cfg_srcdir" "$cfg_xlibdir" "$cfg_etcdir"
		do_setup "/etc/rc.conf.d/videotools" "/var/run/videotools" "$cfg_xdatadir" "$cfg_etcdir" "$cfg_bindir"
	fi
fi

true

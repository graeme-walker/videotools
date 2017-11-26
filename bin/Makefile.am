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
# bin/Makefile.am

EXTRA_DIST = \
	make_install.sh \
	make_uninstall.sh \
	vt-startstop.sh \
	vt-startstop-bsd.sh

noinst_SCRIPTS = \
	make_install.sh \
	make_uninstall.sh

x_libexec_SCRIPTS = \
	vt-startstop.sh \
	vt-startstop-bsd.sh

if GCONFIG_INSTALL_HOOK

install-data-local:
	-sh "$(top_srcdir)/bin/make_install.sh" "$(top_srcdir)/bin" "$(DESTDIR)" "$(x_libexecdir)" "$(x_datadir)" "$(sysconfdir)" "$(bindir)"

uninstall-local:
	-sh "$(top_srcdir)/bin/make_uninstall.sh" "$(top_srcdir)/bin" "$(DESTDIR)" "$(x_libexecdir)" "$(x_datadir)" "$(sysconfdir)" "$(bindir)"

endif

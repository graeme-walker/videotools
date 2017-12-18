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
# doc/Makefile.am

HTML = example.html example-full.html example-fetch.html example-refresh.html example-iframes.html

EXTRA_DIST = abstract.txt doxygen.sh doxygen.cfg.in index.html readme.html doxygen-missing.html example.cfg example.pbm videotools.css $(HTML)

x_doc_DATA = index.html readme.html example.cfg
x_data_DATA = example.pbm $(HTML)

noinst_SCRIPTS = .dox

CLEANFILES = $(noinst_SCRIPTS) doxygen.cfg doxygen.out html/search/* html/*

.dox:
	sh "$(top_srcdir)/doc/doxygen.sh" "$(GCONFIG_HAVE_DOXYGEN)" "$(top_srcdir)" "$(top_builddir)" && touch .dox
	-@rm html/doxygen.png 2>/dev/null

install-data-local: install-x_docDATA
	$(mkinstalldirs) "$(DESTDIR)$(x_docdir)/html"
	for file in html/* ; do $(INSTALL) -m 644 "$$file" "$(DESTDIR)$(x_docdir)/$$file" ; done
	-if test -e "$(DESTDIR)$(sysconfdir)/videotools" ; then : ; else $(mkinstalldirs) "$(DESTDIR)$(sysconfdir)" ; $(INSTALL) -m 644 example.cfg "$(DESTDIR)$(sysconfdir)/videotools" ; fi

uninstall-local:
	-find "$(DESTDIR)$(x_docdir)/html" -type f | xargs rm -f
	-rmdir "$(DESTDIR)$(x_docdir)/html" 2>/dev/null
	-rmdir "$(DESTDIR)$(x_docdir)" 2>/dev/null
	-if cmp -s "$(DESTDIR)$(sysconfdir)/videotools" example.cfg then ; rm -f "$(DESTDIR)$(sysconfdir)/videotools" ; fi

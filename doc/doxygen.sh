#!/bin/sh
#
# doxygen.sh
#
# Used from doc/Makefile to run doxygen. Works correctly in a vpath build by
# creating a doxygen configuration file on the fly. Copies a dummy html 
# file if doxygen is not installed.
#
# The ephemeral doxygen config file ("doxygen.cfg") is created from the
# template file ("doxygen.cfg.in"), with variable substitution for the
# root directories.
#
# usage: doxygen.sh <have-doxygen> <top-srcdir> <top-builddir> [<doxyfile-in> [<doxyfile-out> [<missing-html>]]]
#
# eg: doxygen "$(GCONFIG_HAVE_DOXYGEN)" "$(top_srcdir)" "$(top_builddir)" doxygen.cfg.in doxygen.cfg doxygen-missing.html
#
#

HAVE_DOXYGEN="$1"
top_srcdir="$2"
top_builddir="$3"
doxyfile_in="${4-doxygen.cfg.in}"
doxyfile_out="${5-doxygen.cfg}"
missing_html="${6-doxygen-missing.html}"

if test "$HAVE_DOXYGEN" = "yes"
then 
	cat "${top_srcdir}/doc/${doxyfile_in}" | \
		sed "s:__TOP_SRC__:${top_srcdir}:g" | \
		sed "s:__TOP_BUILD__:${top_builddir}:g" | \
		cat > "${doxyfile_out}"
	rm -f doxygen.out html/index.html 2>/dev/null
	cat "${doxyfile_out}" | doxygen - > doxygen.out 2>&1
	test -f html/index.html
else 
	mkdir html 2>/dev/null
	cp -f "${top_srcdir}/doc/${missing_html}" html/index.html
fi


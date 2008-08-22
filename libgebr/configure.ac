# libgebr - GeBR Library
# Copyright (C) 2007 GeBR core team (http://gebr.sourceforge.net)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT(configure.ac)

PACKAGE=libgebr
VERSION=0.9.4pre`date +"%Y%m%d"`
TODAY=$(LANG=C date +"%a, %d %b %Y %T %z")
GEOXML_VERSION_INFO=`echo $VERSION | awk -F. '{ printf "%d:%d:%d", $1+$2, $3, $2 }'`
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)
AC_SUBST(GEOXML_VERSION_INFO)

DOCUMENT_VERSION=0.2.0
PROJECT_VERSION=0.2.0
LINE_VERSION=0.2.0
FLOW_VERSION=0.2.3
AC_SUBST(FLOW_VERSION)
AC_SUBST(LINE_VERSION)
AC_SUBST(PROJECT_VERSION)
AC_SUBST(DOCUMENT_VERSION)

dnl Debug option
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug], [turn on debugging])],,
	enable_debug=yes)
if test "$enable_debug" = "yes"; then
	DEBUG_CFLAGS="-DLIBGEBR_DEBUG"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

dnl For create static packages
AC_ARG_ENABLE([static-mode],
	[  --enable-static-mode    enables flags for generating a static package],
	STATIC_MODE=1, STATIC_MODE=0)
AC_SUBST(STATIC_MODE)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_HEADER_STDC

dnl gettext stuff
dnl GETTEXT_PACKAGE=$PACKAGE
dnl AC_SUBST(GETTEXT_PACKAGE)
dnl AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
dnl ALL_LINGUAS=""
dnl AM_GLIB_GNU_GETTEXT

dnl check for GLIB >= 2.8.0 (needed for g_access)
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.8.0],,AC_MSG_ERROR([Could not find glib >= 2.8.0]))
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

dnl check for GOBJECT >= 2.8 (which is thread safe)
PKG_CHECK_MODULES(GOBJECT, [gobject-2.0 >= 2.8],,AC_MSG_ERROR([Could not find gobject >= 2.8]))
AC_SUBST(GOBJECT_LIBS)
AC_SUBST(GOBJECT_CFLAGS)

dnl check for GTHREAD >= 2.8 (which is thread safe)
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0 >= 2.8],,AC_MSG_ERROR([Could not find gthread >= 2.8]))
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)

dnl check for GTK
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0,,AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.0.0]))
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

dnl check for gdome >= 0.8.0 (which is thread safe)
PKG_CHECK_MODULES(GDOME2, [gdome2 >= 0.8.0],,AC_MSG_ERROR([Could not find gdome2 >= 0.8.0]))
AC_SUBST(GDOME2_LIBS)
AC_SUBST(GDOME2_CFLAGS)

AC_OUTPUT([
packages/debian/changelog packages/rpm/libgebr.fedora.spec packages/rpm/libgebr.opensuse.spec packages/rpm/libgebr.mandriva.spec
Makefile src/gebr.pc src/Makefile src/defines.h packages/Makefile
src/geoxml/Makefile src/geoxml/gebr-geoxml.pc src/geoxml/geoxml/Makefile src/geoxml/geoxml/defines.h src/geoxml/data/Makefile src/geoxml/validate/Makefile src/geoxml/upgrade/Makefile src/geoxml/doc/Makefile src/geoxml/test/Makefile
src/comm/Makefile src/comm/gebr-comm.pc src/comm/comm/Makefile
src/gui/Makefile src/gui/gebr-gui.pc src/gui/gui/Makefile src/gui/pixmaps/Makefile
src/misc/Makefile src/misc/gebr-misc.pc src/misc/misc/Makefile
])

# libgebr - GeBR Library
# Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

VERSION=0.9.14
GEBR_VERSION_INFO=`echo $VERSION | awk -F. '{ printf "%d:%d:%d", $1+$2, $3, $2 }'`
VERSION="SVN"`svnversion`

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)
AC_SUBST(GEBR_VERSION_INFO)

DOCUMENT_VERSION=0.3.1
PROJECT_VERSION=0.3.1
LINE_VERSION=0.3.1
FLOW_VERSION=0.3.2
AC_SUBST(FLOW_VERSION)
AC_SUBST(LINE_VERSION)
AC_SUBST(PROJECT_VERSION)
AC_SUBST(DOCUMENT_VERSION)

dnl Debug option
AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug], [turn on debugging])],, enable_debug=yes)
AC_ARG_ENABLE(debug, [AC_HELP_STRING([--disable-debug], [turn off debugging])],, enable_debug=no)
if test "$enable_debug" = "yes"; then
	DEBUG_CFLAGS="-DDEBUG"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

dnl Option to turn on GUI build or not (useful for servers).
AC_ARG_ENABLE(gui, [AC_HELP_STRING([--enable-gui], [turn on GUI build])],, enable_gui=yes)
AC_ARG_ENABLE(gui, [AC_HELP_STRING([--disable-gui], [turn off GUI elements])],, enable_gui=no)
if test "$enable_gui" = "yes"; then
	CONFIG_SUBDIRS="geoxml comm gui"
	CONFIG_GUI="gui/gui/libgebr_gui.la"
else
	CONFIG_SUBDIRS="geoxml comm"
fi
AC_SUBST(CONFIG_SUBDIRS)
AC_SUBST(CONFIG_GUI)

dnl For static build
AC_ARG_ENABLE([static-mode],
	[AC_HELP_STRING([--enable-static-mode], [enables flags for generating a static package])]
	STATIC_MODE=1, STATIC_MODE=0)
AC_SUBST(STATIC_MODE)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

# AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL

AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_HEADER_STDC

dnl check for zlib
AC_CHECK_LIB(z,main,, [AC_MSG_ERROR(zlib required)])

dnl Gettext stuff.
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.11])

dnl check for GLIB >= 2.8.0 (needed for g_access)
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.8.0],,AC_MSG_ERROR([Could not find glib >= 2.8.0]))
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
dnl check for JSON-GLIB
PKG_CHECK_MODULES(JSONGLIB, json-glib-1.0,,
	[JSONGLIB_SUBDIR=json-glib
	JSONGLIB_LDADD=json-glib/libjson-glib-1.0.la
	AC_MSG_WARN([Could not find json-glib, building builtin])])
AC_SUBST(JSONGLIB_LIBS)
AC_SUBST(JSONGLIB_CFLAGS)
AC_SUBST(JSONGLIB_SUBDIR)
AC_SUBST(JSONGLIB_LDADD)
dnl check for GOBJECT >= 2.8 (which is thread safe)
PKG_CHECK_MODULES(GOBJECT, [gobject-2.0 >= 2.8],,AC_MSG_ERROR([Could not find gobject >= 2.8]))
AC_SUBST(GOBJECT_LIBS)
AC_SUBST(GOBJECT_CFLAGS)
dnl check for GTHREAD >= 2.8 (which is thread safe)
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0 >= 2.8],,AC_MSG_ERROR([Could not find gthread >= 2.8]))
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)
dnl check for gdome >= 0.8.0 (which is thread safe)
PKG_CHECK_MODULES(GDOME2, [gdome2 >= 0.8.0],,AC_MSG_ERROR([Could not find gdome2 >= 0.8.0]))
AC_SUBST(GDOME2_LIBS)
AC_SUBST(GDOME2_CFLAGS)
dnl check for GTK
if test "$enable_gui" = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0,,AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.0.0]))
	AC_SUBST(GTK_LIBS)
	AC_SUBST(GTK_CFLAGS)
fi
dnl webkit option
PKG_CHECK_MODULES(WEBKIT, webkit-1.0, WEBKIT_ENABLED=-DWEBKIT_ENABLED, AC_MSG_WARN([you will miss some new awesome features]))
AC_SUBST(WEBKIT_CFLAGS)
AC_SUBST(WEBKIT_LIBS)
AC_SUBST(WEBKIT_ENABLED)

AC_OUTPUT([
Makefile
packages/Makefile
packages/rpm/libgebr.fedora.spec
packages/rpm/libgebr.mandriva.spec
packages/rpm/libgebr.opensuse.spec
po/Makefile.in
src/Makefile
src/comm/Makefile src/comm/gebr-comm.pc
src/comm/comm/Makefile
src/comm/comm/json-glib/Makefile
src/defines.h
src/gebr.pc
src/geoxml/Makefile
src/geoxml/data/Makefile
src/geoxml/doc/Makefile
src/geoxml/gebr-geoxml.pc
src/geoxml/geoxml/Makefile
src/geoxml/geoxml/defines.h
src/geoxml/menu-diff/Makefile
src/geoxml/menu-edit/Makefile
src/geoxml/menu-query/Makefile
src/geoxml/upgrade/Makefile
src/geoxml/validate/Makefile
src/gui/Makefile
src/gui/gebr-gui.pc
src/gui/gui/Makefile
src/gui/icons/Makefile
src/gui/icons/gebr-theme/Makefile
src/gui/pixmaps/Makefile
src/gui/ckeditor/Makefile
src/gui/tests/Makefile
])

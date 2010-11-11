# GeBR - An environment for seismic processing.
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

AC_PREREQ([2.59])

m4_define(gebr_major_version,  0)
m4_define(gebr_minor_version,  9)
m4_define(gebr_micro_version, 19)
m4_define(gebr_version, gebr_major_version.gebr_minor_version.gebr_micro_version)

AC_INIT([gebr], [gebr_version], [http://code.google.com/p/gebr/issues/list])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PACKAGE_PREFIX_DIR=`echo ${prefix}`
AC_SUBST(PACKAGE_PREFIX_DIR)

NANOVERSION=`hg parent --template '+{latesttagdistance}-{node|short}' 2> /dev/null || date ++%H%M`
GEBR_VERSION_INFO=`echo $PACKAGE_VERSION | awk -F. '{ printf "%d:%d:%d", $0+$2, $3, $2 }'`
DOCUMENT_VERSION=0.3.2
PROJECT_VERSION=0.3.2
LINE_VERSION=0.3.2
FLOW_VERSION=0.3.6
AC_SUBST(FLOW_VERSION)
AC_SUBST(LINE_VERSION)
AC_SUBST(PROJECT_VERSION)
AC_SUBST(DOCUMENT_VERSION)
AC_SUBST(GEBR_VERSION_INFO)

INSTALL_BIN=/usr/bin/install
$INSTALL_BIN -C configure.ac __install_test 2>/dev/null
if [[ $? -eq 0 \]]; then
        rm __install_test
        INSTALL="$INSTALL_BIN -C"
        AC_ARG_VAR(INSTALL)
else
	INSTALL_BIN=`pwd`/install-sh
	$INSTALL_BIN -C configure.ac __install_test 2>/dev/null
	if [[ $? -eq 0 \]]; then
        	rm __install_test
	        INSTALL="$INSTALL_BIN -C"
        	AC_ARG_VAR(INSTALL)
	fi
fi

GLIB_MINOR_VERSION=`pkg-config glib-2.0 --modversion | awk -F. '{ printf "%d", $2 }'`
AM_CONDITIONAL(ENABLE_TESTS, test ${GLIB_MINOR_VERSION} -ge 16)

dnl Debug option
AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug], [turn on debugging])],, enable_debug=yes)
if test "$enable_debug" = "yes"; then
	DEBUG_CFLAGS="-DDEBUG -g3 -O0"
else
	DEBUG_CFLAGS=""
	NANOVERSION=""
fi
AC_SUBST(DEBUG_CFLAGS)
AC_SUBST(NANOVERSION)

AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_LIBTOOL
AC_HEADER_STDC
AM_GLIB_GNU_GETTEXT

dnl Gettext stuff
IT_PROG_INTLTOOL

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

dnl check for zlib
AC_CHECK_LIB(z,main,, [AC_MSG_ERROR(zlib required)])
# check for gtk-doc
GTK_DOC_CHECK([1.11],[--flavour no-tmpl])
dnl check for GLIB >= 2.8.0 (needed for g_access)
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.12.0],,AC_MSG_ERROR([Could not find glib >= 2.12.0]))
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
dnl check for gdome >= 0.8.0 (which is thread safe)
PKG_CHECK_MODULES(GDOME2, [gdome2 >= 0.8.0],,AC_MSG_ERROR([Could not find gdome2 >= 0.8.0]))
AC_SUBST(GDOME2_LIBS)
AC_SUBST(GDOME2_CFLAGS)
dnl check for GTK
if test "$enable_gui" = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.12.0,,AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.12.0]))
	AC_SUBST(GTK_LIBS)
	AC_SUBST(GTK_CFLAGS)

	PKG_CHECK_MODULES(WEBKIT, webkit-1.0,, AC_MSG_ERROR([Could not find webkitgtk]))
	AC_SUBST(WEBKIT_CFLAGS)
	AC_SUBST(WEBKIT_LIBS)
fi

GEBR_CFLAGS='-I$(top_srcdir) -I$(top_srcdir)/libgebr $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GTHREAD_CFLAGS)'
GEBR_GEOXML_CFLAGS='-I$(top_srcdir)/libgebr/geoxml $(GDOME2_CFLAGS)'
GEBR_COMM_CFLAGS='-I$(top_srcdir)/libgebr/comm'
GEBR_GUI_CFLAGS='-I$(top_srcdir)/libgebr/gui $(GTK_CFLAGS) $(WEBKIT_CFLAGS)'

GEBR_LIBS='$(top_builddir)/libgebr/libgebr.la $(GLIB_LIBS) $(GOBJECT_LIBS) $(GTHREAD_LIBS)'
GEBR_GEOXML_LIBS='$(top_builddir)/libgebr/geoxml/libgebr_geoxml.la $(GDOME2_LIBS)'
GEBR_COMM_LIBS='$(top_builddir)/libgebr/comm/libgebr_comm.la'
GEBR_GUI_LIBS='$(top_builddir)/libgebr/gui/libgebr_gui.la $(GTK_LIBS) $(WEBKIT_LIBS)'

AC_SUBST(GEBR_CFLAGS)
AC_SUBST(GEBR_GEOXML_CFLAGS)
AC_SUBST(GEBR_COMM_CFLAGS)
AC_SUBST(GEBR_GUI_CFLAGS)

AC_SUBST(GEBR_LIBS)
AC_SUBST(GEBR_GEOXML_LIBS)
AC_SUBST(GEBR_COMM_LIBS)
AC_SUBST(GEBR_GUI_LIBS)

AC_CONFIG_FILES([
Makefile
po/Makefile.in
libgebr/Makefile
libgebr/docs/Makefile
libgebr/docs/reference/Makefile
libgebr/docs/reference/libgebr/Makefile
libgebr/docs/reference/libgebr/version.xml
libgebr/packages/Makefile
libgebr/packages/rpm/libgebr.fedora.spec
libgebr/packages/rpm/libgebr.mandriva.spec
libgebr/packages/rpm/libgebr.opensuse.spec
libgebr/po/Makefile
libgebr/comm/Makefile
libgebr/comm/gebr-comm.pc
libgebr/comm/socketchannel/Makefile
libgebr/comm/tests/Makefile
libgebr/defines.h
libgebr/gebr.pc
libgebr/geoxml/Makefile
libgebr/geoxml/data/Makefile
libgebr/geoxml/gebr-geoxml.pc
libgebr/geoxml/menu-diff/Makefile
libgebr/geoxml/menu-edit/Makefile
libgebr/geoxml/menu-query/Makefile
libgebr/geoxml/flow-dump/Makefile
libgebr/geoxml/tests/Makefile
libgebr/geoxml/upgrade/Makefile
libgebr/geoxml/validate/Makefile
libgebr/gui/Makefile
libgebr/gui/ckeditor/Makefile
libgebr/gui/data/Makefile
libgebr/gui/gebr-gui.pc
libgebr/gui/icons/Makefile
libgebr/gui/icons/gebr-theme/Makefile
libgebr/gui/pixmaps/Makefile
libgebr/gui/tests/Makefile
gebrd/packages/rpm/gebrd.fedora.spec
gebrd/packages/rpm/gebrd.opensuse.spec
gebrd/packages/rpm/gebrd.mandriva.spec
gebrd/Makefile
gebrd/src/Makefile
gebrd/src/defines.h
gebrd/packages/Makefile
gebrd/po/Makefile
gebrd/tests/Makefile
gebrd/doc/Makefile
gebr/packages/rpm/gebr.fedora.spec
gebr/packages/rpm/gebr.opensuse.spec
gebr/packages/rpm/gebr.mandriva.spec
gebr/Makefile
gebr/menus/Makefile
gebr/packages/Makefile
gebr/src/Makefile 
gebr/src/defines.h
gebr/src/gebr-client/Makefile
gebr/src/gebr/Makefile
gebr/src/gebr/tests/Makefile
gebr/gebr.desktop
gebr/po/Makefile
gebr/doc/Makefile
gebr/doc/userdoc/Makefile
gebr/doc/develdoc/Makefile
debr/Makefile
debr/data/Makefile
debr/data/help-template.html
debr/debr.desktop
debr/doc/Makefile
debr/doc/userdoc/Makefile
debr/packages/Makefile 
debr/packages/rpm/debr.fedora.spec
debr/packages/rpm/debr.mandriva.spec
debr/packages/rpm/debr.opensuse.spec
debr/po/Makefile
debr/src/Makefile
debr/src/defines.h
debr/tests/Makefile
])

AC_OUTPUT

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

# Making releases:
#   GEBR_MICRO_VERSION += 1;
#   GEBR_INTERFACE_AGE += 1;
#   GEBR_BINARY_AGE += 1;
# if any functions have been added, set GEBR_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set GEBR_BINARY_AGE and GEBR_INTERFACE_AGE to 0.

m4_define(gebr_major_version,   [0])
m4_define(gebr_minor_version,  [11])
m4_define(gebr_micro_version,   [0])
m4_define([gebr_interface_age], [0])
m4_define([gebr_binary_age],
          [m4_eval(100 * gebr_minor_version + gebr_micro_version)])
m4_define(gebr_version, gebr_major_version.gebr_minor_version.gebr_micro_version)

AC_INIT([gebrproject], [gebr_version], [http://code.google.com/p/gebr/issues/list])
AC_CONFIG_SRCDIR([libgebr/libgebr.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ============================================================================
dnl Defining the library version information
dnl ============================================================================
m4_define([lt_current], [m4_eval(100 * gebr_minor_version + gebr_micro_version - gebr_interface_age)])
m4_define([lt_revision], [gebr_interface_age])
m4_define([lt_age], [m4_eval(gebr_binary_age - gebr_interface_age)])
GEBR_VERSION_INFO="lt_current:lt_revision:lt_age"
AC_SUBST(GEBR_VERSION_INFO)

dnl ============================================================================
dnl Other versions stuffs
dnl ============================================================================
GEBR_MAJOR_VERSION=gebr_major_version
GEBR_MINOR_VERSION=gebr_minor_version
GEBR_MICRO_VERSION=gebr_micro_version
GEBR_INTERFACE_AGE=gebr_interface_age
GEBR_BINARY_AGE=gebr_binary_age
AC_SUBST(GEBR_MAJOR_VERSION)
AC_SUBST(GEBR_MINOR_VERSION)
AC_SUBST(GEBR_MICRO_VERSION)
AC_SUBST(GEBR_INTERFACE_AGE)
AC_SUBST(GEBR_BINARY_AGE)

DOCUMENT_VERSION=0.3.2
AC_SUBST(DOCUMENT_VERSION)
PROJECT_VERSION=0.3.2
AC_SUBST(PROJECT_VERSION)
LINE_VERSION=0.3.2
AC_SUBST(LINE_VERSION)
FLOW_VERSION=0.3.6
AC_SUBST(FLOW_VERSION)

dnl ============================================================================
dnl Need for creating the *.desktop files
dnl ============================================================================
PACKAGE_PREFIX_DIR=`echo ${prefix}`
AC_SUBST(PACKAGE_PREFIX_DIR)

dnl ============================================================================
dnl Common compilation cflags
dnl ============================================================================
COMMON_CFLAGS='-Wall -std=gnu99 -fshow-column'
AC_SUBST(COMMON_CFLAGS)

dnl ============================================================================
dnl The nano-version is the mercurial revision hash.
dnl Notice that it is escaped so the shell expansion only occurs when `make'
dnl is executed.
dnl ============================================================================
NANOVERSION='`hg parent --template "+{latesttagdistance}-{node|short}" 2> /dev/null || date ++%H%M`'
AC_SUBST(NANOVERSION)

dnl ============================================================================
dnl Check if glib support test suit
dnl ============================================================================
GLIB_MINOR_VERSION=`pkg-config glib-2.0 --modversion | awk -F. '{ printf "%d", $2 }'`
AM_CONDITIONAL(ENABLE_TESTS, test ${GLIB_MINOR_VERSION} -ge 16)

dnl ============================================================================
dnl Debugging options
dnl ============================================================================
AC_ARG_ENABLE([debug],
	      [AC_HELP_STRING([--enable-debug], [turn on debugging])],
	      [enable_debug=no],
	      [enable_debug=yes])
if test "x$enable_debug" = "xyes"; then
	DEBUG_CFLAGS="-DDEBUG"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

dnl ============================================================================
dnl Check if we have docbook
dnl ============================================================================
AC_CHECK_PROG(DB2HTML, db2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK, $DB2HTML)
if $DB2HTML; then
	have_docbook=yes;
else
	have_docbook=no;
fi

AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_LIBTOOL
AC_HEADER_STDC

dnl ============================================================================
dnl Gettext stuff. Don't define GETTEXT_PACKAGE since we are dealing with
dnl multiple po/ directories. Each one needs its own GETTEXT_PACKAGE.
dnl ============================================================================
GEBR_PROG_INTLTOOL
GEBR_PO_SUBDIR([libgebr/po])
GEBR_PO_SUBDIR([gebrd/po])
GEBR_PO_SUBDIR([gebr/po])
GEBR_PO_SUBDIR([debr/po])

dnl ============================================================================
dnl Option to turn on GUI build or not (useful for servers).
dnl ============================================================================
AC_ARG_ENABLE([gui],
	      [AC_HELP_STRING([--enable-gui], [turn on GUI build])],
	      [enable_gui=no],
	      [enable_gui=yes])

AC_ARG_ENABLE([gui],
	      [AC_HELP_STRING([--disable-gui], [turn off GUI elements])],
	      [enable_gui=yes],
	      [enable_gui=no])

if test "$enable_gui" = "yes"; then
	CONFIG_SUBDIRS="geoxml comm gui"
	CONFIG_GUI="gui/libgebr_gui.la"
else
	CONFIG_SUBDIRS="geoxml comm"
fi
AC_SUBST(CONFIG_SUBDIRS)
AC_SUBST(CONFIG_GUI)

dnl ============================================================================
dnl Checking for tidy library
dnl ============================================================================
AC_ARG_WITH([tidy],
	    [AC_HELP_STRING([--with-tidy],
			   [use TIDY library to format XML outputs [default=yes]])],
	    [want_tidy=${withval}],
	    [want_tidy=yes])

ENABLE_TIDY=0
if test "x${want_tidy}" = xyes; then
	AC_SEARCH_LIBS([tidyCreate], [tidy],
		       [have_tidy=yes],
		       [have_tidy=no])

	if test "x${have_tidy}" = xyes; then
		AC_CHECK_HEADERS([tidy/tidy.h tidy/buffio.h],
				 [],
				 [have_tidy=no])
	fi

	if test "x${have_tidy}" = xyes; then
		ENABLE_TIDY=1
	else
		echo "----------------------------------------"
		echo " Error: libtidy support is enabled but"
		echo " there seems to be no libtidy installed"
		echo " on your system. Consider installing it"
		echo " or disabling tidy support with"
		echo " --without-tidy flag."
		echo "----------------------------------------"
		(exit 1); exit 1;
	fi
fi
AC_SUBST(ENABLE_TIDY)

dnl ============================================================================
dnl Checking for zlib library
dnl ============================================================================
AC_CHECK_LIB(z,main,,[AC_MSG_ERROR(zlib required)])

dnl ============================================================================
dnl Checking for GtkDoc
dnl ============================================================================
GTK_DOC_CHECK([1.11],[--flavour no-tmpl])

dnl ============================================================================
dnl Checking for Packages
dnl ============================================================================
PKG_CHECK_MODULES([GLIB],
		  [glib-2.0 >= 2.12.0],
		  [],
		  [AC_MSG_ERROR([Could not find glib >= 2.12.0])])
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES([GOBJECT],
		  [gobject-2.0 >= 2.8],
		  [],
		  [AC_MSG_ERROR([Could not find gobject >= 2.8])])
AC_SUBST(GOBJECT_LIBS)
AC_SUBST(GOBJECT_CFLAGS)

PKG_CHECK_MODULES([GTHREAD],
		  [gthread-2.0 >= 2.8],
		  [],
		  [AC_MSG_ERROR([Could not find gthread >= 2.8])])
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)

PKG_CHECK_MODULES([GDOME2],
		  [gdome2 >= 0.8.0],
		  [],
		  [AC_MSG_ERROR([Could not find gdome2 >= 0.8.0])])
AC_SUBST(GDOME2_LIBS)
AC_SUBST(GDOME2_CFLAGS)

if test "$enable_gui" = "yes"; then
	PKG_CHECK_MODULES([GTK],
			  [gtk+-2.0 >= 2.12.0],
			  [],
			  [AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.12.0])])
	AC_SUBST(GTK_LIBS)
	AC_SUBST(GTK_CFLAGS)

	PKG_CHECK_MODULES([WEBKIT],
			  [webkit-1.0],
			  [],
			  [AC_MSG_ERROR([Could not find webkitgtk])])
	AC_SUBST(WEBKIT_CFLAGS)
	AC_SUBST(WEBKIT_LIBS)
fi

GEBR_CFLAGS='-I$(top_srcdir) -I$(top_srcdir)/libgebr $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GTHREAD_CFLAGS)'
GEBR_GEOXML_CFLAGS='-I$(top_srcdir)/libgebr/geoxml $(GDOME2_CFLAGS) $(TIDY_FLAGS)'
GEBR_COMM_CFLAGS='-I$(top_srcdir)/libgebr/comm'
GEBR_GUI_CFLAGS='-I$(top_srcdir)/libgebr/gui $(GTK_CFLAGS) $(WEBKIT_CFLAGS)'

GEBR_LIBS='$(top_builddir)/libgebr/libgebr.la $(GLIB_LIBS) $(GOBJECT_LIBS) $(GTHREAD_LIBS)'
GEBR_GEOXML_LIBS='$(top_builddir)/libgebr/geoxml/libgebr_geoxml.la $(GDOME2_LIBS) $(TIDY_LIBS)'
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
docs/Makefile
docs/gebr/Makefile
docs/debr/Makefile
docs/reference/Makefile
docs/reference/libgebr/Makefile
docs/reference/libgebr/version.xml
rpm/Makefile
rpm/gebrproject.fedora.spec

libgebr/Makefile
libgebr/comm/Makefile
libgebr/comm/gebr-comm.pc
libgebr/comm/socketchannel/Makefile
libgebr/comm/tests/Makefile
libgebr/config.h
libgebr/defines.h
libgebr/gebr.pc
libgebr/geoxml/Makefile
libgebr/geoxml/data/Makefile
libgebr/geoxml/flow-dump/Makefile
libgebr/geoxml/gebr-geoxml.pc
libgebr/geoxml/menu-diff/Makefile
libgebr/geoxml/menu-edit/Makefile
libgebr/geoxml/menu-query/Makefile
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
libgebr/po/Makefile.in
libgebr/tests/Makefile

gebrd/Makefile
gebrd/doc/Makefile
gebrd/po/Makefile.in
gebrd/src/Makefile
gebrd/src/defines.h
gebrd/tests/Makefile

gebr/Makefile
gebr/data/Makefile
gebr/data/gebr.desktop.in
gebr/defines.h
gebr/gebr-client/Makefile
gebr/gebr/Makefile
gebr/po/Makefile.in
gebr/tests/Makefile

debr/Makefile
debr/data/Makefile
debr/data/debr.desktop.in
debr/data/help-template.html
debr/po/Makefile.in
debr/src/Makefile
debr/src/defines.h
debr/tests/Makefile

])

AC_OUTPUT

echo "---------------------------------------"
echo " Configuration summary:"
echo " "
echo " Enable debugging messages?    $enable_debug"
echo " Enable GUI support?           $enable_gui"
echo " Enable Tidy support?          $have_tidy"
echo " Has Dockbook to build user's"
echo " documentation?                $have_docbook"
echo "---------------------------------------"

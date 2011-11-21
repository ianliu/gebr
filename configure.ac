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
m4_define(gebr_minor_version,  [15])
m4_define(gebr_micro_version,   [4])
m4_define([gebr_interface_age], [0])
m4_define([gebr_binary_age],
          [m4_eval(100 * gebr_minor_version + gebr_micro_version)])
m4_define(gebr_version, gebr_major_version.gebr_minor_version.gebr_micro_version)

AC_INIT([gebrproject], [gebr_version], [http://code.google.com/p/gebr/issues/list])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([libgebr/libgebr.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

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

DOCUMENT_VERSION=0.3.3
AC_SUBST(DOCUMENT_VERSION)
PROJECT_VERSION=0.3.3
AC_SUBST(PROJECT_VERSION)
LINE_VERSION=0.3.4
AC_SUBST(LINE_VERSION)
FLOW_VERSION=0.3.7
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
dnl Check if glib support test suit
dnl ============================================================================
GLIB_MINOR_VERSION=`pkg-config glib-2.0 --modversion | awk -F. '{ printf "%d", $2 }'`
AM_CONDITIONAL(ENABLE_TESTS, test ${GLIB_MINOR_VERSION} -ge 16)

dnl ============================================================================
dnl Debugging options
dnl ============================================================================
AC_ARG_ENABLE([debug],
	      [AC_HELP_STRING([--enable-debug], [turn on debugging])],
	      [enable_debug=$enableval],
	      [enable_debug=yes])
if test "x$enable_debug" = "xyes"; then
	DEBUG_CFLAGS="-DDEBUG"
	CFLAGS="$CFLAGS -g3 -O0"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

dnl ============================================================================
dnl The nano-version is the mercurial revision hash.
dnl Notice that it is escaped so the shell expansion only occurs when `make'
dnl is executed.
dnl ============================================================================
if test "x$enable_debug" = "xyes"; then
	NANOVERSION='`hg parent --template "+{latesttagdistance}-{node|short}" 2> /dev/null || date ++%Y%m%d.%H%M`'
else
	NANOVERSION=""
fi
AC_SUBST(NANOVERSION)

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
	      [enable_gui=$enableval],
	      [enable_gui=yes])
if test "$enable_gui" = "yes"; then
	AM_CONDITIONAL(ENABLE_GUI, true)
else
	AM_CONDITIONAL(ENABLE_GUI, false)
fi

dnl ============================================================================
dnl Check if we have docbook
dnl ============================================================================
if test "$enable_gui" = "yes"; then
	AC_CHECK_PROG(XSLTPROC, xsltproc, true, false)
	AM_CONDITIONAL(HAVE_DOCBOOK, $XSLTPROC)
	if $XSLTPROC; then
		have_docbook=yes;
	else
		have_docbook=no;
	fi
else
	have_docbook=no;
	AM_CONDITIONAL(HAVE_DOCBOOK, false)
fi

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
				 [AC_CHECK_HEADERS([tidy.h buffio.h],
						   [],
						   [have_tidy=no;])])
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
GTK_DOC_CHECK([1.10],[--flavour no-tmpl])

dnl ============================================================================
dnl Checking for Packages
dnl ============================================================================
PKG_CHECK_MODULES([GLIB], [
	glib-2.0    >= 2.22.0
	gobject-2.0 >= 2.22.0
	gthread-2.0 >= 2.22.0
])
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES([GDOME2], [gdome2 >= 0.8.0])
AC_SUBST(GDOME2_LIBS)
AC_SUBST(GDOME2_CFLAGS)

if test "$enable_gui" = "yes"; then
	PKG_CHECK_MODULES([GUI], [
		gtk+-2.0   >= 2.18.0
		webkit-1.0 >= 1.0.1
	])
	AC_SUBST(GUI_LIBS)
	AC_SUBST(GUI_CFLAGS)
fi

GEBR_CFLAGS='-I$(top_srcdir) -I$(top_srcdir)/libgebr -I$(top_builddir) -I$(top_builddir)/libgebr'
GEBR_GEOXML_CFLAGS='-I$(top_srcdir)/libgebr/geoxml -I$(top_builddir)/libgebr/geoxml'
GEBR_COMM_CFLAGS='-I$(top_srcdir)/libgebr/comm -I$(top_builddir)/libgebr/comm'
GEBR_GUI_CFLAGS='-I$(top_srcdir)/libgebr/gui -I$(top_builddir)/libgebr/gui'
GEBR_JSON_CFLAGS='-I$(top_srcdir)/libgebr/json -I$(top_builddir)/libgebr/json'

GEBR_LIBS='$(top_builddir)/libgebr/libgebr.la'
GEBR_JSON_LIBS='$(top_builddir)/libgebr/json/libgebr_json.la -lm'
GEBR_GEOXML_LIBS='$(top_builddir)/libgebr/geoxml/libgebr_geoxml.la'
GEBR_COMM_LIBS='$(top_builddir)/libgebr/comm/libgebr_comm.la'
GEBR_GUI_LIBS='$(top_builddir)/libgebr/gui/libgebr_gui.la'

AC_SUBST(GEBR_CFLAGS)
AC_SUBST(GEBR_GEOXML_CFLAGS)
AC_SUBST(GEBR_COMM_CFLAGS)
AC_SUBST(GEBR_GUI_CFLAGS)
AC_SUBST(GEBR_JSON_CFLAGS)

AC_SUBST(GEBR_LIBS)
AC_SUBST(GEBR_GEOXML_LIBS)
AC_SUBST(GEBR_COMM_LIBS)
AC_SUBST(GEBR_GUI_LIBS)
AC_SUBST(GEBR_JSON_LIBS)

dnl ============================================================================
dnl Paths & Directories definitions
dnl ============================================================================

if test "x$prefix" = xNONE ; then
	TMP_PREFIX=$ac_default_prefix
else
	TMP_PREFIX=$prefix
fi

AC_DEFINE_UNQUOTED(ENABLE_TIDY, [$ENABLE_TIDY],
		   [If one, tidy is enabled, zero is disabled])

DATA_DIR=`echo $TMP_PREFIX/share`
AC_DEFINE_UNQUOTED(DATA_DIR, ["$DATA_DIR"], [Data dir])

PACKAGE_LOCALE_DIR="$DATA_DIR/locale"
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["$PACKAGE_LOCALE_DIR"], [Locale dir])

AC_DEFINE_UNQUOTED(LIBGEBR_VERSION, ["$VERSION"], [libgebr version])

LIBGEBR_DATA_DIR="$DATA_DIR/libgebr"
AC_DEFINE_UNQUOTED(LIBGEBR_DATA_DIR, ["$LIBGEBR_DATA_DIR"], [libgebr data dir])

LIBGEBR_HELP_TEMPLATE="$LIBGEBR_DATA_DIR/geoxml/help-template.html"
AC_DEFINE_UNQUOTED(LIBGEBR_HELP_TEMPLATE, ["$LIBGEBR_HELP_TEMPLATE"], [Help template])

LIBGEBR_PIXMAPS_DIR="$LIBGEBR_DATA_DIR/pixmaps"
AC_DEFINE_UNQUOTED(LIBGEBR_PIXMAPS_DIR, ["$LIBGEBR_PIXMAPS_DIR"], [libgebr pixmap dir])

LIBGEBR_ICONS_DIR="$LIBGEBR_DATA_DIR/icons"
AC_DEFINE_UNQUOTED(LIBGEBR_ICONS_DIR, ["$LIBGEBR_ICONS_DIR"], [libgebr icons dir])

LIBGEBR_STYLES_DIR="$LIBGEBR_DATA_DIR/styles"
AC_DEFINE_UNQUOTED(LIBGEBR_STYLES_DIR, ["$LIBGEBR_STYLES_DIR"], [libgebr styles])

GEBR_GEOXML_FLOW_VERSION="$FLOW_VERSION"
AC_DEFINE_UNQUOTED(GEBR_GEOXML_FLOW_VERSION, ["$GEBR_GEOXML_FLOW_VERSION"], [Flow version])

GEBR_GEOXML_LINE_VERSION="$LINE_VERSION"
AC_DEFINE_UNQUOTED(GEBR_GEOXML_LINE_VERSION, ["$GEBR_GEOXML_LINE_VERSION"], [Line version])

GEBR_GEOXML_PROJECT_VERSION="$PROJECT_VERSION"
AC_DEFINE_UNQUOTED(GEBR_GEOXML_PROJECT_VERSION, ["$GEBR_GEOXML_PROJECT_VERSION"], [Project version])

GEBR_GEOXML_DOCUMENT_VERSION="$DOCUMENT_VERSION"
AC_DEFINE_UNQUOTED(GEBR_GEOXML_DOCUMENT_VERSION, ["$GEBR_GEOXML_DOCUMENT_VERSION"],
		   [General document version])

GEBR_GEOXML_DTD_DIR="$LIBGEBR_DATA_DIR/geoxml"
AC_DEFINE_UNQUOTED(GEBR_GEOXML_DTD_DIR, ["$GEBR_GEOXML_DTD_DIR"], [DTD dir])

GEBR_DATA_DIR="$DATA_DIR/gebr"
AC_DEFINE_UNQUOTED(GEBR_DATA_DIR, ["$GEBR_DATA_DIR"], ["GeBR's data dir"])

GEBR_GLADE_DIR="$GEBR_DATA_DIR/glade"
AC_DEFINE_UNQUOTED(GEBR_GLADE_DIR, ["$GEBR_GLADE_DIR"], ["GeBR's interface files"])

GEBR_VERSION="$VERSION"
AC_DEFINE_UNQUOTED(GEBR_VERSION, ["$GEBR_VERSION"], ["GeBR's version"])

GEBR_SYS_MENUS_DIR="$GEBR_DATA_DIR/menus"
AC_DEFINE_UNQUOTED(GEBR_SYS_MENUS_DIR, ["$GEBR_SYS_MENUS_DIR"], [Menus dir])

GEBR_SYS_DEMOS_DIR="$GEBR_DATA_DIR/demos"
AC_DEFINE_UNQUOTED(GEBR_SYS_DEMOS_DIR, ["$GEBR_SYS_DEMOS_DIR"], [Demos dir])

GEBR_USERDOC_DIR="$DATA_DIR/doc/gebr"
AC_DEFINE_UNQUOTED(GEBR_USERDOC_DIR, ["$GEBR_USERDOC_DIR"], [Documentation dir])

DEBR_DATA_DIR="$DATA_DIR/debr"
AC_DEFINE_UNQUOTED(DEBR_DATA_DIR, ["$DEBR_DATA_DIR"], ["DeBR's data dir"])

DEBR_GLADE_DIR="$DEBR_DATA_DIR/glade"
AC_DEFINE_UNQUOTED(DEBR_GLADE_DIR, ["$DEBR_GLADE_DIR"], [Glade dir])

DEBR_HELP_EDIT_CKEDITOR="$LIBGEBR_DATA_DIR/ckeditor/ckeditor.js"
AC_DEFINE_UNQUOTED(DEBR_HELP_EDIT_CKEDITOR, ["$DEBR_HELP_EDIT_CKEDITOR"],
		   [TODO Use this instead of dir?])

DEBR_USERDOC_HTML="$DEBR_DATA_DIR/userdoc/debr.html"
AC_DEFINE_UNQUOTED(DEBR_USERDOC_HTML, ["$DEBR_USERDOC_HTML"], [User documentation])

GEBRD_VERSION="$VERSION"
AC_DEFINE_UNQUOTED(GEBRD_VERSION, ["$GEBRD_VERSION"], ["GeBRd's version"]) 

AC_CONFIG_FILES([
Makefile

gtester/Makefile

docs/Makefile
docs/gebr/Makefile
docs/gebr/common-images/Makefile
docs/gebr/en/Makefile
docs/gebr/pt_BR/Makefile
docs/reference/Makefile
docs/reference/libgebr/Makefile
docs/reference/libgebr/version.xml
rpm/Makefile
rpm/gebrproject.fedora.spec
rpm/gebrproject.centos.spec

libgebr/Makefile
libgebr/comm/Makefile
libgebr/comm/gebr-comm.pc
libgebr/comm/socketchannel/Makefile
libgebr/comm/tests/Makefile
libgebr/gebr.pc
libgebr/geoxml-utils/Makefile
libgebr/geoxml/Makefile
libgebr/geoxml/data/Makefile
libgebr/geoxml/data/help-template.html
libgebr/geoxml/gebr-geoxml.pc
libgebr/geoxml/tests/Makefile
libgebr/gui/Makefile
libgebr/gui/ckeditor/Makefile
libgebr/gui/data/Makefile
libgebr/gui/gebr-gui.pc
libgebr/gui/icons/Makefile
libgebr/gui/icons/gebr-theme/Makefile
libgebr/gui/pixmaps/Makefile
libgebr/gui/tests/Makefile
libgebr/json/Makefile
libgebr/json/tests/Makefile
libgebr/po/Makefile.in
libgebr/tests/Makefile

gebrd/Makefile
gebrd/doc/Makefile
gebrd/po/Makefile.in
gebrd/tests/Makefile

gebr/Makefile
gebr/data/Makefile
gebr/data/gebr.desktop.in
gebr/po/Makefile.in
gebr/tests/Makefile

debr/Makefile
debr/data/Makefile
debr/data/debr.desktop.in
debr/po/Makefile.in
debr/tests/Makefile

])

AC_OUTPUT

echo "---------------------------------------"
echo " Configuration summary:"
echo " "
echo " Enable debugging messages?    $enable_debug"
echo " Enable GUI support?           $enable_gui"
echo " Enable Tidy support?          $want_tidy"
echo " Has Dockbook to build user's"
echo " documentation?                $have_docbook"
echo " CFLAGS                        $CFLAGS"
echo "---------------------------------------"

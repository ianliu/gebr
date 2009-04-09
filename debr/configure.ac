# DeBR - GeBR Menu Designer
# Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

PACKAGE=debr
VERSION=0.9.12
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)

FLOW_VERSION=0.3.0
AC_SUBST(FLOW_VERSION)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
PACKAGE_NAME=$PACKAGE
PACKAGE_VERSION=$VERSION

AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_HEADER_STDC

dnl Debug option
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],
	[turn on debugging])],,
	enable_debug=yes)
if test "$enable_debug" = "yes"; then
	DEBUG_CFLAGS="-DDEBR_DEBUG"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

dnl gettext stuff
dnl GETTEXT_PACKAGE=$PACKAGE
dnl AC_SUBST(GETTEXT_PACKAGE)
dnl AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
dnl ALL_LINGUAS=""
dnl AM_GLIB_GNU_GETTEXT

dnl check for GTK
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0,,AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.0.0]))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl check for libgebr_geoxml
PKG_CHECK_MODULES(LIBGEBR_GEOXML, gebr-geoxml,,AC_MSG_ERROR([Could not find libgebr_geoxml]))
AC_SUBST(LIBGEBR_GEOXML_CFLAGS)
AC_SUBST(LIBGEBR_GEOXML_LIBS)

dnl check for libgebr_gui
PKG_CHECK_MODULES(LIBGEBR_GUI, gebr-gui,,AC_MSG_ERROR([Could not find libgebr_gui]))
AC_SUBST(LIBGEBR_GUI_CFLAGS)
AC_SUBST(LIBGEBR_GUI_LIBS)

dnl check for libgebr_misc
PKG_CHECK_MODULES(LIBGEBR_MISC, gebr-misc,,AC_MSG_ERROR([Could not find libgebr_misc]))
AC_SUBST(LIBGEBR_MISC_CFLAGS)
AC_SUBST(LIBGEBR_MISC_LIBS)

AC_OUTPUT([
packages/rpm/debr.fedora.spec packages/rpm/debr.opensuse.spec packages/rpm/debr.mandriva.spec
data/help-template.html packages/Makefile 
Makefile data/Makefile src/Makefile src/defines.h
])

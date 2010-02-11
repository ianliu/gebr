# GeBR Daemon - Process and control execution of flows
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

PACKAGE=gebrd
VERSION=`if test -d .svn; then echo -n r; svnversion; else git log -1 --date=local --pretty=%cd; fi`
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)

m4_include(../configure.ac.decl)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
PACKAGE_NAME=$PACKAGE
PACKAGE_VERSION=$VERSION

IT_PROG_INTLTOOL

AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_HEADER_STDC

dnl Debug option
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],
	[turn on debugging])],,
	enable_debug=yes)
if test "$enable_debug" = "yes"; then
	DEBUG_CFLAGS="-DGEBRD_DEBUG"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

dnl gettext stuff
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.11])

dnl check for libgebr
PKG_CHECK_MODULES(GEBR, gebr,,AC_MSG_ERROR([Could not find libgebr]))
AC_SUBST(GEBR_CFLAGS)
AC_SUBST(GEBR_LIBS)
dnl check for libgebr_geoxml
PKG_CHECK_MODULES(GEOXML, gebr-geoxml,,AC_MSG_ERROR([Could not find libgebr_geoxml]))
AC_SUBST(GEBR_GEOXML_CFLAGS)
AC_SUBST(GEBR_GEOXML_LIBS)
dnl check for libgebr_comm
PKG_CHECK_MODULES(GEBR_COMM, gebr-comm,,AC_MSG_ERROR([Could not find libgebr_comm]))
AC_SUBST(GEBR_COMM_CFLAGS)
AC_SUBST(GEBR_COMM_LIBS)

AC_OUTPUT([
packages/rpm/gebrd.fedora.spec packages/rpm/gebrd.opensuse.spec packages/rpm/gebrd.mandriva.spec
Makefile src/Makefile src/defines.h packages/Makefile po/Makefile.in tests/Makefile
])

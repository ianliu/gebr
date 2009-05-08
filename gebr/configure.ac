# GeBR - An environment for seismic processing.
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

PACKAGE=gebr
VERSION=0.9.12
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)

dnl Debug option
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],
	[turn on debugging])],,
	enable_debug=yes)
if test "$enable_debug" = "yes"; then
	DEBUG_CFLAGS="-DGEBR_DEBUG"
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
PACKAGE_NAME=$PACKAGE
PACKAGE_VERSION=$VERSION

# AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL([0.40.0])

AC_PROG_INSTALL
AC_PROG_CC

dnl Gettext stuff.
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT

dnl check for GTK
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0,,AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.0.0]))
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

dnl check for libgebr
PKG_CHECK_MODULES(LIBGEBR, gebr,,AC_MSG_ERROR([Could not find libgebr]))
AC_SUBST(LIBGEBR_CFLAGS)
AC_SUBST(LIBGEBR_LIBS)

dnl check for libgebr_comm
PKG_CHECK_MODULES(LIBGEBR_COMM, gebr-comm,,AC_MSG_ERROR([Could not find libgebr_comm]))
AC_SUBST(LIBGEBR_COMM_CFLAGS)
AC_SUBST(LIBGEBR_COMM_LIBS)

dnl check for libgebr_misc
PKG_CHECK_MODULES(LIBGEBR_MISC, gebr-misc,,AC_MSG_ERROR([Could not find libgebr_misc]))
AC_SUBST(LIBGEBR_MISC_CFLAGS)
AC_SUBST(LIBGEBR_MISC_LIBS)

AC_OUTPUT([
packages/rpm/gebr.fedora.spec packages/rpm/gebr.opensuse.spec packages/rpm/gebr.mandriva.spec
Makefile data/Makefile menus/Makefile packages/Makefile
src/Makefile src/defines.h src/gebr/Makefile src/gebr-client/Makefile
gebr.desktop
po/Makefile.in
])

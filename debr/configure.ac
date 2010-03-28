# DeBR - GeBR Menu Designer
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

m4_include(configure.ac.decl)

PACKAGE=debr
AC_SUBST(PACKAGE)
AC_SUBST(TODAY)

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
	DEBUG_CFLAGS="-DDEBR_DEBUG"
else
	DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)


dnl Gettext stuff.
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.11])

dnl check for libgebr
PKG_CHECK_MODULES(LIBGEBR, [gebr = $VERSION],,AC_MSG_ERROR([Could not find libgebr = $VERSION]))
AC_SUBST(CFLAGS)
AC_SUBST(LIBS)
dnl check for libgebr_geoxml
PKG_CHECK_MODULES(GEBR_GEOXML, [gebr-geoxml = $VERSION],,AC_MSG_ERROR([Could not find libgebr_geoxml = $VERSION]))
AC_SUBST(GEBR_GEOXML_CFLAGS)
AC_SUBST(GEBR_GEOXML_LIBS)
dnl check for libgebr_gui
PKG_CHECK_MODULES(GEBR_GUI, [gebr-gui = $VERSION],,AC_MSG_ERROR([Could not find libgebr_gui = $VERSION]))
AC_SUBST(GEBR_GUI_CFLAGS)
AC_SUBST(GEBR_GUI_LIBS)

AC_OUTPUT([
packages/rpm/debr.fedora.spec packages/rpm/debr.opensuse.spec packages/rpm/debr.mandriva.spec
data/help-template.html packages/Makefile 
Makefile data/Makefile src/Makefile src/defines.h
debr.desktop
po/Makefile.in
doc/Makefile doc/userdoc/Makefile
tests/Makefile
])

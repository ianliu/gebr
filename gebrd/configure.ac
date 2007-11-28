# GêBR Daemon - Process and control execution of flows
# Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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
VERSION=0.9.2pre`date +"%Y%m%d"`
TODAY=$(LANG=C date +"%a, %d %b %Y %T %z")
FLOW_VERSION=0.1.0
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)
AC_SUBST(FLOW_VERSION)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
PACKAGE_NAME=$PACKAGE
PACKAGE_VERSION=$VERSION

AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_HEADER_STDC

dnl gettext stuff
dnl GETTEXT_PACKAGE=$PACKAGE
dnl AC_SUBST(GETTEXT_PACKAGE)
dnl AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
dnl ALL_LINGUAS=""
dnl AM_GLIB_GNU_GETTEXT

dnl check for GLIB >= 2.8 (which is thread safe)
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.8],,AC_MSG_ERROR([Could not find glib >= 2.8]))
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

dnl check for GTHREAD >= 2.8
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0 >= 2.8],,AC_MSG_ERROR([Could not find gthread >= 2.8]))
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)

dnl check for libgebr_geoxml
PKG_CHECK_MODULES(LIBGEBR_GEOXML, gebr-geoxml,,AC_MSG_ERROR([Could not find libgebr_geoxml]))
AC_SUBST(LIBGEBR_GEOXML_CFLAGS)
AC_SUBST(LIBGEBR_GEOXML_LIBS)

dnl check for libgebr_misc
PKG_CHECK_MODULES(LIBGEBR_MISC, gebr-misc,,AC_MSG_ERROR([Could not find libgebr_misc]))
AC_SUBST(LIBGEBR_MISC_CFLAGS)
AC_SUBST(LIBGEBR_MISC_LIBS)

AC_OUTPUT([
debian/changelog
rpm/gebrd.fedora.spec rpm/gebrd.opensuse.spec rpm/gebrd.mandriva.spec
Makefile src/Makefile test/Makefile
])

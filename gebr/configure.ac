# GÍBR - An environment for seismic processing.
# Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

AC_PROG_INSTALL
AC_PROG_CC

dnl check for GTK
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0.0,,AC_MSG_ERROR([Could not find gtk+-2.0 >= 2.0.0]))
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

dnl check for libgebr
PKG_CHECK_MODULES(LIBGEBR, gebr,,AC_MSG_ERROR([Could not find libgebr]))
AC_SUBST(LIBGEBR_CFLAGS)
AC_SUBST(LIBGEBR_LIBS)

AC_OUTPUT([
debian/changelog
rpm/gebr.fedora.spec rpm/gebr.opensuse.spec rpm/gebr.mandriva.spec
Makefile data/Makefile menus/Makefile pixmaps/Makefile src/Makefile
])

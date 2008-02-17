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
VERSION=0.9.4pre`date +"%Y%m%d"`
TODAY=$(LANG=C date +"%a, %d %b %Y %T %z")
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(TODAY)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
PACKAGE_NAME=$PACKAGE
PACKAGE_VERSION=$VERSION

AC_CONFIG_SUBDIRS([libgebr gebrd gebr gebrme])

AC_OUTPUT([
Makefile
])

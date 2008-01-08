# libgebr - GÍBR Library
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

SUBDIRS = geoxml comm gui misc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr.pc
EXTRADIST = ${pkgconfig_DATA}

lib_LTLIBRARIES = libgebr.la
libgebr_la_SOURCES = libgebr.c

libgebrincludedir = $(includedir)/libgebr
libgebrinclude_HEADERS = libgebr.h

INCLUDES = -Igeoxml -Icomm -Igui -Imisc
AM_CFLAGS = -Wall $(GTK_CFLAGS) $(GDOME2_CFLAGS)
libgebr_la_LIBADD = geoxml/geoxml/libgebr_geoxml.la comm/comm/libgebr_comm.la gui/gui/libgebr_gui.la misc/misc/libgebr_misc.la
libgebr_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@

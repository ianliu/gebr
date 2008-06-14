# libgebr - G�BR Library
# Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

lib_LTLIBRARIES = libgebr_geoxml.la
libgebr_geoxml_la_SOURCES = \
	xml.c error.c object.c \
	sequence.c value_sequence.c \
	enum_option.c program.c \
	parameters.c parameter.c program_parameter.c parameter_group.c \
	document.c flow.c line.c project.c

libgebr_geoxmlsubincludedir=$(includedir)/libgebr/geoxml
libgebr_geoxmlsubinclude_HEADERS = \
	defines.h macros.h error.h object.h \
	sequence.h value_sequence.h \
	enum_option.h program.h \
	parameters.h parameter.h program_parameter.h parameter_group.h \
	document.h flow.h line.h project.h
noinst_HEADERS = \
	xml.h document_p.h program_p.h \
	parameter_p.h program_parameter_p.h parameters_p.h \
	types.h support.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES =
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GLIB_CFLAGS) $(GDOME2_CFLAGS)
libgebr_geoxml_la_LIBADD = $(GLIB_LIBS) $(GDOME2_LIBS)
libgebr_geoxml_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@

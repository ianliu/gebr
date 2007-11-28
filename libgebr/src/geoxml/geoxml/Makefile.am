# libgeoxml - An interface to describe seismic software in XML
# Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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
libgebr_geoxml_la_SOURCES=					\
	xml.c sequence.c				\
 	category.c program.c program_parameter.c	\
 	document.c flow.c line.c project.c

libgebr_geoxmlsubincludedir=$(includedir)/libgebr/geoxml
libgebr_geoxmlsubinclude_HEADERS=			\
	error.h macros.h sequence.h			\
	category.h program.h program_parameter.h	\
	document.h flow.h line.h project.h
noinst_HEADERS =					\
	xml.h document_p.h program_p.h			\
	types.h sequence_p.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES = -DGEOXML_DTDDIR=\"$(datadir)/libgebr/geoxml\"
AM_CFLAGS = -Wall $(GLIB_CFLAGS) $(GDOME2_CFLAGS)
libgebr_geoxml_la_LIBADD = $(GLIB_LIBS) $(GDOME2_LIBS)
libgebr_geoxml_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@

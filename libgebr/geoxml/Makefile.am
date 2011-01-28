# libgebr - GeBR Library
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
include $(top_srcdir)/Makefile.decl

if ENABLE_TESTS
TESTSDIR=tests
endif
SUBDIRS = . data validate upgrade menu-diff menu-edit menu-query flow-dump ${TESTSDIR}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr-geoxml.pc

man_MANS = gebr-geoxml.1
EXTRA_DIST =			\
	gebr-geoxml.pc		\
	gebr-geoxml.1

libgebr_geoxmlincludedir=$(includedir)/libgebr
libgebr_geoxmlinclude_HEADERS=geoxml.h

lib_LTLIBRARIES = libgebr_geoxml.la
libgebr_geoxml_la_SOURCES =	\
	clipboard.c		\
	document.c		\
	enum_option.c		\
	error.c			\
	flow.c			\
	gebr-geoxml-validate.c	\
	gebr-geoxml-tmpl.c	\
	line.c			\
	object.c		\
	parameter.c		\
	parameter_group.c	\
	parameters.c		\
	program-parameter.c	\
	program.c		\
	project.c		\
	sequence.c		\
	value_sequence.c	\
	xml.c			\
	$(NULL)

libgebr_geoxmlsubincludedir=$(includedir)/libgebr/geoxml
libgebr_geoxmlsubinclude_HEADERS =	\
	clipboard.h			\
	document.h			\
	enum_option.h			\
	error.h				\
	flow.h				\
	gebr-geoxml-validate.h		\
	gebr-geoxml-tmpl.h		\
	line.h				\
	macros.h			\
	object.h			\
	parameter.h			\
	parameter_group.h 		\
	parameters.h			\
	program-parameter.h		\
	program.h 			\
	project.h			\
	sequence.h			\
	value_sequence.h		\
	$(NULL)

noinst_HEADERS =		\
	document_p.h		\
	parameter_group_p.h	\
	parameter_p.h		\
	parameters_p.h		\
	program_p.h		\
	program_parameter_p.h	\
	sequence_p.h		\
	types.h			\
	xml.h			\
	$(NULL)

AM_CPPFLAGS = -I$(top_srcdir)		\
	      -I$(top_srcdir)/libgebr	\
	      -I..			\
	      $(GLIB_CFLAGS)		\
	      $(GDOME2_CFLAGS)		\
	      @DEBUG_CFLAGS@

AM_CFLAGS = $(COMMON_CFLAGS) -fPIC
libgebr_geoxml_la_LIBADD = ../libgebr.la $(GLIB_LIBS) $(GDOME2_LIBS)
libgebr_geoxml_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@

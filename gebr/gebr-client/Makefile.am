# GeBR - An environment for seismic processing.
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

bin_PROGRAMS = gebr-client

gebr_client_SOURCES = main.c \
	gebrclient.c \
	server.c client.c
noinst_HEADERS = \
	gebrclient.h \
	server.h client.h
EXTRA_DIST = ${noinst_HEADERS}

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_GEOXML_CFLAGS)	\
	      $(GEBR_COMM_CFLAGS)	\
	      @DEBUG_CFLAGS@

gebr_client_LDADD = $(GEBR_LIBS)	\
		    $(GEBR_GEOXML_LIBS)	\
		    $(GEBR_COMM_LIBS)

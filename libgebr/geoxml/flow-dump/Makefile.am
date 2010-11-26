# libgebr - G�BR Library
# Copyright (C) 2007  Br�ulio Barros de Oliveira (brauliobo@gmail.com)
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

bin_PROGRAMS = gebr-geoxml-flow-dump
gebr_geoxml_flow_dump_SOURCES = flow-dump.c

AM_CFLAGS = $(COMMON_CFLAGS)
AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_GEOXML_CFLAGS)	\
	      -I$(srcdir)/..

gebr_geoxml_flow_dump_LDADD = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)

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

bin_PROGRAMS = gebr-geoxml-menu-diff
gebr_geoxml_menu_diff_SOURCES = menu-diff.c

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../.. 
AM_CFLAGS = -Wall $(GLIB_CFLAGS) $(GDOME2_CFLAGS)
gebr_geoxml_menu_diff_LDADD = ../geoxml/libgebr_geoxml.la ../../libgebr.la $(GLIB_LIBS)

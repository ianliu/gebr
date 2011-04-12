# libgebr - GêBR Library
# Copyright (C) 2011 Ian Liu Rodrigues <ian.liu@gebrproject.com>
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

bin_PROGRAMS = 			\
	gebr-geoxml-flow-dump	\
	gebr-geoxml-menu-diff	\
	gebr-geoxml-menu-edit	\
	gebr-geoxml-menu-query	\
	gebr-geoxml-upgrade	\
	gebr-geoxml-validate	\
	$(NULL)

AM_CFLAGS = $(COMMON_CFLAGS)
AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_GEOXML_CFLAGS)	\
	      -I$(srcdir)/../geoxml

gebr_geoxml_flow_dump_SOURCES  = flow-dump.c
gebr_geoxml_menu_diff_SOURCES  = menu-diff.c
gebr_geoxml_menu_edit_SOURCES  = menu-edit.c
gebr_geoxml_menu_query_SOURCES = menu-query.c
gebr_geoxml_upgrade_SOURCES    = upgrade.c
gebr_geoxml_validate_SOURCES   = validate.c

gebr_geoxml_flow_dump_LDADD  = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)
gebr_geoxml_menu_diff_LDADD  = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)
gebr_geoxml_menu_edit_LDADD  = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)
gebr_geoxml_menu_query_LDADD = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)
gebr_geoxml_upgrade_LDADD    = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)
gebr_geoxml_validate_LDADD   = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)

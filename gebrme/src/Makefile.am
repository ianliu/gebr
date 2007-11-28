# GêBR ME - GêBR Menu Editor
# Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

bin_PROGRAMS = gebrme

gebrme_SOURCES =	main.c \
			interface.c \
			callbacks.c \
			gebrme.c \
			menu.c \
			info.c \
			category.c \
			program.c \
			parameter.c \
			help.c \
			preferences.c \
			support.c
noinst_HEADERS =	interface.h \
			callbacks.h \
			gebrme.h \
			menu.h \
			info.h \
			category.h \
			program.h \
			parameter.h \
			help.h \
			preferences.h \
			support.h

GEBRME_PIXMAPS_DIR = $(datadir)/gebrme/pixmaps/

EXTRA_DIST = $(noinst_HEADERS)
INCLUDES =	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
		-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
		-DGEBRME_DATA_DIR=\"$(datadir)/gebrme/\" \
		-DGEBRME_PIXMAPS_DIR=\""$(GEBRME_PIXMAPS_DIR)"\"
AM_CFLAGS = -Wall $(GTK_CFLAGS) $(LIBGEBR_GEOXML_CFLAGS)
gebrme_LDADD = $(INTLLIBS) $(GTK_LIBS) $(LIBGEBR_GEOXML_LIBS)

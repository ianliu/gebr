# G�BR - An environment for seismic processing.
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

bin_PROGRAMS = gebr-client

gebr_client_SOURCES = main.c \
	server.c client.c
noinst_HEADERS = \
	server.h client.h \
	support.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GLIB_CFLAGS) $(LIBGEBR_COMM_CFLAGS)
gebr_client_LDADD = $(GLIB_LIBS) $(LIBGEBR_COMM_LIBS)

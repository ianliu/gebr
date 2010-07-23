# libgebr - GeBR Library
#  Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

SUBDIRS = @JSONGLIB_SUBDIR@

lib_LTLIBRARIES = libgebr_comm.la
libgebr_comm_la_SOURCES = \
	socketaddress.c hostinfo.c socket.c \
	streamsocket.c listensocket.c channelsocket.c \
	process.c terminalprocess.c \
	protocol.c server.c

libgebr_commsubincludedir = $(includedir)/libgebr/comm
libgebr_commsubinclude_HEADERS = \
	socketaddress.h hostinfo.h socket.h \
	streamsocket.h listensocket.h channelsocket.h \
	process.h terminalprocess.h \
	protocol.h server.h
noinst_HEADERS = \
	socketaddressprivate.h socketprivate.h streamsocketprivate.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES = -I$(srcdir)/../.. -I$(srcdir)/../../geoxml -I .
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GTHREAD_CFLAGS) $(JSONGLIB_CFLAGS)
libgebr_comm_la_LIBADD = -lutil ../../libgebr.la ../../geoxml/geoxml/libgebr_geoxml.la \
	$(INTLLIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(GTHREAD_LIBS) $(JSONGLIB_LIBS) @JSONGLIB_LDADD@
libgebr_comm_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@

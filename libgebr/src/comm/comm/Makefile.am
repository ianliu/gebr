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

SUBDIRS = @JSONGLIB_SUBDIR@

lib_LTLIBRARIES = libgebr_comm.la
libgebr_comm_la_SOURCES = \
	gsocketaddress.c ghostinfo.c gsocket.c \
	gstreamsocket.c glistensocket.c gchannelsocket.c \
	gprocess.c gterminalprocess.c \
	protocol.c server.c

libgebr_commsubincludedir = $(includedir)/libgebr/comm
libgebr_commsubinclude_HEADERS = \
	gsocketaddress.h ghostinfo.h gsocket.h \
	gstreamsocket.h glistensocket.h gchannelsocket.h \
	gprocess.h gterminalprocess.h \
	protocol.h server.h
noinst_HEADERS = \
	gsocketaddressprivate.h gsocketprivate.h gstreamsocketprivate.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES = -I../.. -I../../geoxml -I .
AM_CFLAGS = -Wall -std=gnu99 -fPIC @DEBUG_CFLAGS@ $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GTHREAD_CFLAGS) $(JSONGLIB_CFLAGS)
libgebr_comm_la_LIBADD = -lutil $(INTLLIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(GTHREAD_LIBS) $(JSONGLIB_LIBS) \
	../../libgebr.la \
	../../geoxml/geoxml/libgebr_geoxml.la \
	@JSONGLIB_LDADD@
libgebr_comm_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@

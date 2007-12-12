# libgebr - GêBR Library
# Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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

lib_LTLIBRARIES = libgebr_comm.la
libgebr_comm_la_SOURCES = \
	ghostaddress.c ghostinfo.c gsocket.c gtcpsocket.c gtcpserver.c \
	gprocess.c \
	protocol.c ssh.c

libgebr_commsubincludedir = $(includedir)/libgebr/comm
libgebr_commsubinclude_HEADERS = \
	ghostaddress.h ghostinfo.h gsocket.h gtcpsocket.h gtcpserver.h \
	gprocess.h \
	protocol.h ssh.h
noinst_HEADERS = \
	gsocketprivate.h gtcpsocketprivate.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES =
AM_CFLAGS = -Wall $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GTHREAD_CFLAGS)
libgebr_comm_la_LIBADD = $(GLIB_LIBS) $(GOBJECT_LIBS) $(GTHREAD_LIBS)
libgebr_comm_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@

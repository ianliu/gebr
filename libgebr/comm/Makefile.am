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

if ENABLE_TESTS
TESTDIR=tests
endif
SUBDIRS = . socketchannel ${TESTDIR}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr-comm.pc

EXTRA_DIST = gebr-comm.pc

libcommincludedir=$(includedir)/libgebr
libcomminclude_HEADERS=gebr-comm.h

touch:
	if [ -d $(top_srcdir)/.hg ]; \
	then \
	  touch $(srcdir)/gebr-comm-protocol.c -r $(top_srcdir)/.hg; \
	fi

gebr-comm-protocol.c: touch

lib_LTLIBRARIES = libgebr_comm.la
libgebr_comm_la_SOURCES = \
	gebr-comm-channelsocket.c	\
	gebr-comm-hostinfo.c		\
	gebr-comm-http-msg.c 		\
	gebr-comm-job.c			\
	gebr-comm-json-content.c	\
	gebr-comm-listensocket.c	\
	gebr-comm-process.c		\
	gebr-comm-protocol-socket.c	\
	gebr-comm-protocol.c		\
	gebr-comm-runner.c		\
	gebr-comm-server.c		\
	gebr-comm-socket.c		\
	gebr-comm-socketaddress.c	\
	gebr-comm-streamsocket.c	\
	gebr-comm-terminalprocess.c	\
	gebr-comm-uri.c			\
	$(NULL)

libgebr_commsubincludedir = $(includedir)/libgebr/comm
libgebr_commsubinclude_HEADERS = \
	gebr-comm-channelsocket.h	\
	gebr-comm-hostinfo.h		\
	gebr-comm-http-msg.h 		\
	gebr-comm-job.h			\
	gebr-comm-json-content.h	\
	gebr-comm-listensocket.h	\
	gebr-comm-process.h		\
	gebr-comm-protocol-socket.h	\
	gebr-comm-protocol.h		\
	gebr-comm-runner.h		\
	gebr-comm-server.h		\
	gebr-comm-socket.h		\
	gebr-comm-socketaddress.h	\
	gebr-comm-streamsocket.h	\
	gebr-comm-terminalprocess.h	\
	gebr-comm-uri.h			\
	$(NULL)

noinst_HEADERS = \
	gebr-comm-protocol_p.h			\
	gebr-comm-socketaddressprivate.h	\
	gebr-comm-socketprivate.h		\
	gebr-comm-streamsocketprivate.h

AM_CPPFLAGS =			\
	$(GLIB_CFLAGS)		\
	$(GEBR_CFLAGS)		\
	$(GEBR_JSON_CFLAGS)	\
	$(GEBR_GEOXML_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	-DNANOVERSION=\"$(NANOVERSION)\" \
	$(NULL)

AM_CFLAGS = $(COMMON_CFLAGS) -fPIC

libgebr_comm_la_LIBADD =		\
	-lutil				\
	$(GLIB_LIBS)			\
	$(GDOME2_LIBS)			\
	$(GEBR_LIBS)			\
	$(GEBR_JSON_LIBS)		\
	$(GEBR_GEOXML_LIBS)		\
	$(NULL)

libgebr_comm_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@

# GeBR Daemon - Process and control execution of flows
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

SUBDIRS = . po doc tests

noinst_LTLIBRARIES = libgebrd.la
libgebrd_la_SOURCES =			\
	gebrd-client.c			\
	gebrd-client.h			\
	gebrd-gettext.h			\
	gebrd-job.c			\
	gebrd-job.h			\
	gebrd-mpi-implementations.c	\
	gebrd-mpi-implementations.h	\
	gebrd-mpi-interface.c		\
	gebrd-mpi-interface.h		\
	gebrd-server.c			\
	gebrd-server.h			\
	gebrd-sysinfo.c			\
	gebrd-sysinfo.h			\
	gebrd-user.c			\
	gebrd-user.h			\
	gebrd.c				\
	gebrd.h				\
	$(NULL)

libgebrd_la_LIBADD =		\
	$(GLIB_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(NULL)

touch:
	if [ -d $(top_srcdir)/.hg ]; then touch $(srcdir)/gebrd-main.c -r $(top_srcdir)/.hg; fi
gebrd.c: touch

bin_PROGRAMS = gebrd
gebrd_SOURCES = \
	gebrd-main.c 

AM_CPPFLAGS =			\
	$(GLIB_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_JSON_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	-DNANOVERSION=\"$(NANOVERSION)\"\
	@DEBUG_CFLAGS@		\
	$(NULL)

AM_CFLAGS = $(COMMON_CFLAGS)

gebrd_LDADD =			\
	$(GLIB_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(builddir)/libgebrd.la	\
	$(NULL)

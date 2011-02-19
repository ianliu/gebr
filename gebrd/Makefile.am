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

noinst_LIBRARIES = libgebrd.a
libgebrd_a_SOURCES =			\
	gebrd-client.c			\
	gebrd-sysinfo.c			\
	gebrd.c				\
	gebrd-job-queue.c		\
	gebrd-job.c			\
	gebrd-mpi-implementations.c	\
	gebrd-mpi-interface.c		\
	gebrd-queues.c			\
	gebrd-server.c			\
	$(NULL)

touch:
	if [ -d $(top_srcdir)/.hg ]; then touch gebrd-main.c -r $(top_srcdir)/.hg; fi
gebrd.c: touch

bin_PROGRAMS = gebrd
gebrd_SOURCES = \
	gebrd-main.c 

noinst_HEADERS =			\
	gebrd-client.h			\
	gebrd-sysinfo.h			\
	gebrd.h				\
	gebrd-job-queue.h		\
	gebrd-job.h			\
	gebrd-mpi-implementations.h	\
	gebrd-mpi-interface.h		\
	gebrd-queues.h			\
	gebrd-server.h			\
	$(NULL)

EXTRA_DIST = $(noinst_HEADERS)

AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_COMM_CFLAGS)	\
	      $(GEBR_GEOXML_CFLAGS)	\
	      $(GEBR_JSON_CFLAGS)	\
	      -DNANOVERSION=\"$(NANOVERSION)\"\
	      @DEBUG_CFLAGS@ 

AM_CFLAGS = $(COMMON_CFLAGS)

gebrd_LDADD = libgebrd.a		\
	      $(GEBR_LIBS)		\
	      $(GEBR_COMM_LIBS)		\
	      $(GEBR_GEOXML_LIBS)	\
	      $(GEBR_JSON_LIBS)		\
	      $(NULL)

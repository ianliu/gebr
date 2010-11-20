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
libgebrd_a_SOURCES = \
	gebrd.c \
	server.c \
	client.c \
	job.c \
	job-queue.c \
	queues.c \
	mpi-implementations.c \
	mpi-interface.c

bin_PROGRAMS = gebrd
gebrd_SOURCES = \
	main.c 

noinst_HEADERS = \
	gebrd.h \
	server.h \
	client.h \
	job.h \
	macros.h \
	job-queue.h \
	queues.h \
	mpi-implementations.h \
	mpi-interface.h

EXTRA_DIST = $(noinst_HEADERS)

AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_COMM_CFLAGS)	\
	      $(GEBR_GEOXML_CFLAGS)	\
	      -DNANOVERSION=\"`hg parent --template '+{latesttagdistance}-{node|short}' 2> /dev/null || date ++%H%M`\" \
	      @DEBUG_CFLAGS@ 

AM_CFLAGS = -Wall -std=gnu99

gebrd_LDADD = libgebrd.a	\
	      $(INTLLIBS)	\
	      $(GEBR_LIBS)	\
	      $(GEBR_COMM_LIBS)	\
	      $(GEBR_GEOXML_LIBS)

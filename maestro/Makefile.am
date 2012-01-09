# GeBR - An environment for seismic processing.
# Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
EXTRADIR =

AM_CFLAGS = $(COMMON_CFLAGS)
AM_CPPFLAGS =			\
	$(GUI_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GLIB_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	$(GEBR_GUI_CFLAGS)	\
	$(GEBR_JSON_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	-DNANOVERSION=\"$(NANOVERSION)\" \
	$(NULL)

bin_PROGRAMS = gebrm
gebrm_SOURCES = gebrm-main.c
gebrm_LDADD =			\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GLIB_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	libmaestro.la		\
	$(NULL)

noinst_LTLIBRARIES = libmaestro.la

libmaestro_la_SOURCES =        \
	gebrm-app.c	       \
	gebrm-app.h	       \
	gebrm-client.c	       \
	gebrm-client.h	       \
	gebrm-daemon.c	       \
	gebrm-daemon.h	       \
	gebrm-job-controller.c \
	gebrm-job-controller.h \
	gebrm-job.c	       \
	gebrm-job.h	       \
	gebrm-marshal.c        \
	gebrm-marshal.h        \
	gebrm-task.c	       \
	gebrm-task.h	       \
	$(NULL)

touch:
	if [ -d $(top_srcdir)/.hg ]; then touch $(srcdir)/gebrm-main.c -r $(top_srcdir)/.hg; fi
gebrm-client.c: touch

libmaestro_la_LIBADD =		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GLIB_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	$(NULL)

# glib-genmarshal rules
glib_marshal_list = gebrm-marshal.list
glib_marshal_prefix = gebrm_cclosure_marshal
include $(top_srcdir)/Makefile.am.marshal

EXTRA_DIST += gebrm-marshal.list

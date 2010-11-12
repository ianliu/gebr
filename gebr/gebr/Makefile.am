# GeBR - An environment for seismic processing.
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

NULL =

bin_PROGRAMS = gebr

#TODO: remove cmdline.{c,h} in future versions (remove old config format support)
noinst_LIBRARIES = libgebr.a
libgebr_a_SOURCES =		\
	callbacks.c		\
	client.c		\
	cmdline.c		\
	document.c		\
	flow.c			\
	gebr-help-edit-widget.c	\
	gebr.c			\
	interface.c		\
	job.c			\
	line.c			\
	menu.c			\
	project.c		\
	server.c		\
	ui_document.c		\
	ui_flow.c		\
	ui_flow_browse.c	\
	ui_flow_edition.c	\
	ui_help.c		\
	ui_job_control.c	\
	ui_log.c		\
	ui_moab.c		\
	ui_parameters.c		\
	ui_paths.c		\
	ui_preferences.c	\
	ui_project_line.c	\
	ui_server.c		\
	$(NULL)

gebr_SOURCES = main.c
noinst_HEADERS = 		\
	callbacks.h		\
	client.h		\
	cmdline.h		\
	document.h		\
	flow.h			\
	gebr-help-edit-widget.h	\
	gebr.h			\
	interface.h		\
	job.h			\
	line.h			\
	menu.h			\
	project.h		\
	server.h		\
	ui_document.h		\
	ui_flow.h		\
	ui_flow_browse.h	\
	ui_flow_edition.h	\
	ui_help.h		\
	ui_job_control.h	\
	ui_log.h		\
	ui_moab.h		\
	ui_parameters.h		\
	ui_paths.h		\
	ui_preferences.h	\
	ui_project_line.h	\
	ui_server.h		\
	$(NULL)

EXTRA_DIST = ${noinst_HEADERS}

AM_CPPFLAGS =			\
	-I..			\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	$(GEBR_GUI_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	$(NULL)

AM_CFLAGS = -Wall -std=gnu99

gebr_LDADD =			\
	libgebr.a		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(GEBR_GUI_LIBS)	\
	$(NULL)

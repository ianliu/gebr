# GeBR - An environment for seismic processing.
# Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

bin_PROGRAMS = gebr

#TODO: remove cmdline.{c,h} in future versions (remove old config format support)

gebr_SOURCES = main.c \
	cmdline.c \
	gebr.c \
	callbacks.c \
	document.c \
	project.c \
	line.c \
	flow.c \
	job.c \
	server.c client.c \
	menu.c \
	interface.c \
	ui_document.c \
	ui_flow.c \
	ui_parameters.c \
	ui_help.c \
	ui_project_line.c \
	ui_flow_browse.c \
	ui_flow_edition.c \
	ui_log.c \
	ui_job_control.c \
	ui_paths.c \
	ui_preferences.c \
	ui_server.c
noinst_HEADERS = \
	cmdline.h \
	gebr.h \
	callbacks.h \
	document.h \
	project.h \
	line.h \
	flow.h \
	job.h \
	server.h client.h \
	menu.h \
	interface.h \
	ui_document.h \
	ui_flow.h \
	ui_parameters.h \
	ui_help.h \
	ui_project_line.h \
	ui_flow_browse.h \
	ui_flow_edition.h \
	ui_job_control.h \
	ui_log.h \
	ui_paths.h \
	ui_preferences.h \
	ui_server.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GTK_CFLAGS) $(LIBGEBR_CFLAGS) $(LIBGEBR_MISC_CFLAGS) $(LIBGEBR_GEOXML_CFLAGS) $(LIBGEBR_COMM_CFLAGS) $(LIBGEBR_GUI_CFLAGS) $(WEBKIT_CFLAGS) $(WEBKIT_ENABLED)
gebr_LDADD = $(GTK_LIBS) $(LIBGEBR_LIBS) $(LIBGEBR_MISC_LIBS) $(LIBGEBR_GEOXML_LIBS) $(LIBGEBR_COMM_LIBS) $(LIBGEBR_GUI_LIBS) $(WEBKIT_LIBS)

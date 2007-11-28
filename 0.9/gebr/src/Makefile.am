# GÍBR - An environment for seismic processing.
# Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#TODO: check for gengetopt at configure script
cmdline.h: cmdline.c
cmdline.c: gebr.ggo
	gengetopt < gebr.ggo --file-name=cmdline -C -a ggopt

gebr_SOURCES =	main.c \
		cmdline.c \
		gebr.c \
		callbacks.c \
		cb_proj.c \
		cb_line.c \
		cb_flow.c \
		cb_flowcomp.c \
		cb_help.c \
		cb_job.c \
		cb_prop.c \
		cb_server.c \
		cb_job.h \
		ui_help.c \
		ui_menubar.c \
		ui_pages.c \
		ui_prop.c \
		ui_server.c \
		interface.c \
		menus.c \
		server.c client.c \
		parse.c
noinst_HEADERS = cmdline.h \
		gebr.h \
		callbacks.h \
		cb_proj.h \
		cb_line.h \
		cb_flow.h \
		cb_flowcomp.h \
		cb_help.h \
		cb_job.h \
		cb_prop.h \
		cb_server.h \
		cb_job.h \
		ui_help.h \
		ui_menubar.h \
		ui_pages.h \
		ui_prop.h \
		ui_server.h \
		interface.h \
		menus.h \
		parse.h \
		server.h client.h \
		widgets.h
EXTRA_DIST = gebr.ggo

GEBR_PIXMAPS_DIR = $(datadir)/gebr/pixmaps/
GEBR_MENUSYS_DIR = $(datadir)/gebr/menus/
GEBR_DATA_DIR = $(datadir)/gebr/

INCLUDES =	-D GEBRMENUSYS=\"$(GEBR_MENUSYS_DIR)\" \
		-D GEBR_PIXMAPS_DIR=\"$(GEBR_PIXMAPS_DIR)\" \
		-D GEBRDATADIR=\"$(GEBR_DATA_DIR)\"
AM_CFLAGS = -Wall $(GTK_CFLAGS) $(LIBGEBR_CFLAGS)
gebr_LDADD = $(GTK_LIBS) $(LIBGEBR_LIBS)



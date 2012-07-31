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

gebrmenudir = $(DISTDIR)$(datadir)/gebr/menus/Default
gebrmenu_DATA = grace.mnu sleep.mnu forloop.mnu

desktopdir = $(DISTDIR)$(datadir)/applications
desktop_DATA = gebr.desktop

pythondir = $(DISTDIR)$(datadir)/gebr/python
python_DATA = \
	gebr-xdot-graph.py \
	xdot.py \
	$(NULL)

gladedir = $(DISTDIR)$(datadir)/gebr/glade
glade_DATA = \
	gebr-job-control.glade \
	gebr-maestro-dialog.glade \
	document-properties.glade \
	mpi-parameters-help.glade \
	project-properties.glade \
	line-properties.glade \
	connect-maestro-dialog.glade \
	gebr-connections-settings.glade \
	flow-properties.glade \
	snapshot-revert.glade\
	l.png \
	$(NULL)

gebr.desktop:gebr.desktop.in $(INTLTOOL_MERGE) $(srcdir)/../po/*.po
	LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(builddir)/../po/.intltool-merge-cache $(srcdir)/../po $< $@

all:
	perl -i -pe 's/&#xEA;/ê/' *.glade

EXTRA_DIST = gebr.desktop.in $(gebrmenu_DATA) $(glade_DATA) $(python_DATA)
CLEANFILES = gebr.desktop

install-data-hook:
	$(top_builddir)/libgebr/geoxml-utils/gebr-geoxml-menu-indices $(gebrmenudir) $(gebrmenudir)/menus.idx2   $(gebrmenudir)/categories.idx2
	
uninstall-hook:
	( cd $(gebrmenudir) && rm -f *.idx2 )

# DeBR - GeBR Designer
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

debrdatadir = $(datadir)/debr/
debrdata_DATA = help-template.html \
		help-template-content.html

desktopdir = $(datadir)/applications
desktop_DATA = debr.desktop

debr.desktop:debr.desktop.in $(INTLTOOL_MERGE) $(srcdir)/../po/*.po
	LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(builddir)/../po/.intltool-merge-cache $(srcdir)/../po $< $@

EXTRA_DIST = debr.desktop.in $(debrdata_DATA)
CLEANFILES = $(desktop_DATA)

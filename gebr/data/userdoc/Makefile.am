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

HTMLDIR = $(DESTDIR)$(datadir)/doc/gebr/html

html:
	if test -w $(srcdir); \
	  (cd $(srcdir); \
	   test -d html && rm -r html; \
	   db2html gebr-pt.docbook -o html); \
	fi

dist-hook: html
	cp -Rp $(srcdir)/html $(distdir)

install-data-local: html
	${INSTALL_SCRIPT} -d $(HTMLDIR)
	${INSTALL_SCRIPT} -d $(HTMLDIR)/images
	${INSTALL_DATA} $(srcdir)/html/*.html $(HTMLDIR)
	${INSTALL_DATA} $(srcdir)/images/* $(HTMLDIR)/images

uninstall-hook:
	( cd $(DESTDIR)$(datadir)/doc && rm -r gebr )

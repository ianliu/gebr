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

HTMLDIR = $(DESTDIR)$(datadir)/doc/gebr
LANG = en

if HAVE_DOCBOOK
HTML_DEP = html-ok
DIST_DEP = dist-ok
INST_DEP = inst-ok
UINST_DEP = uinst-ok
else
HTML_DEP = html-error
DIST_DEP = dist-error
INST_DEP =
UINST_DEP =
endif

html-ok: gebr.xml
	xsltproc -o $(srcdir)/html/index.html \
		$(srcdir)/../gebr-guide.xsl \
		$(srcdir)/gebr.xml && touch html-ok

dist-ok: html-ok
	cp -Rp $(srcdir)/html $(srcdir)/images $(distdir)

inst-ok: html-ok
	${INSTALL_SCRIPT} -d $(HTMLDIR)/$(LANG)/html
	${INSTALL_SCRIPT} -d $(HTMLDIR)/$(LANG)/images
	${INSTALL_DATA} $(srcdir)/html/* $(HTMLDIR)/$(LANG)/html
	${INSTALL_DATA} $(srcdir)/images/* $(HTMLDIR)/$(LANG)/images

uinst-ok:
	( cd $(HTMLDIR) && rm -r $(LANG) )

html-error:
	echo "***"
	echo "*** Warning: GeBR Userdoc not built"
	echo "*** Warning: Check whether you have Docbook installed"
	echo "***"

dist-error:
	echo "***"
	echo "*** Warning: GeBR Userdoc not built"
	echo "*** Warning: Check whether you have Docbook installed"
	echo "*** DISTRIBUTION IS INCOMPLETE"
	echo "***"

html: $(HTML_DEP)

dist-hook: $(DIST_DEP)

install-data-local: $(INST_DEP)

uninstall-hook: $(UINST_DEP)

EXTRA_DIST = gebr.xml
CLEANFILES = html-ok

.PHONY: html

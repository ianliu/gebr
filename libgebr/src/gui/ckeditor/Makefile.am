# libgebr - GeBR Library
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

ckeditordirectory = $(DESTDIR)/$(datadir)/libgebr

install-data-local:
	find ckeditor -type d -exec ${INSTALL_SCRIPT} -d {} ${ckeditordirectory}/{} \;
	find ckeditor -type f -exec ${INSTALL_DATA} {} ${ckeditordirectory}/{} \;

EXTRA_DIST = ckeditor

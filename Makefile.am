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

ACLOCAL_AMFLAGS = -I m4 --install
DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

SUBDIRS = libgebr gebrd gebr debr

EXTRA_DIST = libgebr/po/Makefile.in.in	\
	     gebrd/po/Makefile.in.in	\
	     gebr/po/Makefile.in.in	\
	     debr/po/Makefile.in.in

DISTCLEANFILES = libgebr/po/Makefile.in	\
		 gebrd/po/Makefile.in	\
		 gebr/po/Makefile.in	\
		 debr/po/Makefile.in

distcheck-hook: all

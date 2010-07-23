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

SUBDIRS =

EXTRA_DIST = \
	rpm/debr.fedora.spec \
	rpm/debr.opensuse.spec \
	rpm/debr.mandriva.spec

packages: deb-package rpm-packages
rpm-packages: fedora-package opensuse-package mandriva-package

deb-package:
	cd ..; dpkg-buildpackage -kA594D681 -tc -rfakeroot; cd -

rpm-structure:
	cd ..; make dist; cd -
	mkdir -p rpm/BUILD rpm/RPMS rpm/SRPMS rpm/SOURCES && cp ../@PACKAGE@-@VERSION@.tar.gz rpm/SOURCES

fedora-package: rpm-structure
	rpmbuild -ba rpm/debr.fedora.spec

opensuse-package: rpm-structure
	rpmbuild -ba rpm/debr.opensuse.spec

mandriva-package: rpm-structure
	rpmbuild -ba rpm/debr.mandriva.spec


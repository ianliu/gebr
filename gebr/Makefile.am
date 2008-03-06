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

SUBDIRS=data menus src

EXTRA_DIST=
	rpm/gebr.fedora.spec \
	rpm/gebr.opensuse.spec \
	rpm/gebr.mandriva.spec

doc:
	NaturalDocs -i src/ -p ndproj/ -o FramedHtml doc/

packages: deb-package rpm-packages
rpm-packages: fedora-package opensuse-package mandriva-package

deb-package:
	dpkg-buildpackage -kA594D681 -tc -rfakeroot; \
	dpkg-buildpackage -kA594D681 -tc -rfakeroot -ai386

rpm-structure: dist
	mkdir -p rpm/BUILD rpm/RPMS rpm/SRPMS rpm/SOURCES && cp @PACKAGE@-@VERSION@.tar.gz rpm/SOURCES

fedora-package: rpm-structure
	rpmbuild -ba rpm/gebr.fedora.spec

opensuse-package: rpm-structure
	rpmbuild -ba rpm/gebr.opensuse.spec

mandriva-package: rpm-structure
	rpmbuild -ba rpm/gebr.mandriva.spec

.PHONY: doc deb-package rpm-structure fedora-package opensuse-package mandriva-package

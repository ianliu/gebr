# GeBR Daemon - Process and control execution of flows
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

EXTRA_DIST =
	rpm/gebrd.fedora.spec \
	rpm/gebrd.opensuse.spec \
	rpm/gebrd.mandriva.spec

packages: deb-package rpm-packages
rpm-packages: fedora-package opensuse-package mandriva-package

deb-package:
	cd ..; dpkg-buildpackage -kA594D681 -tc -rfakeroot; cd -

rpm-structure:
	cd ..; make dist; cd -
	mkdir -p rpm/BUILD rpm/RPMS rpm/SRPMS rpm/SOURCES && cp ../@PACKAGE@-@VERSION@.tar.gz rpm/SOURCES

fedora-package: rpm-structure
	rpmbuild -ba rpm/gebrd.fedora.spec

opensuse-package: rpm-structure
	rpmbuild -ba rpm/gebrd.opensuse.spec

mandriva-package: rpm-structure
	rpmbuild -ba rpm/gebrd.mandriva.spec

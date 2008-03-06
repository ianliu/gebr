# libgebr - GÍBR Library
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

SUBDIRS = src

PKG_DEBIAN = debian/changelog.in debian/copyright debian/info debian/libgebr-default.ex \
	debian/libgebr.doc-base.ex debian/postrm.ex debian/README.Debian \
	debian/compat debian/dirs debian/libgebr0.dirs debian/libgebr-dev.dirs \
	debian/manpage.1.ex debian/preinst.ex debian/rules debian/control \
	debian/docs debian/libgebr0.install debian/libgebr-dev.install \
	debian/postinst.ex debian/prerm.ex debian/shlibs.local.ex
PKG_RPM = rpm/libgebr.fedora.spec \
	rpm/libgebr.opensuse.spec \
	rpm/libgebr.mandriva.spec
EXTRA_DIST = ${PKG_DEBIAN} ${PKG_RPM}

packages: deb-package rpm-packages
rpm-packages: fedora-package opensuse-package mandriva-package

deb-package:
	dpkg-buildpackage -kA594D681 -tc -rfakeroot; \
	dpkg-buildpackage -kA594D681 -tc -rfakeroot -ai386

rpm-structure: dist
	mkdir -p rpm/BUILD rpm/RPMS rpm/SRPMS rpm/SOURCES && cp @PACKAGE@-@VERSION@.tar.gz rpm/SOURCES

fedora-package: rpm-structure
	rpmbuild -ba rpm/libgebr.fedora.spec

opensuse-package: rpm-structure
	rpmbuild -ba rpm/libgebr.opensuse.spec

mandriva-package: rpm-structure
	rpmbuild -ba rpm/libgebr.mandriva.spec

rpm-package: fedora-package opensuse-package mandriva-package

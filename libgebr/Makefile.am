# libgebr - GÍBR Library
# Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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

PKG_DEBIAN = changelog.in copyright info libgebr-default.ex libgebr.doc-base.ex postrm.ex README.Debian \
	compat dirs libgebr0.dirs libgebr-dev.dirs manpage.1.ex preinst.ex rules \
	control docs libgebr0.install libgebr-dev.install postinst.ex prerm.ex shlibs.local.ex
EXTRA_DIST = ${PKG_DEBIAN} \
	rpm/libgebr.fedora.spec		\
	rpm/libgebr.opensuse.spec	\
	rpm/libgebr.mandriva.spec

deb-package:
	dpkg-buildpackage -rfakeroot

rpm-structure: dist
	mkdir -p rpm/BUILD rpm/RPMS rpm/SRPMS rpm/SOURCES && cp @PACKAGE@-@VERSION@.tar.gz rpm/SOURCES

fedora-package: rpm-structure
	rpmbuild -ba rpm/libgebr.fedora.spec

opensuse-package: rpm-structure
	rpmbuild -ba rpm/libgebr.opensuse.spec

mandriva-package: rpm-structure
	rpmbuild -ba rpm/libgebr.mandriva.spec

# libgeoxml - An interface to describe seismic software in XML
# Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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

SUBDIRS = geoxml data validate doc test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr-geoxml.pc

info_TEXINFOS = gebr-geoxml.texinfo
man_MANS = gebr-geoxml.1
EXTRA_DIST =					\
	gebr-geoxml.pc				\
	gebr-geoxml.1 gebr-geoxml.texinfo

libgebr_geoxmlincludedir=$(includedir)/libgebr
libgebr_geoxmlinclude_HEADERS=geoxml.h



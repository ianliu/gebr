#!/bin/sh
# GeBR - An environment for seismic processing.
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

#dir
GEBR_DIR=`pwd`/gebr-static
rm -fr $GEBR_DIR
mkdir $GEBR_DIR

#compilation flags
export CFLAGS="-g"
# export LDFLAGS="-Wl,-rpath ."

#GeBR libray
cd libgebr
aclocal;automake;autoconf;./configure --prefix=$GEBR_DIR --enable-static-mode
cat src/geoxml/geoxml/defines.h.in | sed 's/@prefix@/../' > src/geoxml/geoxml/defines.h
# make -j5 clean install
make -j5 install
cd ..

#preparation for compiling GÃªBR's programs
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GEBR_DIR/lib/pkgconfig
export LD_LIBRARY_PATH=$GEBR_DIR/lib
export LDFLAGS="-Wl,-rpath ../lib"

#GeBR
mkdir lib gebr/lib
cd gebr
aclocal;automake;autoconf;./configure --prefix=$GEBR_DIR --enable-static-mode
cat src/defines.h.in | sed 's/@prefix@/../' | sed 's/@STATIC_MODE@/1/' > src/defines.h
#make -j5 clean install
make -j5 install
cd ..
rmdir lib gebr/lib

#GeBRD
mkdir lib gebrd/lib
cd gebrd
aclocal;automake;autoconf;./configure --prefix=$GEBR_DIR
#make -j5 clean install
make -j5 install
cd ..
rmdir lib gebrd/lib

#DeBR
mkdir lib debr/lib
cd debr
aclocal;automake;autoconf;./configure --prefix=$GEBR_DIR
cat src/defines.h.in | sed 's/@prefix@/../' > src/defines.h
#make -j5 clean install
make -j5 install
cd ..
rmdir lib debr/lib

#final adjustments

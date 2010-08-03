#!/bin/sh
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

#dir
GEBR_DIR=`pwd`/gebr-static
rm -fr $GEBR_DIR
mkdir -p $GEBR_DIR

export PKG_CONFIG_PATH=$GEBR_DIR/lib/pkgconfig:$PKG_CONFIG_PATH

#./autogen.sh

#GeBR libray
cd libgebr
./configure --prefix=$GEBR_DIR --disable-debug
perl -i -pe "s|$GEBR_DIR|..|" src/defines.h
make -j5 install
cd ..

#GeBRD
cd gebrd
./configure --prefix=$GEBR_DIR --disable-debug
make -j5 install
cd ..

#GeBR
cd gebr
./configure --prefix=$GEBR_DIR --disable-debug
perl -i -pe "s|$GEBR_DIR|..|" src/defines.h
make -j5 install
cd ..

#DeBR
cd debr
./configure --prefix=$GEBR_DIR --disable-debug
perl -i -pe "s|$GEBR_DIR|..|" src/defines.h
make -j5 install
cd ..

#final adjustments
mkdir -p $GEBR_DIR/real-bin
mv $GEBR_DIR/bin/* $GEBR_DIR/real-bin
echo '#!/bin/sh
if test -z $GEBR_HOME; then
	export GEBR_HOME=$PWD/`dirname $0`/..
	export LD_LIBRARY_PATH=$GEBR_HOME/lib:$LD_LIBRARY_PATH
	export PATH=$GEBR_HOME/bin:$PATH
fi
GEBR_PROG=`basename $0`
cd $GEBR_HOME/bin
$GEBR_HOME/real-bin/$GEBR_PROG $*' > $GEBR_DIR/bin/script
chmod +x $GEBR_DIR/bin/script
cd $GEBR_DIR/real-bin
for bin in *; do
	ln -sf script $GEBR_DIR/bin/$bin
done


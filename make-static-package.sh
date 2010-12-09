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
cd `dirname $0`
GEBR_HOME=`dirname \`pwd\``
export PATH=$GEBR_HOME/real-bin:$PATH
export LD_LIBRARY_PATH=$GEBR_HOME/lib:$LD_LIBRARY_PATH
$GEBR_HOME/real-bin/`basename $0` "$@"' > $GEBR_DIR/bin/script
chmod +x $GEBR_DIR/bin/script
cd $GEBR_DIR/real-bin
for bin in *; do
	ln -sf script $GEBR_DIR/bin/$bin
done

#libs
cd $GEBR_DIR/lib
cp -a /usr/lib/libgdome.* .
cp -a /usr/lib/libwebkit-1.0.so* .
cp -a /usr/lib/libsoup-2.4.* .
cp -a /usr/lib/libgio-2.0.* .
cp -a /usr/lib/libenchant.* .
cp -a /usr/lib/libgstapp-0.10* .
cp -a /usr/lib/libgstpbutils-0.10* .
cp -a /usr/lib/libicui18n.so* .
cp -a /usr/lib/libgnutls.so* .
cp -a /lib/libpcre.so.3* .
cp -a /usr/lib/libtasn1.* .
cp -a /usr/lib/libxml2.* .
cp -a /usr/lib/libxslt.* .
cp -a /usr/lib/libicuuc.so.* .
cp -a /usr/lib/libicudata.so.* .
cp -a /lib/libc.so.6 .
cp -a /lib/libc-2* .
cp -a /usr/lib/libgstreamer-0.10.so.0* .
#jaunty
cp -a /usr/lib/libcurl-gnutls.so.* .
cp -a /usr/lib/liblber-2.4.so* .
cp -a /usr/lib/libldap_r-2.4.so* .
cp -a /usr/lib/libsasl2.so.2* .
cp -a /usr/lib/libgthread-2.0.* .
cp -a /usr/lib/libglib-2.0.* .
#lucid

cp -a /usr/lib/libgmodule-2.0.* .
cp -a /usr/lib/libgdk-x11-2.0.* .
cp -a /usr/lib/libgtk-x11-2.0.* .
cp -a /usr/lib/libatk-1.0.* .
cp -a /usr/lib/libpangoft2-1.0.* .
cp -a /usr/lib/libpangocairo-1.0.* .
cp -a /usr/lib/libpango-1.0.* .
cp -a /usr/lib/libgdk_pixbuf-2.0.* .
cp -a /lib/libgcrypt.* .
cp -a /usr/lib/libgobject-2.0.* .
cp -a /usr/lib/libcairo.so* .
cp -a /usr/lib/libpixman-1.* .
cp -a /usr/lib/libdirectfb-1.0* .
cp -a /usr/lib/libfusion-1.0* .
cp -a /usr/lib/libdirect-1.0* .
cp -a /usr/lib/libxcb.so* .
cp -a /usr/lib/libxcb-render* .

rm *.la *.a

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

if [ "x$GEBR_DIR" = "x" ]; then
	GEBR_DIR=$HOME/.local
fi

GEBR_BINS="gebr* debr"

cd $GEBR_DIR/bin && rm -f $GEBR_BINS && cd -
cd $GEBR_DIR/lib/gebr/bin && rm -f $GEBR_BINS && cd -

./configure --prefix=$GEBR_DIR --disable-debug
# perl -i -pe "s|$GEBR_DIR|..|" `find . -name defines.h`
make -j5 install

#final adjustments
mkdir -p $GEBR_DIR/lib/gebr/bin
cd $GEBR_DIR/bin
mv $GEBR_BINS $GEBR_DIR/lib/gebr/bin
cd $GEBR_DIR/lib/gebr/bin && GEBR_SAVE=`ls $GEBR_BINS` && cd -

echo '#!/bin/sh
if test -z $GEBR_HOME; then
	dir=`dirname $0`
	if [[ "$dir" == /* ]]; then
		export GEBR_HOME=$dir/..
	else
		export GEBR_HOME=$PWD/$dir/..
	fi
	export LD_LIBRARY_PATH=$GEBR_HOME/lib:$LD_LIBRARY_PATH
	export PATH=$GEBR_HOME/bin:$PATH
fi
GEBR_PROG=`basename $0`
cd $GEBR_HOME/bin
$GEBR_HOME/lib/gebr/bin/$GEBR_PROG $*' > $GEBR_DIR/lib/gebr/bin/gebr-run.sh

chmod +x $GEBR_DIR/lib/gebr/bin/gebr-run.sh
cd $GEBR_DIR/bin
for bin in $GEBR_SAVE; do
	ln -sf ../lib/gebr/bin/gebr-run.sh $bin
done

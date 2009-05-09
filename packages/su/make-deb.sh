#!/bin/sh
touch src/LICENSE_41_ACCEPTED
touch src/MAILHOME_41
cp Makefile.config.deb src/Makefile.config
export CWPROOT=`pwd`
dpkg-buildpackage -kA594D681 -tc -rfakeroot

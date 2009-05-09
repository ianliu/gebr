#!/bin/sh
touch LICENSE_41_ACCEPTED
touch MAILHOME_41
cp Makefile.config.deb Makefile.config
dpkg-buildpackage -kA594D681 -tc -rfakeroot

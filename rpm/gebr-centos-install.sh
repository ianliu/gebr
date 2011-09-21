#!/bin/bash

# Version 0.10
# Updated gebrproject and menus versions.
# Added support to docbook and libtidy.
#
# Version 0.9
# Downloads GeBR and menus from official repository.
#
# Version 0.8
# Merged centos5 script into this one.
#
# Version 0.7
# Added getopts for the script.
# 
# Version 0.6
# Also install menus for shtools and seismicunix.
#
# Version 0.5
# Better gebr wrapper script to workaround a X freeze.
# Fixed libpng url.
# Only tidy up if not debuging.
# Bumped gebrproject version to 0.10.1.
#
# Version 0.4
# Added option to compile with debug enabled.
#
# Version 0.3
# Creates gebr-run.sh to setup the enviroment before run.
# Clean up /opt/gebrproject install dir for rpm build.

print_usage()
{
	name=`basename $0`
	echo "Usage: $name"
	echo "       $name --cores=2 --prefix=$HOME/bin"
	echo "       $name --enable-debug"
	echo
	echo " Convenience script for installing GêBR in CentOS 4 and 5. It will download"
	echo " many  dependencies which these systems  lack and compile them.  After that,"
	echo " GêBR is downloaded and installed."
	echo
	echo "  --prefix=PREFIX       The directory which GeBR will be installed. The"
	echo "                        default installation folder is \$HOME/.local for"
	echo "                        nonadministrative accounts and /opt/gebrproject"
	echo "                        for super users."
	echo
	echo "  -c, --cores=CORES     The number of cores to use in \`make install'."
	echo "                        Default value is 4."
	echo
	echo "  --enable-debug        To enable debug mode (default is --disable-debug)"
	exit 1
}

GEBR_REPO=http://gebr.googlecode.com/files/
MENU_REPO=http://gebr-menus.googlecode.com/files/
GEBR_MAJOR=0
GEBR_MINOR=12
GEBR_MICRO=0
GEBR_VERSION=$GEBR_MAJOR.$GEBR_MINOR.$GEBR_MICRO
GEBR_MENUS="gebr-menus-su-0.5.1 gebr-menus-shtools-0.2.1"

SCRIPT_NAME=`basename $0`
MANTAINER='GeBR Team <gebr-devel@googlegroups.com>'
LOGFILE='gebr-install.log'

TEMP=`getopt -o hc: --longoptions prefix:,cores:,enable-debug,help \
	-n "$SCRIPT_NAME" -- "$@"`

if [ $? != 0 ] ; then
	echo "*** Error while running getopt" >&2
	echo "*** Please report this to the script mantainer $MANTAINER." >&2
	exit 1 ;
fi

eval set -- "$TEMP"

while true ; do
	case "$1" in
		-h|--help) print_usage ;;
		-c|--cores) CORES=$2 ; shift 2 ;;
		--prefix) INSTALL_DIR=$2 ; shift 2 ;;
		--enable-debug) DEBUG=$1 ; shift ;;
		--) shift ; break ;;
		*)
			echo "*** Error while parsing getopt's string" >&2
			echo "*** Please report this to the script mantainer $MANTAINER." >&2
			exit 1 ;;
	esac
done

if [ "x`id -u`" == "x0" ]; then
	IS_ROOT=true
else
	IS_ROOT=false
fi

if test -z $INSTALL_DIR; then
	if $IS_ROOT ; then
		INSTALL_DIR=/opt/gebrproject
	else
		INSTALL_DIR=$HOME/.local
	fi
fi

test -z $DEBUG && DEBUG=--disable-debug
test -z $CORES && CORES=4
test -r /etc/redhat-release \
	&& CENTOS_MAJOR_VERSION=`cat /etc/redhat-release | awk '{ print $3 }' | awk -F. '{ print $1 }'`

echo "This script was last executed on"        >> $LOGFILE
echo "  `date \"+%A, %B %d, %Y\"`"             >> $LOGFILE
echo
echo "Installation summary:"                   >> $LOGFILE
echo "  Installation directory: $INSTALL_DIR"  >> $LOGFILE
echo "  Debugging flags:        $DEBUG"        >> $LOGFILE
echo "  Number of cores:        $CORES"        >> $LOGFILE
echo "  System detected:        CentOS $CENTOS_MAJOR_VERSION" >> $LOGFILE

common_deps="
automake
bison
bzip2
expat-devel
file
flex
gcc
gettext
gperf
gtk-doc
intltool
libjpeg-devel
libtiff-devel
make
openssh-clients
python-devel
shared-mime-info
unzip
wget
xauth
zlib-devel"

case $CENTOS_MAJOR_VERSION in
	4) specific_deps="
		gcc4
		gcc4-c++
		xorg-x11-devel" ;;

	5) specific_deps="
		gcc-c++
		libICE-devel
		libX11-devel
		libXext-devel
		libXt-devel
		xorg-x11-proto-devel
		xorg-x11-xtrans-devel" ;;

	*)
		echo "*** Error while checking your system version. The script only supports CentOS 4 and 5." >&2
		echo "*** Please contact the script mantainer $MANTAINER to get support." >&2
		exit 1 ;;
esac

deps="$common_deps $specific_deps"

MISSING_PACKAGES=`yum list $deps | grep 'Available Packages' -A 100 | sed 's/Available Packages/Please ask root to install the following packages:/'`

if [ "x$MISSING_PACKAGES" != "x" ]; then
	if $IS_ROOT ; then
		yum -y install $deps
	else
		echo "$MISSING_PACKAGES"
		exit 1
	fi
fi

test `ls /dev/pts 2> /dev/null | wc -l` == '0' \
	&& echo "*** WARNING: Could not find your /dev/pts.
If you are inside a chroot, you should add the following line into /etc/fstab
  devpts /dev/pts devpts rw,noexec,nosuid,gid=5,mode=620 0 0

and then issue the command \`mount -a'" >&2

if test $CENTOS_MAJOR_VERSION -eq 4; then
	export CC=gcc4
	export CXX=g++4
fi
export PATH=$INSTALL_DIR/bin:$PATH
export LD_LIBRARY_PATH=$INSTALL_DIR/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH

TMP_DIR=${TMP_DIR-gebr-tmp-dir}
mkdir -p $TMP_DIR && cd $TMP_DIR

# libtool-2.2
	wget -c ftp.gnu.org/gnu/libtool/libtool-2.2.tar.bz2 \
	&& (cd libtool-2.2 2> /dev/null || (tar xjf libtool-2.2.tar.bz2 \
	&& cd libtool-2.2 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd libtool-2.2 \
	&& make install -j$CORES && cd .. || exit 1

# pkg-config-0.25
	wget -c http://pkgconfig.freedesktop.org/releases/pkg-config-0.25.tar.gz \
	&& (cd pkg-config-0.25 2> /dev/null || (tar xzf pkg-config-0.25.tar.gz \
	&& cd pkg-config-0.25 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd pkg-config-0.25 \
	&& make install -j$CORES && cd .. || exit 1

# glib-2.24.1
	wget -c http://ftp.gnome.org/pub/gnome/sources/glib/2.24/glib-2.24.1.tar.bz2 \
	&& (cd glib-2.24.1 2> /dev/null || (tar jxf glib-2.24.1.tar.bz2 \
	&& cd glib-2.24.1 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd glib-2.24.1 \
	&& make install -j$CORES && cd .. || exit 1

# atk-1.20.0
	wget -c http://ftp.acc.umu.se/pub/GNOME/sources/atk/1.13/atk-1.13.2.tar.bz2 \
	&& (cd atk-1.13.2 2> /dev/null || (tar jxf atk-1.13.2.tar.bz2 \
	&& cd atk-1.13.2 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd atk-1.13.2 \
	&& make install -j$CORES && cd .. || exit 1

# pixman-0.10.0
	wget -c http://cairographics.org/releases/pixman-0.10.0.tar.gz \
	&& (cd pixman-0.10.0 2> /dev/null || (tar zxf pixman-0.10.0.tar.gz \
	&& cd pixman-0.10.0 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd pixman-0.10.0 \
	&& make install -j$CORES && cd .. || exit 1

# libpng-1.2.44.tar.gz
	wget -c http://downloads.sourceforge.net/project/libpng/libpng12/older-releases/1.2.44/libpng-1.2.44.tar.gz \
	&& (cd libpng-1.2.44 2> /dev/null || (tar xzf libpng-1.2.44.tar.gz \
	&& cd libpng-1.2.44 \
	&& ./configure --prefix=$INSTALL_DIR)) \
	&& cd libpng-1.2.44 \
	&& make install -j$CORES && cd .. || exit 1

# freetype-2.3.12
	wget -c http://download.savannah.gnu.org/releases/freetype/freetype-2.3.12.tar.bz2 \
	&& (cd freetype-2.3.12 2> /dev/null || (tar jxf freetype-2.3.12.tar.bz2 \
	&& cd freetype-2.3.12 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd freetype-2.3.12 \
	&& make install -j$CORES && cd .. || exit 1

# fontconfig-2.4.92
	wget -c http://www.fontconfig.org/release/fontconfig-2.4.92.tar.gz \
	&& (cd fontconfig-2.4.92 2> /dev/null || (tar xzf fontconfig-2.4.92.tar.gz \
	&& cd fontconfig-2.4.92 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd fontconfig-2.4.92 \
	&& make install -j$CORES && cd .. || exit 1

# cairo-1.6.10
	wget -c http://cairographics.org/releases/cairo-1.6.4.tar.gz \
	&& (cd cairo-1.6.4 2> /dev/null || (tar zxf cairo-1.6.4.tar.gz \
	&& cd cairo-1.6.4 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG --enable-xlib=yes)) \
	&& cd cairo-1.6.4 \
	&& make install -j$CORES && cd .. || exit 1

# pango-1.20.5
	wget -c http://ftp.gnome.org/pub/gnome/sources/pango/1.20/pango-1.20.5.tar.bz2 \
	&& (cd pango-1.20.5 2> /dev/null || (tar xjf pango-1.20.5.tar.bz2 \
	&& cd pango-1.20.5 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd pango-1.20.5 \
	&& make install -j$CORES && cd .. || exit 1

# gtk-2.20.1
	wget -c http://ftp.gnome.org/pub/gnome/sources/gtk+/2.20/gtk+-2.20.1.tar.bz2 \
	&& (cd gtk+-2.20.1 2> /dev/null || (tar jxf gtk+-2.20.1.tar.bz2 \
	&& cd gtk+-2.20.1 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG --without-libjasper)) \
	&& cd gtk+-2.20.1 \
	&& make install -j$CORES && cd .. || exit 1

# libxml2-2.6.32
	wget -c ftp://xmlsoft.org/libxml2/old/libxml2-2.6.32.tar.gz \
	&& (cd libxml2-2.6.32 2> /dev/null || (tar zxf libxml2-2.6.32.tar.gz \
	&& cd libxml2-2.6.32 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG --without-python)) \
	&& cd libxml2-2.6.32 \
	&& make install -j$CORES && cd .. || exit 1

# gdome2-0.8.1
	wget -c http://gdome2.cs.unibo.it/tarball/gdome2-0.8.1.tar.gz \
	&& (cd gdome2-0.8.1 2> /dev/null || (tar zxf gdome2-0.8.1.tar.gz \
	&& cd gdome2-0.8.1 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd gdome2-0.8.1 \
	&& make install -j$CORES && cd .. || exit 1

# icu4c-4_2_1-src
	wget -c http://download.icu-project.org/files/icu4c/4.2.1/icu4c-4_2_1-src.tgz \
	&& (cd icu/source 2> /dev/null || (tar xzf icu4c-4_2_1-src.tgz \
	&& cd icu/source \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG --disable-tests --disable-samples)) \
	&& cd icu/source \
	&& make install && cd ../.. || exit 1

if test $CENTOS_MAJOR_VERSION -eq 4; then
# xproto-6.6.2
	wget -c http://xlibs.freedesktop.org/release/xproto-6.6.2.tar.bz2 \
	&& (cd xproto-6.6.2 2> /dev/null || (tar xjf xproto-6.6.2.tar.bz2 \
	&& cd xproto-6.6.2 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd xproto-6.6.2 \
	&& make -j$CORES install && cd .. || exit 1

# libxtrans-0.1
	wget -c http://xlibs.freedesktop.org/release/libXtrans-0.1.tar.bz2 \
	&& (cd libXtrans-0.1 2> /dev/null || (tar xjf libXtrans-0.1.tar.bz2 \
	&& cd libXtrans-0.1 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd libXtrans-0.1 \
	&& make -j$CORES install && cd .. || exit 1

# libICE-6.3.3
	wget -c http://xlibs.freedesktop.org/release/libICE-6.3.3.tar.bz2 \
	&& (cd libICE-6.3.3 2> /dev/null || (tar xjf libICE-6.3.3.tar.bz2 \
	&& cd libICE-6.3.3 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd libICE-6.3.3 \
	&& make -j$CORES install && cd .. || exit 1

# libSM-6.0.3
	wget -c http://xlibs.freedesktop.org/release/libSM-6.0.3.tar.bz2 \
	&& (cd libSM-6.0.3 2> /dev/null || (tar xjf libSM-6.0.3.tar.bz2 \
	&& cd libSM-6.0.3 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd libSM-6.0.3 \
	&& make -j$CORES install && cd .. || exit 1

# libXt-0.1.5
	wget -c http://xlibs.freedesktop.org/release/libXt-0.1.5.tar.bz2 \
	&& (cd libXt-0.1.5 2> /dev/null || (tar xjf libXt-0.1.5.tar.bz2 \
	&& cd libXt-0.1.5 \
	&& patch Initialize.c << EOF
306,310c306
< 	if ((pw = _XGetpwuid(getuid(),pwparams)) != NULL) {
< 	    (void) strncpy (dest, pw->pw_name, len-1);
< 	    dest[len-1] = '\\0';
< 	} else
< 	    *dest = '\\0';
---
> 	*dest = '\\0';
353,361c349
< 	if ((ptr = getenv("USER")))
< 	    pw = _XGetpwnam(ptr,pwparams);
< 	else
<  	    pw = _XGetpwuid(getuid(),pwparams);
< 	if (pw != NULL) {
< 	    (void) strncpy (dest, pw->pw_dir, len-1);
< 	    dest[len-1] = '\\0';
< 	} else
< 	    *dest = '\\0';
---
> 	*dest = '\\0';
EOF
	[ $? -eq 0 ] && ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd libXt-0.1.5 \
	&& make -j$CORES install && cd .. || exit 1
fi

# libxslt-1.1.26
	wget -c ftp://xmlsoft.org/libxslt/libxslt-1.1.26.tar.gz \
	&& (cd libxslt-1.1.26 2> /dev/null || (tar xzf libxslt-1.1.26.tar.gz \
	&& cd libxslt-1.1.26 && touch libtoolT \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd libxslt-1.1.26 \
	&& make -j$CORES install && cd .. || exit 1

# docbook-xsl-1.75.2
	wget -c http://sourceforge.net/projects/docbook/files/docbook-xsl/1.75.2/docbook-xsl-1.75.2.tar.bz2 \
	&& (cd docbook-xsl-1.75.2 2> /dev/null || tar xjf docbook-xsl-1.75.2.tar.bz2) \
	&& cd docbook-xsl-1.75.2 \
	&& export XML_CATALOG_FILES=$PWD/catalog.xml \
	&& cd - || exit 1

# tidy-20091223cvs
	wget -c http://ftp.de.debian.org/debian/pool/main/t/tidy/tidy_20091223cvs.orig.tar.gz \
	&& (cd tidy-20091223cvs 2> /dev/null || (tar xzf tidy_20091223cvs.orig.tar.gz \
	&& cd tidy-20091223cvs \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd tidy-20091223cvs \
	&& make -j$CORES install && cd .. || exit 1

# libsoup
	wget -c http://ftp.gnome.org/pub/GNOME/desktop/2.25/2.25.91/sources/libsoup-2.25.91.tar.bz2 \
	&& (cd libsoup-2.25.91 2> /dev/null || (tar xjf libsoup-2.25.91.tar.bz2 \
	&& cd libsoup-2.25.91 \
	&& patch configure << EOF
23482c23482
< 		-Wmissing-include-dirs -Wundef -Waggregate-return \\
---
> 		-Wundef -Waggregate-return \\
EOF
	[ $? -eq 0 ] && ./configure --prefix=$INSTALL_DIR $DEBUG --without-gnome)) \
	&& cd libsoup-2.25.91 \
	&& make -j$CORES install && cd .. || exit 1

# sqlite
	([ -f sqlite-3.6.10.tar.gz ] || wget -c http://www.sqlite.org/sqlite-3.6.10.tar.gz) \
	&& (cd sqlite-3.6.10 2> /dev/null || (tar xzf sqlite-3.6.10.tar.gz \
	&& cd sqlite-3.6.10 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG --disable-tcl)) \
	&& cd sqlite-3.6.10 \
	&& make -j$CORES install && cd .. || exit 1

# webkit-1.1.5
	([ -f webkit-1.1.5.tar.gz ] || wget -c http://webkitgtk.org/webkit-1.1.5.tar.gz) \
	&& (cd webkit-1.1.5 2> /dev/null || (tar xzf webkit-1.1.5.tar.gz \
	&& cd webkit-1.1.5 \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG --disable-xslt --disable-video)) \
	&& cd webkit-1.1.5
	if test $CENTOS_MAJOR_VERSION -eq 5; then
		cat >> autotoolsconfig.h <<EOF
/* _X_SENTINEL BS */
#if defined(__GNUC__) && (__GNUC__ >= 4)
# define _X_SENTINEL(x) __attribute__ ((__sentinel__(x)))

# define _X_ATTRIBUTE_PRINTF(x,y) __attribute__((__format__(__printf__,x,y)))

#else
# define _X_SENTINEL(x)
# define _X_ATTRIBUTE_PRINTF(x,y)
#endif /* GNUC >= 4 */
EOF
		[ $? -eq 0 ] && patch WebCore/plugins/gtk/gtk2xtbin.h <<EOF
42a43,51
> /* _X_SENTINEL BS */
> #if defined(__GNUC__) && (__GNUC__ >= 4) 
> # define _X_SENTINEL(x) __attribute__ ((__sentinel__(x))) 
> # define _X_ATTRIBUTE_PRINTF(x,y) __attribute__((__format__(__printf__,x,y))) 
> #else 
> # define _X_SENTINEL(x) 
> # define _X_ATTRIBUTE_PRINTF(x,y) 
> #endif /* GNUC >= 4 */
> 
157a167
> 
EOF
		[ $? -eq 0 ] && make install -j$CORES && cd .. || exit 1
	else
		make install -j$CORES && cd .. || exit 1
	fi

# gebr
	[ -d ../../gebrproject-$GEBR_VERSION ] && ln -s ../../gebrproject-$GEBR_VERSION \
	|| (([ -f gebrproject-$GEBR_VERSION.tar.gz ] || wget -c $GEBR_REPO/gebrproject-$GEBR_VERSION.tar.gz) \
	&& rm -fr gebrproject-$GEBR_VERSION && tar zxf gebrproject-$GEBR_VERSION.tar.gz) \
	&& cd gebrproject-$GEBR_VERSION
# Check for version 0.11.0 or greater
if [ $GEBR_MAJOR -ge 0 -a $GEBR_MINOR -ge 11 ]; then
	./configure --prefix=$INSTALL_DIR $DEBUG CFLAGS=-I/opt/gebrproject/include LIBS=-L/opt/gebrproject/lib \
	&& make install -j$CORES && cd .. || exit 1
else
	for dir in libgebr gebrd gebr debr; do
		cd $dir && ./configure --prefix=$INSTALL_DIR $DEBUG \
		&& make install -j$CORES && cd ..
	done
	cd ..
fi

# GeBR menus
for menu in $GEBR_MENUS; do
	([ -f $menu.tar.gz ] || wget -c $MENU_REPO/$menu.tar.gz) \
	&& (cd $menu 2> /dev/null || (tar xzf $menu.tar.gz \
	&& cd $menu \
	&& ./configure --prefix=$INSTALL_DIR $DEBUG)) \
	&& cd $menu \
	&& make -j$CORES install && cd .. || exit 1
done

#Save binaries
cd $INSTALL_DIR/bin
GEBR_BINS=`ls debr gebr*`
mkdir -p $INSTALL_DIR/lib/libgebr/bin
mv $GEBR_BINS $INSTALL_DIR/lib/libgebr/bin
cd - > /dev/null

#Tidy yp
if [ "$INSTALL_DIR" = "/opt/gebrproject" -a "$DEBUG" != "--enable-debug" ]; then
	rm /opt/gebrproject/bin/*
	strip /opt/gebrproject/lib/libgebr/bin/*
	find /opt/gebrproject/lib -iname "*\.a" | xargs rm
	find /opt/gebrproject/lib -iname "*\.la" | xargs rm
	find /opt/gebrproject/lib -iname "*\.so" | xargs strip
	rm -r /opt/gebrproject/sbin
	rm -r /opt/gebrproject/man
	rm -r /opt/gebrproject/include
	rm -r /opt/gebrproject/share/gtk-doc
	find /opt/gebrproject/share/locale/* -maxdepth 0 \( ! -iname "pt*" ! -iname "en*" \) | xargs rm -r
	find /opt/gebrproject/share/doc/* -maxdepth 0 \( ! -iname "gebr*" \) | xargs rm -r
fi

#Create wrapper
echo '#!/bin/sh
GEBR_HOME=$(cd -P "$(dirname "$0")"/.. && pwd -P) || exit 1
GEBR_PROG=`basename $0`
LD_LIBRARY_PATH=$GEBR_HOME/lib:$LD_LIBRARY_PATH PATH=$GEBR_HOME/lib/libgebr/bin:$PATH $GEBR_HOME/lib/libgebr/bin/$GEBR_PROG "$@"' > $INSTALL_DIR/lib/libgebr/bin/gebr-run.sh
chmod +x $INSTALL_DIR/lib/libgebr/bin/gebr-run.sh

#Create links
for bin in $GEBR_BINS; do
        ln -sf ../lib/libgebr/bin/gebr-run.sh $INSTALL_DIR/bin/$bin
done

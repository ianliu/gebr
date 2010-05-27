for sub in libgebr gebrd gebr debr;
do
	cp -a configure.ac.decl Makefile.decl $sub;
done

##
# Generates gtk-doc.make file
#
cd libgebr; gtkdocize || exit 1; cd ..

AUTOPOINT='intltoolize --automake --copy' autoreconf --install $*

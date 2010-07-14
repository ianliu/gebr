# Generates gtk-doc.make file
#
cd libgebr && mkdir -p m4 && gtkdocize && cd .. || exit 1

for sub in libgebr gebrd gebr debr;
do
	cp -a configure.ac.decl Makefile.decl $sub;
done

AUTOPOINT='intltoolize --automake --copy' autoreconf --install $*

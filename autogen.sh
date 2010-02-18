for sub in libgebr gebrd gebr debr; do cp -a configure.ac.decl Makefile.decl $sub; done
AUTOPOINT='intltoolize --automake --copy' autoreconf --install $*

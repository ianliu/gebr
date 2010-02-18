for sub in libgebr gebrd gebr debr; do cp -al configure.ac.decl Makefile.decl $sub; done
AUTOPOINT='intltoolize --automake --copy' autoreconf --install $*

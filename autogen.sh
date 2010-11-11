# Generates gtk-doc.make file
#
mkdir m4
gtkdocize
autoreconf --install $*

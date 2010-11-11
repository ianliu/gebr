# Generates gtk-doc.make file
#
mkdir -p m4
gtkdocize
autoreconf --install $*

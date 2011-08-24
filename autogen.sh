libtoolize --copy --force
# Don't run gtkdocize since it will overwrite the gtk-doc.make
# which has some changes to solve a 'distcheck' problem.
# The only this it does is copy the gtk-doc.make and gtk-doc.m4,
# which are already in the package.
# gtkdocize
autoreconf -i

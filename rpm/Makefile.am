include $(top_srcdir)/Makefile.decl

EXTRA_DIST = gebrproject.fedora.spec	\
	     gebrproject.opensuse.spec	\
	     gebrproject.mandriva.spec

packages: fedora-package

rpm-structure:
	(cd $(top_srcdir); \
	 if ! make dist; then \
	   echo Failed to make dist, exiting...; \
	   exit 1; \
	 fi)
	mkdir -p BUILD RPMS SRPMS SOURCES;
	cp $(top_srcdir)/@PACKAGE_TARNAME@-@PACKAGE_VERSION@.tar.gz SOURCES

fedora-package: rpm-structure
	rpmbuild -ba gebrproject.fedora.spec

opensuse-package: rpm-structure
	rpmbuild -ba gebrproject.opensuse.spec

mandriva-package: rpm-structure
	rpmbuild -ba gebrproject.mandriva.spec

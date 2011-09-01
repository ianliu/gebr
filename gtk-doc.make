# -*- mode: makefile -*-

####################################
# Everything below here is generic #
####################################

if GTK_DOC_USE_LIBTOOL
GTKDOC_CC = $(LIBTOOL) --tag=CC --mode=compile $(CC) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
GTKDOC_LD = $(LIBTOOL) --tag=CC --mode=link $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS)
GTKDOC_RUN = $(LIBTOOL) --mode=execute
else
GTKDOC_CC = $(CC) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
GTKDOC_LD = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS)
GTKDOC_RUN =
endif

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH, when it comes to
# whether a source that is a target of one rule is then
# searched for in VPATH/GPATH.
#
GPATH = $(srcdir)

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

DOC_STAMPS=			\
	scan-build.stamp	\
	sgml-build.stamp	\
	html-build.stamp	\
	pdf-build.stamp		\
	$(srcdir)/html.stamp	\
	$(srcdir)/pdf.stamp

SCANOBJ_FILES =				\
	$(DOC_MODULE).args		\
	$(DOC_MODULE).hierarchy		\
	$(DOC_MODULE).interfaces	\
	$(DOC_MODULE).prerequisites	\
	$(DOC_MODULE).signals

REPORT_FILES = \
	$(DOC_MODULE)-undocumented.txt \
	$(DOC_MODULE)-undeclared.txt \
	$(DOC_MODULE)-unused.txt

CLEANFILES = $(SCANOBJ_FILES) $(REPORT_FILES) $(DOC_STAMPS)

if ENABLE_GTK_DOC
if GTK_DOC_BUILD_HTML
HTML_BUILD_STAMP=html-build.stamp
else
HTML_BUILD_STAMP=
endif
if GTK_DOC_BUILD_PDF
PDF_BUILD_STAMP=pdf-build.stamp
else
PDF_BUILD_STAMP=
endif

all-local: $(HTML_BUILD_STAMP) $(PDF_BUILD_STAMP)
else
all-local:
endif

docs: $(HTML_BUILD_STAMP) $(PDF_BUILD_STAMP)

$(REPORT_FILES): sgml-build.stamp

#### scan ####

scan-build.stamp: $(HFILE_GLOB) $(CFILE_GLOB)
	@echo 'gtk-doc: Scanning header files'
	@-chmod -R u+w $(srcdir)
	@cd $(srcdir) && \
	  gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)" $(SCAN_OPTIONS) $(EXTRA_HFILES)
	@if grep -l '^..*$$' $(srcdir)/$(DOC_MODULE).types > /dev/null 2>&1 ; then \
	    CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)" RUN="$(GTKDOC_RUN)" CFLAGS="$(GTKDOC_CFLAGS) $(CFLAGS)" LDFLAGS="$(GTKDOC_LIBS) $(LDFLAGS)" gtkdoc-scangobj $(SCANGOBJ_OPTIONS) --module=$(DOC_MODULE) --output-dir=$(srcdir) ; \
	else \
	    cd $(srcdir) ; \
	    for i in $(SCANOBJ_FILES) ; do \
               test -f $$i || touch $$i ; \
	    done \
	fi
	@touch scan-build.stamp

#### xml ####

sgml-build.stamp: scan-build.stamp
	@echo 'gtk-doc: Building XML'
	@-chmod -R u+w $(srcdir)
	@cd $(srcdir) && \
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --output-format=xml --expand-content-files="$(expand_content_files)" --main-sgml-file=$(DOC_MAIN_SGML_FILE) $(MKDB_OPTIONS)
	@touch sgml-build.stamp

#### html ####

html-build.stamp: sgml-build.stamp $(DOC_MAIN_SGML_FILE) $(content_files)
	@echo 'gtk-doc: Building HTML'
	@-chmod -R u+w $(srcdir)
	@rm -rf $(srcdir)/html
	@mkdir $(srcdir)/html
	@mkhtml_options=""; \
	gtkdoc-mkhtml 2>&1 --help | grep  >/dev/null "\-\-path"; \
	if test "$$?" = "0"; then \
	  mkhtml_options=--path="..:$${PWD}"; \
	  echo "** DEBUG: gtkdoc-mkhtml $$mkhtml_options $(MKHTML_OPTIONS) $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)" ; \
	fi ; \
	cd $(srcdir)/html && gtkdoc-mkhtml $$mkhtml_options $(MKHTML_OPTIONS) $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	@test "x$(HTML_IMAGES)" = "x" || ( cd $(srcdir) && cp $(HTML_IMAGES) html )
	@echo 'gtk-doc: Fixing cross-references'
	@cd $(srcdir) && gtkdoc-fixxref --module=$(DOC_MODULE) --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	@touch html-build.stamp

#### pdf ####

pdf-build.stamp: sgml-build.stamp $(DOC_MAIN_SGML_FILE) $(content_files)
	@echo 'gtk-doc: Building PDF'
	@-chmod -R u+w $(srcdir)
	@rm -rf $(srcdir)/$(DOC_MODULE).pdf
	@mkpdf_imgdirs=""; \
	if test "x$(HTML_IMAGES)" != "x"; then \
	  for img in $(HTML_IMAGES); do \
	    part=`dirname $$img`; \
	    echo $$mkpdf_imgdirs | grep >/dev/null "\-\-imgdir=$$part "; \
	    if test $$? != 0; then \
	      mkpdf_imgdirs="$$mkpdf_imgdirs --imgdir=$$part"; \
	    fi; \
	  done; \
	fi; \
	cd $(srcdir) && gtkdoc-mkpdf --path="$(abs_srcdir)" $$mkpdf_imgdirs $(DOC_MODULE) $(DOC_MAIN_SGML_FILE) $(MKPDF_OPTIONS)
	@touch pdf-build.stamp

##############

clean-local:
	rm -f *~ *.bak
	rm -rf .libs

distclean-local:
	cd $(srcdir) && \
	  rm -rf xml $(REPORT_FILES) $(DOC_MODULE).pdf \
	         $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf html

install-data-local:
	@installfiles=`echo $(srcdir)/html/*`; \
	if test "$$installfiles" = '$(srcdir)/html/*'; \
	then echo '-- Nothing to install' ; \
	else \
	  if test -n "$(DOC_MODULE_VERSION)"; then \
	    installdir="$(DESTDIR)$(TARGET_DIR)-$(DOC_MODULE_VERSION)"; \
	  else \
	    installdir="$(DESTDIR)$(TARGET_DIR)"; \
	  fi; \
	  $(mkinstalldirs) $${installdir} ; \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $${installdir}; \
	  done; \
	  if test -n "$(DOC_MODULE_VERSION)"; then \
	    mv -f $${installdir}/$(DOC_MODULE).devhelp2 \
	      $${installdir}/$(DOC_MODULE)-$(DOC_MODULE_VERSION).devhelp2; \
	    mv -f $${installdir}/$(DOC_MODULE).devhelp \
	      $${installdir}/$(DOC_MODULE)-$(DOC_MODULE_VERSION).devhelp; \
	  fi; \
	  $(GTKDOC_REBASE) --relative --dest-dir=$(DESTDIR) --html-dir=$${installdir}; \
	fi

uninstall-local:
	@if test -n "$(DOC_MODULE_VERSION)"; then \
	  installdir="$(DESTDIR)$(TARGET_DIR)-$(DOC_MODULE_VERSION)"; \
	else \
	  installdir="$(DESTDIR)$(TARGET_DIR)"; \
	fi; \
	rm -rf $${installdir}

.PHONY : docs

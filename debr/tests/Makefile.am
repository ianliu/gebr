include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = -I$(srcdir)/../src	\
	      $(GEBR_CFLAGS)		\
	      $(GEBR_GUI_CFLAGS)	\
	      $(GEBR_GEOXML_CFLAGS)	\
	      @DEBUG_CFLAGS@

AM_LDFLAGS = ../src/libdebr.a		\
	     $(GEBR_LIBS)		\
	     $(GEBR_GEOXML_LIBS)	\
	     $(GEBR_GUI_LIBS)

TEST_PROGS += test-debr-tmpl
test_debr_tmpl_SOURCES = test-debr-tmpl.c

include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

EXTRA_DIST = test.mnu

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = -I$(srcdir)/../gebr	\
	      $(GEBR_CFLAGS)		\
	      $(GEBR_GEOXML_CFLAGS)	\
	      $(GEBR_COMM_CFLAGS)	\
	      $(GEBR_GUI_CFLAGS)	\
	      @DEBUG_CFLAGS@		\
	      -DTEST_DIR=\"$(srcdir)\"
	    
AM_LDFLAGS = $(GEBR_LIBS)		\
	     $(GEBR_GEOXML_LIBS)	\
	     $(GEBR_COMM_LIBS)		\
	     $(GEBR_GUI_LIBS)

#TEST_PROGS += test-help
#test_help_SOURCES = test-help.c
#test_help_LDADD = ../gebr/libgebr.a

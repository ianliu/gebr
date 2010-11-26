include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_GUI_CFLAGS)	\
	      $(GEBR_GEOXML_CFLAGS)	\
	      @DEBUG_CFLAGS@

AM_LDFLAGS = $(GEBR_LIBS)		\
	     $(GEBR_GEOXML_LIBS)	\
	     $(GEBR_GUI_LIBS)

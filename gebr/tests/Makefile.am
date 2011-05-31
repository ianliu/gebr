include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

EXTRA_DIST = test.mnu

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GUI_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_JSON_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	$(GEBR_GUI_CFLAGS)	\
	-I$(srcdir)/..		\
	@DEBUG_CFLAGS@		\
	-DTEST_DIR=\"$(srcdir)\"\
	$(NULL)
	    
AM_LDFLAGS =			\
	$(GUI_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(GEBR_GUI_LIBS)	\
	../libgebr.a		\
	$(NULL)

#TEST_PROGS += test-help
#test_help_SOURCES = test-help.c
#test_help_LDADD = ../gebr/libgebr.a

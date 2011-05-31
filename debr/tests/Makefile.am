include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GUI_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
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
	$(GEBR_GUI_LIBS)	\
	../libdebr.a	\
	$(NULL)

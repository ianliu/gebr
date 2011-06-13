include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GUI_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_GUI_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	$(NULL)

AM_LDFLAGS =			\
	$(GUI_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_GUI_LIBS)	\
	$(NULL)

TEST_PROGS += test-js
test_js_SOURCES = test-js.c

TEST_PROGS += test-utils
test_utils_SOURCES = test-utils.c

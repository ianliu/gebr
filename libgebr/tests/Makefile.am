include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_GUI_CFLAGS)	\
	      $(GEBR_GEOXML_CFLAGS)	\
	      @DEBUG_CFLAGS@		\
	      -DTAR_TEST_FILE='"$(srcdir)/tar-test.tar.gz"'

AM_LDFLAGS = $(GEBR_LIBS)		\
	     $(GEBR_GUI_LIBS)		\
	     $(GEBR_GEOXML_LIBS)	\
	     ../libgebr.la

TEST_PROGS += test-utils
test_utils_SOURCES = test-utils.c

TEST_PROGS += test-gebr-tar
test_gebr_tar_SOURCES = test-gebr-tar.c

include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = $(GEBR_CFLAGS) @DEBUG_CFLAGS@ -DTEST_SRCDIR='"$(srcdir)"'

AM_LDFLAGS = $(GEBR_LIBS) ../libgebr.la -lm

TEST_PROGS += test-utils
test_utils_SOURCES = test-utils.c

TEST_PROGS += test-gebr-tar
test_gebr_tar_SOURCES = test-gebr-tar.c

EXTRA_DIST = tar-test.tar.gz
CLEANFILES = tar-create-test.tar.gz

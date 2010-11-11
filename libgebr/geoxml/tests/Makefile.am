include $(top_srcdir)/Makefile.decl

EXTRA_DIST = test.mnu

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ -I$(srcdir)/.. -I$(srcdir)/../.. $(GLIB_CFLAGS) $(GDOME2_CFLAGS) -DTEST_DIR=\"$(srcdir)\"
AM_LDFLAGS = ../libgebr_geoxml.la $(GLIB_LIBS) $(GDOME2_LIBS) 

TEST_PROGS += test-sequence
test_sequence_SOURCES = test-sequence.c

TEST_PROGS += test-program
test_program_SOURCES = test-program.c


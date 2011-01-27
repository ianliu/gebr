include $(top_srcdir)/Makefile.decl

EXTRA_DIST = test.mnu

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)
AM_CPPFLAGS =  $(GEBR_CFLAGS)		\
	       $(GEBR_GEOXML_CFLAGS)	\
	       @DEBUG_CFLAGS@		\
	       -I$(srcdir)/..		\
	       -DTEST_DIR=\"$(srcdir)\"
AM_LDFLAGS = $(GEBR_LIBS) $(GEBR_GEOXML_LIBS)

TEST_PROGS += test-sequence
test_sequence_SOURCES = test-sequence.c

TEST_PROGS += test-program
test_program_SOURCES = test-program.c

TEST_PROGS += test-tmpl
test_tmpl_SOURCES = test-tmpl.c

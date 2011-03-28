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

TEST_PROGS += test-document
test_document_SOURCES = test-document.c

TEST_PROGS += test-line
test_line_SOURCES = test-line.c

TEST_PROGS += test-project
test_project_SOURCES = test-project.c

TEST_PROGS += test-flow
test_flow_SOURCES = test-flow.c

TEST_PROGS += test-object
test_object_SOURCES = test-object.c
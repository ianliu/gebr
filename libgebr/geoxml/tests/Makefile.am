include $(top_srcdir)/Makefile.decl

EXTRA_DIST = test.mnu

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)
AM_CPPFLAGS =				\
	$(GLIB_CFLAGS)			\
	$(GDOME2_CFLAGS)		\
	$(GEBR_CFLAGS)			\
	$(GEBR_GEOXML_CFLAGS)		\
	@DEBUG_CFLAGS@			\
	-DTEST_DIR=\"$(srcdir)\"	\
	$(NULL)

AM_LDFLAGS =			\
	$(GLIB_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(NULL)

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

TEST_PROGS += test-parameters
test_parameters_SOURCES = test-parameters.c

TEST_PROGS += test-parameter
test_parameter_SOURCES = test-parameter.c

TEST_PROGS += test-program-parameter
test_program_parameter_SOURCES = test-program-parameter.c

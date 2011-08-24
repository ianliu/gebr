include $(top_srcdir)/Makefile.decl

EXTRA_DIST =			\
	dict_test_flow.flw	\
	forloop.mnu		\
	new_test.flw		\
	promo035.flw		\
	test.mnu		\
	test2.flw		\
	unimodeling.flw		\
	z2xyz.mnu		\
	$(NULL)

DISTCLEANFILES = test.mnu forloop.mnu dict_test_flow.flw test2.flw

noinst_PROGRAMS = $(INT_TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)
AM_CPPFLAGS =				\
	$(GLIB_CFLAGS)			\
	$(GDOME2_CFLAGS)		\
	$(GEBR_CFLAGS)			\
	$(GEBR_GEOXML_CFLAGS)		\
	@DEBUG_CFLAGS@			\
	-DTEST_DIR=\"$(srcdir)\"	\
	-DDTD_DIR=\"$(top_srcdir)/libgebr/geoxml/data\" \
	$(NULL)

AM_LDFLAGS =			\
	$(GLIB_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(NULL)

INT_TEST_PROGS += test-sequence
test_sequence_SOURCES = test-sequence.c

INT_TEST_PROGS += test-program
test_program_SOURCES = test-program.c

INT_TEST_PROGS += test-tmpl
test_tmpl_SOURCES = test-tmpl.c

INT_TEST_PROGS += test-document
test_document_SOURCES = test-document.c

INT_TEST_PROGS += test-line
test_line_SOURCES = test-line.c

INT_TEST_PROGS += test-project
test_project_SOURCES = test-project.c

INT_TEST_PROGS += test-flow
test_flow_SOURCES = test-flow.c

INT_TEST_PROGS += test-object
test_object_SOURCES = test-object.c

INT_TEST_PROGS += test-parameters
test_parameters_SOURCES = test-parameters.c

INT_TEST_PROGS += test-parameter
test_parameter_SOURCES = test-parameter.c

INT_TEST_PROGS += test-program-parameter
test_program_parameter_SOURCES = test-program-parameter.c

INT_TEST_PROGS += test-geoxml-leaks
test_geoxml_leaks_SOURCES = test-geoxml-leaks.c

include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GLIB_CFLAGS)		\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	-DTEST_SRCDIR='"$(srcdir)"' \
	$(NULL)

AM_LDFLAGS =			\
	$(GLIB_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(NULL)

TEST_PROGS += test-utils
test_utils_SOURCES = test-utils.c

TEST_PROGS += test-gebr-tar
test_gebr_tar_SOURCES = test-gebr-tar.c

TEST_PROGS += test-gebr-expr
test_gebr_expr_SOURCES = test-gebr-expr.c

TEST_PROGS += test-gebr-arith-expr
test_gebr_arith_expr_SOURCES = test-gebr-arith-expr.c

TEST_PROGS += test-gebr-string-expr
test_gebr_string_expr_SOURCES = test-gebr-string-expr.c

TEST_PROGS += test-gebr-validator
test_gebr_validator_SOURCES = test-gebr-validator.c

EXTRA_DIST = tar-test.tar.gz
CLEANFILES = tar-create-test.tar.gz

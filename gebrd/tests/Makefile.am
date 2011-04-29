include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS = $(GEBR_CFLAGS)		\
	      $(GEBR_GEOXML_CFLAGS)	\
	      $(GEBR_COMM_CFLAGS)	\
	      $(GEBR_JSON_CFLAGS)	\
	      @DEBUG_CFLAGS@ 		\
	      -I$(srcdir)/..

AM_LDFLAGS = $(GEBR_LIBS)		\
	     $(GEBR_GEOXML_LIBS)	\
	     $(GEBR_JSON_LIBS)		\
	     $(GEBR_COMM_LIBS)

TEST_PROGS += test-job-queue
test_job_queue_SOURCES = test-job-queue.c
test_job_queue_LDADD = ../libgebrd.a

# TEST_PROGS += test-job
# test_job_SOURCES = test-job.c
# test_job_LDADD = ../libgebrd.a

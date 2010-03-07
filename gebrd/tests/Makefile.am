include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ -I../src \
	    $(GEBR_CFLAGS) $(GEBR_GEOXML_CFLAGS) $(GEBR_COMM_CFLAGS)
AM_LDFLAGS = $(INTLLIBS) $(GEBR_LIBS) $(GEBR_GEOXML_LIBS) \
	     $(GEBR_COMM_LIBS)

TEST_PROGS += test-job-queue
test_job_queue_SOURCES = test-job-queue.c
test_job_queue_LDADD = $(top_srcdir)/src/libgebrd.a


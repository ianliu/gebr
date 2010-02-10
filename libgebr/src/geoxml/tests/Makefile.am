include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ -I.. $(GDOME2_CFLAGS)
AM_LDFLAGS = ../geoxml/libgebr_geoxml.la $(GDOME2_LIBS) 

TEST_PROGS += test-sequence
test_sequence_SOURCES = test-sequence.c


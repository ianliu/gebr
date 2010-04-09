include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ -I../comm $(GLIB_CFLAGS)
AM_LDFLAGS = ../comm/libgebr_comm.la $(GLIB_LIBS)

TEST_PROGS += test-protocol
test_protocol_SOURCES = test-protocol.c


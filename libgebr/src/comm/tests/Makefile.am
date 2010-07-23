include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = -I$(srcdir)/../comm -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GTHREAD_CFLAGS) $(JSONGLIB_CFLAGS)
AM_LDFLAGS = ../comm/libgebr_comm.la $(GLIB_LIBS)

TEST_PROGS += test-protocol
test_protocol_SOURCES = test-protocol.c

TEST_PROGS += test-socket
test_socket_SOURCES = test-socket.c

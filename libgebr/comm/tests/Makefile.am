include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GEBR_CFLAGS)		\
	$(GEBR_JSON_CFLAGS)	\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	-I$(srcdir)/..		\
	$(NULL)

AM_LDFLAGS =			\
	$(GEBR_LIBS)		\
	$(GEBR_JSON_LIBS)	\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(NULL)

TEST_PROGS += test-protocol
test_protocol_SOURCES = test-protocol.c

TEST_PROGS += test-socket
test_socket_SOURCES = test-socket.c

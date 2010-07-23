include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ -I$(srcdir)/../gui $(GLIB_CFLAGS) $(WEBKIT_CFLAGS)
AM_LDFLAGS = ../gui/libgebr_gui.la $(GLIB_LIBS) 

TEST_PROGS += test-js
test_js_SOURCES = test-js.c


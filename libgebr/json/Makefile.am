include $(top_srcdir)/Makefile.decl

if ENABLE_TESTS
TESTSDIR=tests
endif
SUBDIRS = . ${TESTSDIR}

EXTRA_DIST =

INCLUDES = -I$(top_srcdir)

AM_CPPFLAGS = \
	-DJSON_COMPILATION \
	-I$(top_srcdir)/libgebr \
	-DPREFIX=\""$(prefix)"\" 	\
	-DLIBDIR=\""$(libdir)"\" 	\
	-DJSON_COMPILATION=1		\
	-DG_DISABLE_DEPRECATED 		\
	-DJSON_DISABLE_DEPRECATED	\
	-DG_LOG_DOMAIN=\"Json\" 	\
	$(NULL)
AM_CFLAGS = $(GEBR_JSON_CFLAGS)

BUILT_SOURCES =
CLEANFILES =
DISTCLEANFILES = json-version.h

source_h = \
	json-builder.h		\
	json-generator.h 	\
	json-gobject.h 		\
	json-utils.h 		\
	json-parser.h 		\
	json-reader.h		\
	json-types.h 		\
	$(NULL)

source_h_private = \
	json-debug.h		\
	json-gobject-private.h	\
	json-scanner.h 		\
	json-marshal.h 		\
	json-types-private.h	\
	$(NULL)

source_c = \
	json-marshal.c 		\
	json-array.c 		\
	json-builder.c		\
	json-debug.c		\
	json-gboxed.c		\
	json-generator.c 	\
	json-gobject.c 		\
	json-utils.c	 	\
	json-node.c 		\
	json-object.c 		\
	json-parser.c 		\
	json-reader.c		\
	json-scanner.c 		\
	json-serializable.c	\
	$(NULL)

json-marshal.h: json-marshal.list
	glib-genmarshal --prefix=_json_marshal json-marshal.list --header >json-marshal.h
json-marshal.c: json-marshal.h json-marshal.list
	glib-genmarshal --prefix=_json_marshal json-marshal.list --body >json-marshal.c
CLEANFILES += json-marshal.c json-marshal.h

lib_LTLIBRARIES = libgebr_json.la

libgebr_json_la_LIBADD = ../libgebr.la
libgebr_json_la_SOURCES = $(source_c) $(source_h) $(source_h_private) $(BUILT_SOURCES)
libgebr_json_la_LDFLAGS = $(JSON_LT_LDFLAGS) -export-dynamic -export-symbols-regex "^json.*"

jsonincludedir = $(includedir)/libgebr/json/
jsoninclude_HEADERS = 		\
	$(source_h) 		\
	json-enum-types.h 	\
	json-version.h		\
	json-glib.h		\
	$(NULL)

EXTRA_DIST += json-version.h.in json-glib.h


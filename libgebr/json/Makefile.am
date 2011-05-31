include $(top_srcdir)/Makefile.decl

if ENABLE_TESTS
TESTSDIR=tests
endif
SUBDIRS = . ${TESTSDIR}

AM_CFLAGS = $(COMMON_CFLAGS) -fPIC

AM_CPPFLAGS = \
	-DJSON_COMPILATION 		\
	-DPREFIX=\""$(prefix)"\" 	\
	-DLIBDIR=\""$(libdir)"\" 	\
	-DJSON_COMPILATION=1		\
	-DG_DISABLE_DEPRECATED 		\
	-DJSON_DISABLE_DEPRECATED	\
	-DG_LOG_DOMAIN=\"Json\" 	\
	$(GLIB_CFLAGS)			\
	$(GEBR_CFLAGS)			\
	$(GEBR_JSON_CFLAGS)		\
	$(NULL)

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES = 

source_h = \
	json-builder.h		\
	json-generator.h 	\
	json-gobject.h 		\
	json-parser.h 		\
	json-reader.h		\
	json-types.h 		\
	json-utils.h 		\
	$(NULL)

source_h_private = \
	json-debug.h		\
	json-gobject-private.h	\
	json-scanner.h 		\
	json-types-private.h	\
	$(NULL)

source_c = \
	json-array.c 		\
	json-builder.c		\
	json-debug.c		\
	json-gboxed.c		\
	json-generator.c 	\
	json-gobject.c 		\
	json-node.c 		\
	json-object.c 		\
	json-parser.c 		\
	json-reader.c		\
	json-scanner.c 		\
	json-serializable.c	\
	json-utils.c		\
	$(NULL)

# glib-mkenums rules
glib_enum_h = json-enum-types.h
glib_enum_c = json-enum-types.c
glib_enum_headers =			\
	$(srcdir)/json-builder.h	\
	$(srcdir)/json-generator.h 	\
	$(srcdir)/json-gobject.h 	\
	$(srcdir)/json-parser.h 	\
	$(srcdir)/json-reader.h		\
	$(srcdir)/json-types.h 		\
	$(srcdir)/json-utils.h 		\
	$(NULL)

include $(top_srcdir)/Makefile.am.enums

# glib-genmarshal rules
glib_marshal_list = json-marshal.list
glib_marshal_prefix = _json_marshal
include $(top_srcdir)/Makefile.am.marshal

lib_LTLIBRARIES = libgebr_json.la

libgebr_json_la_LIBADD =	\
	$(GLIB_LIBS)		\
	$(GEBR_LIBS)		\
	$(NULL)

libgebr_json_la_SOURCES = $(source_c) $(source_h) $(source_h_private) $(BUILT_SOURCES)
libgebr_json_la_LDFLAGS = -export-dynamic -export-symbols-regex "^json.*"

jsonincludedir = $(includedir)/libgebr/json/
jsoninclude_HEADERS = \
	$(source_h) 		\
	json-enum-types.h 	\
	json-glib.h		\
	$(NULL)

EXTRA_DIST += json-glib.h

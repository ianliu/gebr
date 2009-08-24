BUILT_SOURCES = json-marshal.h json-marshal.c json-enum-types.h json-enum-types.c
STAMP_FILES = stamp-json-marshal.h stamp-json-enum-types.h

source_h = \
	json-generator.h \
	json-gobject.h \
	json-parser.h \
	json-types.h
source_h_private = \
	json-scanner.h
source_c = \
	json-array.c \
	json-generator.c \
	json-gobject.c \
	json-node.c \
	json-object.c \
	json-parser.c \
	json-scanner.c
EXTRA_DIST = \
	json-scanner.h \
	json-glib.h \
	json-marshal.list \
	json-enum-types.h.in \
	json-enum-types.c.in

INCLUDES = -I ..
AM_CFLAGS = $(GOBJECT_CFLAGS)
noinst_LTLIBRARIES = libjson-glib-1.0.la
libjson_glib_1_0_la_LDFLAGS = $(GOBJECT_LIBS)
libjson_glib_1_0_la_SOURCES = \
	$(source_c) \
	$(source_h) \
	$(source_h_priv) \
	$(BUILT_SOURCES)

json-marshal.h: stamp-json-marshal.h
	@true
stamp-json-marshal.h: Makefile json-marshal.list
	glib-genmarshal \
		--prefix=_json_marshal \
		--header \
	$(srcdir)/json-marshal.list > xgen-jmh && \
	(cmp -s xgen-jmh json-marshal.h || cp -f xgen-jmh json-marshal.h) && \
	rm -f xgen-jmh && \
	echo timestamp > $(@F)

json-marshal.c: Makefile json-marshal.list
	(echo "#include \"json-marshal.h\"" ; \
	 glib-genmarshal \
		--prefix=_json_marshal \
		--body \
	 $(srcdir)/json-marshal.list ) > xgen-jmc && \
	cp -f xgen-jmc json-marshal.c && \
	rm -f xgen-jmc

json-enum-types.h: stamp-json-enum-types.h
	@true
stamp-json-enum-types.h: $(source_h) Makefile
	glib-mkenums \
	  	--template json-enum-types.h.in \
	  $(source_h) > xgen-ceth \
	&& (cmp xgen-ceth json-enum-types.h || cp xgen-ceth json-enum-types.h) \
	&& rm -f xgen-ceth \
	&& echo timestamp > $(@F)

json-enum-types.c: json-enum-types.h
	glib-mkenums \
	  	--template json-enum-types.c.in \
	  $(source_h) > xgen-cetc \
	&& cp xgen-cetc json-enum-types.c \
	&& rm -f xgen-cetc

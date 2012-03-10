INCLUDES = -I$(top_srcdir)
REC = re2c
AM_YFLAGS = -d -t -v -p syck
SUFFIXES = .re
AM_CFLAGS = -fPIC

.re.c:
	$(REC) -i $< > $@.new && mv $@.new $@

lib_LTLIBRARIES = libsyck.la
lib_LIBRARIES = libsyck.a
include_HEADERS = syck.h syck_st.h

libsyck_a_SOURCES = \
	emitter.c \
	handler.c \
	node.c \
	syck.c \
	syck_st.c \
	gram.y \
	bytecode.re \
	yaml2byte.c \
	token.re \
	implicit.re

libsyck_la_CFLAGS = $(AM_CFLAGS)
libsyck_la_LDFLAGS = -version-info $(SYCK_SO_VERSION)
libsyck_la_SOURCES = $(libsyck_a_SOURCES)


INCLUDES = -I$(top_srcdir)

LEXLIB = @LEXLIB@

AM_YFLAGS = -d -t -v -p syck

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

# libsyck_a_LIBADD = $(LEXLIB)

REC = re2c
.re.c:
	$(REC) $< > $@.new && mv $@.new $@


INCLUDES = -I$(top_srcdir)

LEXLIB = @LEXLIB@

AM_YFLAGS = -d -t -v

lib_LIBRARIES = libsyck.a
include_HEADERS = syck.h syck_st.h

libsyck_a_SOURCES = \
            handler.c \
            node.c \
            syck.c \
            syck_st.c \
            gram.y \
            token.re

# libsyck_a_LIBADD = $(LEXLIB)

REC = re2c
.re.c:
	$(REC) -b $< > $@.new && mv $@.new $@


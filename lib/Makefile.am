INCLUDES = -I$(top_srcdir)

LEXLIB = @LEXLIB@

AM_YFLAGS = -d -t -v

lib_LIBRARIES = libsyck.a
libsyck_a_SOURCES = \
            handler.c \
            node.c \
            syck.c \
            syck.h \
            st.c \
            st.h \
            gram.y \
            token.l

# libsyck_a_LIBADD = $(LEXLIB)

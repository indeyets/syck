INCLUDES = -I$(top_srcdir)/lib
LDFLAGS = -L$(top_srcdir)/lib

TESTS = test-basic test-parse
noinst_PROGRAMS = test-basic test-parse
test_basic_SOURCES = Basic.c CuTest.c CuTest.h
test_basic_LDADD = -lsyck
test_parse_SOURCES = Parse.c CuTest.c CuTest.h
test_parse_LDADD = -lsyck

INCLUDES = -I$(top_srcdir)/lib
LDFLAGS = -L$(top_srcdir)/lib

TESTS = test-basic test-parse test-yts test-emit
noinst_PROGRAMS = test-basic test-parse test-yts test-emit
test_basic_SOURCES = Basic.c CuTest.c CuTest.h
test_basic_LDADD = -lsyck
test_parse_SOURCES = Parse.c CuTest.c CuTest.h
test_parse_LDADD = -lsyck
test_yts_SOURCES = YTS.c CuTest.c CuTest.h
test_yts_LDADD = -lsyck
test_emit_SOURCES = Emit.c CuTest.c CuTest.h
test_emit_LDADD = -lsyck

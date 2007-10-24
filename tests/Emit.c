//
// Emit.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include <string.h>
#include "syck.h"
#include "CuTest.h"

//
// 1. Test the buffering -- print 10 bytes at a time
//
void
TestSyckEmit_Output( SyckEmitter *e, const char *str, long len )
{
    // char *tmp = syck_strndup( str, len );
    // printf( "OUT: %s [%d]\n", tmp, len );
    // S_FREE( tmp );
}

void 
TestSyckEmit( CuTest *tc )
{
    SyckEmitter *emitter;

    emitter = syck_new_emitter();
    emitter->bufsize = 10;
    emitter->output_handler = TestSyckEmit_Output;

    syck_emitter_write( emitter, "Test [1]", 8 );
    syck_emitter_write( emitter, ".", 1 );
    syck_emitter_write( emitter, "Test [2]", 8 );
    syck_emitter_write( emitter, ".", 1 );
    syck_emitter_write( emitter, "Test [3]", 8 );
    syck_emitter_write( emitter, ".", 1 );
    syck_emitter_write( emitter, "Test [4]", 8 );
    syck_emitter_write( emitter, ".", 1 );
    syck_emitter_write( emitter, "END!", 4 );

    syck_free_emitter( emitter );
}

/*
 * Ensure that our base64 encoder can do some basic
 * binary encoding.
 */
void TestBase64Encode( CuTest *tc )
{
    char gif[] = "GIF89a\f\000\f\000\204\000\000\377\377\367\365\365\356\351\351\345fff\000\000\000\347\347\347^^^\363\363\355\216\216\216\340\340\340\237\237\237\223\223\223\247\247\247\236\236\236iiiccc\243\243\243\204\204\204\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371!\376\016Made with GIMP\000,\000\000\000\000\f\000\f\000\000\005,  \216\2010\236\343@\024\350i\020\304\321\212\010\034\317\200M$z\357\3770\205p\270\2601f\r\033\316\001\303\001\036\020' \202\n\001\000;"; 
    char *enc = syck_base64enc( gif, 185 );
    CuAssertStrEquals( tc, enc, "R0lGODlhDAAMAIQAAP//9/X17unp5WZmZgAAAOfn515eXvPz7Y6OjuDg4J+fn5OTk6enp56enmlpaWNjY6Ojo4SEhP/++f/++f/++f/++f/++f/++f/++f/++f/++f/++f/++f/++f/++f/++SH+Dk1hZGUgd2l0aCBHSU1QACwAAAAADAAMAAAFLCAgjoEwnuNAFOhpEMTRiggcz4BNJHrv/zCFcLiwMWYNG84BwwEeECcgggoBADs=\n" );
    S_FREE( enc );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckEmit );
    SUITE_ADD_TEST( suite, TestBase64Encode );
    return suite;
}

int main(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = SyckGetSuite();
    int count;

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);

	printf("%s\n", output->buffer);
    count = suite->failCount;

    CuStringFree( output );
    CuSuiteFree( suite );

    return count;
}

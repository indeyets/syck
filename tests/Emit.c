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
TestSyckEmit_Output( SyckEmitter *e, char *str, long len )
{
    // char *tmp = syck_strndup( str, len );
    // printf( "OUT: %s [%d]\n", tmp, len );
    // S_FREE( tmp );
}

void 
TestSyckEmit( CuTest *tc )
{
    SyckEmitter *emitter;
    char *tmp;
    int len = 0;

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
    char gif[] = "GIF89a\001\000\001\000\200\377\000\300\300\300\000\000\000!\371\004\001\000\000\000\000,\000\000\000\000\001\000\001\000\000\002\002D\001\000;"; 
    char *enc = syck_base64enc( gif, strlen( gif ) );
    printf( "ENCODED:\n%s\n", enc );
    S_FREE( enc );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckEmit );
//  SUITE_ADD_TEST( suite, TestBase64Encode );
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

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
    emitter->handler = TestSyckEmit_Output;

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

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckEmit );
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

//
// Parse.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"
#include "CuTest.h"

//
// Test parsing tokens of a simple string.
//
SYMID
SyckParseStringHandler( struct SyckNode *n )
{
}

void 
TestSyckParseString( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    free( parser );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckParseString );
    return suite;
}

int main(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();
	
	CuSuiteAddSuite(suite, SyckGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
    return suite->failCount;
}

//
// Parse.c
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
// Test parsing tokens of a simple string.
//
SYMID
SyckParseStringHandler( SyckParser *p, SyckNode *n )
{
    printf( "NODE: %s\n", n->type_id );
    return 1112;
}

enum st_retval 
ListAnchors( char *key, SyckNode *n, char *arg )
{
    printf( "KEY: %s, NODE: %s\n", key, syck_str_read( n ) );
    return ST_CONTINUE;
}

void 
TestSyckReadString( CuTest *tc )
{
    SyckParser *parser;
    char *buf = S_ALLOC_N( char, 4 );
    int len = 0;

    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "test: 1\nand: 2\nalso: 3", NULL );

    len = syck_parser_read( buf, parser, 4 );
    CuAssert( tc, "Wrong length, line 1.", 4 == len );
    CuAssertStrEquals( tc, "test", syck_strndup( buf, len ) );
    len = syck_parser_read( buf, parser, 4 );
    CuAssert( tc, "Wrong length, line 2.", 4 == len );
    CuAssertStrEquals( tc, ": 1\n", syck_strndup( buf, len ) );
    len = syck_parser_read( buf, parser, 4 );
    CuAssert( tc, "Wrong length, line 3.", 4 == len );
    CuAssertStrEquals( tc, "and:", syck_strndup( buf, len ) );
    len = syck_parser_read( buf, parser, 4 );
    CuAssert( tc, "Wrong length, line 4.", 4 == len );
    CuAssertStrEquals( tc, " 2\na", syck_strndup( buf, len ) );
    len = syck_parser_read( buf, parser, 4 );
    CuAssert( tc, "Wrong length, line 5.", 4 == len );
    CuAssertStrEquals( tc, "lso:", syck_strndup( buf, len ) );
    len = syck_parser_read( buf, parser, 4 );
    CuAssert( tc, "Wrong length, line 6.", 2 == len );
    CuAssertStrEquals( tc, " 3", syck_strndup( buf, len ) );

    free_any_io( parser );
    free( buf );
    free( parser );
}

void 
TestSyckParseString( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "--- {test: 1, and: 2, or: &test 13, also: *test}", NULL );
    syck_parser_init( parser, 1 );
    yyparse( parser );
    st_foreach( parser->anchors, ListAnchors, NULL );
    syck_free_parser( parser );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckReadString );
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

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
    return 1112;
}

enum st_retval 
ListAnchors( char *key, SyckNode *n, CuTest *tc )
{
    CuAssertStrEquals( tc, "test", key );
    CuAssertStrEquals( tc, "13", syck_strndup( n->data.str->ptr, n->data.str->len ) );
    return ST_CONTINUE;
}

void 
TestSyckReadString( CuTest *tc )
{
    SyckParser *parser;
    char *tmp;
    int len = 0;

    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "test: 1\nand: 2\nalso: 3", syck_io_str_read );

    len = syck_parser_readlen( parser, 4 );
    CuAssert( tc, "Wrong length, line 1.", 4 == len );
    parser->token = parser->buffer + 4;
    tmp = syck_strndup( parser->buffer, len );
    CuAssertStrEquals( tc, "test", tmp );
    free( tmp );

    len = syck_parser_readlen( parser, 4 );
    CuAssert( tc, "Wrong length, line 2.", 4 == len );
    parser->token = parser->buffer + 4;
    tmp = syck_strndup( parser->buffer, len );
    CuAssertStrEquals( tc, ": 1\n", tmp );
    free( tmp );
    
    len = syck_parser_readlen( parser, 4 );
    CuAssert( tc, "Wrong length, line 3.", 4 == len );
    parser->token = parser->buffer + 4;
    tmp = syck_strndup( parser->buffer, len );
    CuAssertStrEquals( tc, "and:", tmp );
    free( tmp );

    len = syck_parser_readlen( parser, 4 );
    CuAssert( tc, "Wrong length, line 4.", 4 == len );
    parser->token = parser->buffer + 4;
    tmp = syck_strndup( parser->buffer, len );
    CuAssertStrEquals( tc, " 2\na", tmp );
    free( tmp );

    len = syck_parser_readlen( parser, 4 );
    CuAssert( tc, "Wrong length, line 5.", 4 == len );
    parser->token = parser->buffer + 4;
    tmp = syck_strndup( parser->buffer, len );
    CuAssertStrEquals( tc, "lso:", tmp );
    free( tmp );

    len = syck_parser_readlen( parser, 4 );
    CuAssert( tc, "Wrong length, line 6.", 2 == len );
    parser->token = parser->buffer + 4;
    tmp = syck_strndup( parser->buffer, len );
    CuAssertStrEquals( tc, " 3", tmp );
    free( tmp );

    free_any_io( parser );
    syck_free_parser( parser );
}

void 
TestSyckParseString( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "--- {test: 1, and: 2, or: &test 13, also: *test}", NULL );
    yyparse( parser );
    st_foreach( parser->anchors, ListAnchors, tc );
    syck_free_parser( parser );
}

void 
TestSyckParseString2( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "--- {test: 1, and: 2, or: &test 13}", NULL );
    yyparse( parser );
    st_foreach( parser->anchors, ListAnchors, tc );
    syck_free_parser( parser );
}

void 
TestSyckParseMap( CuTest *tc )
{
    SYMID id;
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "\ntest: 1\nand: 2\nor: test", NULL );
    syck_parse( parser );
    syck_free_parser( parser );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckReadString );
    SUITE_ADD_TEST( suite, TestSyckParseString );
    SUITE_ADD_TEST( suite, TestSyckParseString2 );
    //SUITE_ADD_TEST( suite, TestSyckParseMap );
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

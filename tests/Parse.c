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
// 1. Test the buffering -- read 4 bytes at a time
//
void 
TestSyckReadString( CuTest *tc )
{
    SyckParser *parser;
    char *tmp;
    int len = 0;

    parser = syck_new_parser();
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

//
// 2. Test parsing a simple string and handler
// 
SYMID
SyckParseStringHandler( SyckParser *p, SyckNode *n )
{
    if ( n->kind != syck_str_kind )
        return 100;

    if ( strcmp( syck_str_read( n ), "a_test_string" ) != 0 )
        return 200;

    return 1112;
}

void 
TestSyckParseString( CuTest *tc )
{
    SyckParser *parser;
    SYMID id;

    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "--- a_test_string", NULL );

    id = syck_parse( parser );
    CuAssert( tc, "Handler returned incorrect value.", 1112 == id );

    syck_free_parser( parser );
}

//
// 3.  
//
SYMID
SyckParseString2Handler( SyckParser *p, SyckNode *n )
{
    if ( n->kind != syck_str_kind )
        return 100;

    if ( strcmp( syck_str_read( n ), "a_test_string" ) != 0 )
        return 200;

    return 1112;
}

enum st_retval 
ListAnchors( char *key, SyckNode *n, CuTest *tc )
{
    char *sd = syck_strndup( n->data.str->ptr, n->data.str->len );
    CuAssertStrEquals( tc, "test", key );
    CuAssertStrEquals( tc, "13", sd );
    free( sd );
    return ST_CONTINUE;
}

void 
TestSyckParseString2( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "--- {test: 1, and: 2, or: &test 13}", NULL );
    syckparse( parser );
    st_foreach( parser->anchors, ListAnchors, tc );
    syck_free_parser( parser );
}

void 
TestSyckParseMap( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "\ntest: 1\nand: 2\nor:\n  test: 1\n  and: 2\n  fourdepth:\n    deep: 1\nlast: end", NULL );
    syck_parse( parser );
    syck_free_parser( parser );
}

void 
TestSyckParseFold( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "\ntest: |\n   deep: 1\nlast: end\n  \n", NULL );
    syck_parse( parser );
    syck_free_parser( parser );
}

void 
TestSyckParseMultidoc( CuTest *tc )
{
    SyckParser *parser;
    parser = syck_new_parser();
    syck_parser_handler( parser, SyckParseStringHandler );
    syck_parser_str_auto( parser, "---\ntest: |\n   deep: 1\n---\nlast: end\n  \n", NULL );
    syck_parse( parser );
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
    SUITE_ADD_TEST( suite, TestSyckParseMap );
    SUITE_ADD_TEST( suite, TestSyckParseFold );
    SUITE_ADD_TEST( suite, TestSyckParseMultidoc );
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

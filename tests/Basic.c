//
// Basic.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"
#include "CuTest.h"

//
// Test allocating a single node of kind 'str'.
//
void 
TestSyckNodeAlloc( CuTest *tc )
{
    SyckNode* n;

    n = syck_new_str( "YAML", scalar_plain );

    CuAssert( tc, "Allocated 'str' node reporting as 'seq'.", n->kind != syck_seq_kind );
    CuAssert( tc, "Allocated 'str' node reporting as 'map'.", n->kind != syck_map_kind );
    CuAssert( tc, "Allocated 'str' not reporting as 'str'.", n->kind == syck_str_kind );
    CuAssertStrEquals( tc, "YAML", syck_str_read( n ) );

    syck_free_node( n );
}

//
// Test building a simple sequence
//
void
TestSyckSeqAlloc( CuTest *tc )
{
    SyckNode *n;
    SYMID id;

    n = syck_new_seq( 1 );
    for ( id = 11001; id < 23000; id += 24 )
    {
        syck_seq_add( n, id );
    }

    CuAssert( tc, "Invalid value at '0'", 1 == syck_seq_read( n, 0 ) );
    CuAssert( tc, "Invalid value at '1'", 11001 == syck_seq_read( n, 1 ) );
    CuAssert( tc, "Invalid value at '200'", 15801 == syck_seq_read( n, 201 ) );

    syck_free_node( n );
}

//
// Test building a simple map
//
void
TestSyckMapAlloc( CuTest *tc )
{
    SyckNode *n;

    n = syck_new_map( 24556, 24557 );
    syck_map_add( n, 24558, 24559 );
    syck_map_add( n, 24658, 24659 );
    syck_map_add( n, 24758, 24759 );
    syck_map_add( n, 24858, 24859 );
    syck_map_add( n, 24958, 24959 );
    syck_map_add( n, 24058, 24059 );
    syck_map_add( n, 24158, 24159 );

    CuAssert( tc, "Invalid key at '0'.", 24556 == syck_map_read( n, map_key, 0 ) );
    CuAssert( tc, "Invalid key at '1'.", 24558 == syck_map_read( n, map_key, 1 ) );
    CuAssert( tc, "Invalid key at '2'.", 24658 == syck_map_read( n, map_key, 2 ) );
    CuAssert( tc, "Invalid key at '3'.", 24758 == syck_map_read( n, map_key, 3 ) );
    CuAssert( tc, "Invalid key at '4'.", 24858 == syck_map_read( n, map_key, 4 ) );
    CuAssert( tc, "Invalid key at '5'.", 24958 == syck_map_read( n, map_key, 5 ) );
    CuAssert( tc, "Invalid key at '6'.", 24058 == syck_map_read( n, map_key, 6 ) );
    CuAssert( tc, "Invalid key at '7'.", 24158 == syck_map_read( n, map_key, 7 ) );
    CuAssert( tc, "Invalid value at '0'", 24557 == syck_map_read( n, map_value, 0 ) );
    CuAssert( tc, "Invalid value at '1'", 24559 == syck_map_read( n, map_value, 1 ) );
    CuAssert( tc, "Invalid value at '2'", 24659 == syck_map_read( n, map_value, 2 ) );
    CuAssert( tc, "Invalid value at '3'", 24759 == syck_map_read( n, map_value, 3 ) );
    CuAssert( tc, "Invalid value at '4'", 24859 == syck_map_read( n, map_value, 4 ) );
    CuAssert( tc, "Invalid value at '5'", 24959 == syck_map_read( n, map_value, 5 ) );
    CuAssert( tc, "Invalid value at '6'", 24059 == syck_map_read( n, map_value, 6 ) );
    CuAssert( tc, "Invalid value at '7'", 24159 == syck_map_read( n, map_value, 7 ) );

    syck_free_node( n );
}

//
// Test building a simple map
//
void
TestSyckMapUpdate( CuTest *tc )
{
    SyckNode *n1, *n2;

    n1 = syck_new_map( 51116, 51117 );
    syck_map_add( n1, 51118, 51119 );
    n2 = syck_new_map( 51126, 51127 );
    syck_map_add( n2, 51128, 51129 );

    syck_map_update( n1, n2 );
    CuAssert( tc, "Invalid key at '2'", 51126 == syck_map_read( n1, map_key, 2 ) );
    CuAssert( tc, "Invalid key at '3'", 51128 == syck_map_read( n1, map_key, 3 ) );
    CuAssert( tc, "Invalid value at '2'", 51127 == syck_map_read( n1, map_value, 2 ) );
    CuAssert( tc, "Invalid value at '3'", 51129 == syck_map_read( n1, map_value, 3 ) );

    syck_free_node( n2 );
    syck_free_node( n1 );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckNodeAlloc );
    SUITE_ADD_TEST( suite, TestSyckSeqAlloc );
    SUITE_ADD_TEST( suite, TestSyckMapAlloc );
    SUITE_ADD_TEST( suite, TestSyckMapUpdate );
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

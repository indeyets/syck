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
    struct SyckNode* n;

    n = syck_new_str( "YAML" );

    CuAssert( tc, "Allocated 'str' node reporting as 'seq'.", n->kind != syck_seq_kind );
    CuAssert( tc, "Allocated 'str' node reporting as 'map'.", n->kind != syck_map_kind );
    CuAssert( tc, "Allocated 'str' not reporting as 'str'.", n->kind == syck_str_kind );
    CuAssertStrEquals( tc, "YAML", syck_str_read( n ) );

    free( n );
}

//
// Test building a simple sequence
//
void
TestSyckSeqAlloc( CuTest *tc )
{
    struct SyckNode *n;
    SYMID id;

    n = syck_new_seq( 1 );
    for ( id = 11001; id < 23000; id += 24 )
    {
        syck_seq_add( n, id );
    }

    CuAssert( tc, "Invalid value at '0'", 1 == syck_seq_read( n, 0 ) );
    CuAssert( tc, "Invalid value at '1'", 11001 == syck_seq_read( n, 1 ) );
    CuAssert( tc, "Invalid value at '200'", 15801 == syck_seq_read( n, 201 ) );

    free( n );
}

//
// Test building a simple map
//
void
TestSyckMapAlloc( CuTest *tc )
{
    struct SyckNode *n;

    n = syck_new_map( 24556, 24557 );
    syck_map_add( n, 24558, 24559 );

    CuAssert( tc, "Invalid key at '0'.", 24556 == syck_map_read( n, map_key, 0 ) );
    CuAssert( tc, "Invalid key at '1'.", 24558 == syck_map_read( n, map_key, 1 ) );
    CuAssert( tc, "Invalid value at '0'", 24557 == syck_map_read( n, map_value, 0 ) );
    CuAssert( tc, "Invalid value at '1'", 24559 == syck_map_read( n, map_value, 1 ) );

    free( n );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckNodeAlloc );
    SUITE_ADD_TEST( suite, TestSyckSeqAlloc );
    SUITE_ADD_TEST( suite, TestSyckMapAlloc );
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

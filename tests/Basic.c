//
// Basic.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"
#include "st.h"
#include "CuTest.h"

//
// Test allocating a single node of kind 'str'.
//
void 
TestSyckNodeAlloc( CuTest *tc )
{
    struct SyckNode* n;

    n = new_str_node( "YAML" );

    CuAssert( tc, "Allocated 'str' node reporting as 'seq'.", n->kind != seq_kind );
    CuAssert( tc, "Allocated 'str' node reporting as 'map'.", n->kind != map_kind );
    CuAssert( tc, "Allocated 'str' not reporting as 'str'.", n->kind == str_kind );
    CuAssertStrEquals( tc, "YAML", read_str_node( n ) );

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

    n = new_seq_node( 1 );
    for ( id = 11001; id < 23000; id += 24 )
    {
        add_seq_item( n, id );
    }

    CuAssert( tc, "Invalid value at '0'", 1 == read_seq_node( n, 0 ) );
    CuAssert( tc, "Invalid value at '1'", 11001 == read_seq_node( n, 1 ) );
    CuAssert( tc, "Invalid value at '200'", 15801 == read_seq_node( n, 201 ) );

    free( n );
}

//
// Test building a simple map
//
void
TestSyckMapAlloc( CuTest *tc )
{
    struct SyckNode *n;

    n = new_map_node( 24556, 24557 );
    add_map_pair( n, 24558, 24559 );

    CuAssert( tc, "Invalid key at '0'.", 24556 == read_map_node( n, map_key, 0 ) );
    CuAssert( tc, "Invalid key at '1'.", 24558 == read_map_node( n, map_key, 1 ) );
    CuAssert( tc, "Invalid value at '0'", 24557 == read_map_node( n, map_value, 0 ) );
    CuAssert( tc, "Invalid value at '1'", 24559 == read_map_node( n, map_value, 1 ) );

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

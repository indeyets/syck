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
    struct SyckNode *n, *n1, *n2;

    n1 = new_str_node( "ONE" );
    n2 = new_str_node( "TWO" );
    n = new_seq_node( n1 );
    add_seq_item( n, n2 );

    CuAssertStrEquals( tc, "ONE", read_str_node( read_seq_node( n, 0 ) ) );
    CuAssertStrEquals( tc, "TWO", read_str_node( read_seq_node( n, 1 ) ) );

    free( n2 );
    free( n1 );
    free( n );
}

//
// Test building a simple map
//
void
TestSyckMapAlloc( CuTest *tc )
{
    struct SyckNode *n, *n1, *n2;

    n1 = new_str_node( "KEY" );
    n2 = new_str_node( "VALUE" );
    n = new_map_node( n1, n2 );

    CuAssertStrEquals( tc, "VALUE", read_str_node( read_map_node( n, map_value, 0 ) ) );
    CuAssertStrEquals( tc, "KEY", read_str_node( read_map_node( n, map_key, 0 ) ) );

    free( n2 );
    free( n1 );
    free( n );
}

//
// Test building a large array
//
void
TestSyckLargeSeqAlloc( CuTest *tc )
{
    struct SyckNode *n;
    int i;
    char *list[] = { 
        "ONE", "TWO", "THREE", "FOUR", "FIVE",
        "SIX", "SEVEN", "EIGHT", "NINE", "TEN",
        "ELEVEN", "TWELVE", "THIRTEEN", "FOURTEEN", "FIFTEEN",
        "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN", "TWENTY"
    };

    n = new_seq_node( new_str_node( "ZERO" ) );
    for ( i = 0; i < 20; i++ )
    {
        add_seq_item( n, new_str_node( list[i] ) );
    }

    CuAssertStrEquals( tc, "THIRTEEN", read_str_node( read_seq_node( n, 13 ) ) );
    CuAssertStrEquals( tc, "NINETEEN", read_str_node( read_seq_node( n, 19 ) ) );

    free_node_deeply( n );
}

//
// Test a nested sequence and map
//
void
TestSyckNestedAlloc( CuTest *tc )
{
    struct SyckNode *n;

    n = new_map_node( new_str_node( "KEY1" ), new_str_node( "VALUE1" ) );
    add_map_pair( n, new_str_node( "KEY2" ), 
            new_seq_node( new_str_node( "A" ) ) );

    CuAssertStrEquals( tc, "A", read_str_node( read_seq_node( read_map_node( n, map_value, 1 ), 0 ) ) );

    free_node_deeply( n );
}


CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckNodeAlloc );
    SUITE_ADD_TEST( suite, TestSyckSeqAlloc );
    SUITE_ADD_TEST( suite, TestSyckMapAlloc );
    SUITE_ADD_TEST( suite, TestSyckLargeSeqAlloc );
    SUITE_ADD_TEST( suite, TestSyckNestedAlloc );
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

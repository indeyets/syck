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
    CuAssertStrEquals( tc, "YAML", n->data.str );

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

    CuAssertStrEquals( tc, "VALUE", n->data.pairs->values[0]->data.str );

    free( n2 );
    free( n1 );
    free( n );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckNodeAlloc );
    SUITE_ADD_TEST( suite, TestSyckMapAlloc );
    return suite;
}

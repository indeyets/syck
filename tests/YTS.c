//
// YTS.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//
// Well, this is the Yaml Testing Suite in the form of a plain C
// API.  Basically, this is as good as C integration gets for Syck.
// You've got to have a symbol table around.  From there, you can
// query your data.
//

#include <string.h>
#include "syck.h"
#include "CuTest.h"

/* YAML test node structures */
#define T_STR 10
#define T_SEQ 20
#define T_MAP 30
#define T_END 40
#define ILEN  2

struct test_node {
    int type;
    char *tag;
    char *key;
    struct test_node *value;
};
struct test_node end_node = { T_END };

/*
 * Assertion which compares a YAML document with an
 * equivalent set of test_node structs.
 */
SYMID
syck_copy_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    int i = 0;
    struct test_node *tn = S_ALLOC_N( struct test_node, 1 );

    switch ( n->kind )
    {
        case syck_str_kind:
            tn->type = T_STR;
            tn->key = syck_strndup( n->data.str->ptr, n->data.str->len );
            tn->value = 0;
        break;

        case syck_seq_kind:
        {
            struct test_node *val;
            struct test_node *seq = S_ALLOC_N( struct test_node, n->data.list->idx + 1 );
            tn->type = T_SEQ;
            tn->key = 0;
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                SYMID oid = syck_seq_read( n, i );
                syck_lookup_sym( p, oid, (char **)&val );
                seq[i] = val[0];
            }
            seq[n->data.list->idx] = end_node;
            tn->value = seq;
        }
        break;

        case syck_map_kind:
        {
            struct test_node *val;
            struct test_node *map = S_ALLOC_N( struct test_node, ( n->data.pairs->idx * 2 ) + 1 );
            tn->type = T_MAP;
            tn->key = 0;
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                SYMID oid = syck_map_read( n, map_key, i );
                syck_lookup_sym( p, oid, (char **)&val );
                map[i * 2] = val[0];

                oid = syck_map_read( n, map_value, i );
                syck_lookup_sym( p, oid, (char **)&val );
                map[(i * 2) + 1] = val[0];
            }
            map[n->data.pairs->idx * 2] = end_node;
            tn->value = map;
        }
        break;
    }

    tn->tag = 0;
    if ( n->type_id != NULL )
        tn->tag = syck_strndup( n->type_id, strlen( n->type_id ) );

    return syck_add_sym( p, (char *) tn );
}

int
syck_free_copies( char *key, struct test_node *tn, char *arg )
{
    if ( tn != NULL ) {
        switch ( tn->type ) {
            case T_STR:
                S_FREE( tn->key );
            break;

            case T_SEQ:
            case T_MAP:
                S_FREE( tn->value );
            break;
        }
        if ( tn->tag != NULL ) S_FREE( tn->tag );
        S_FREE( tn );
    }
    tn = NULL;
    return ST_CONTINUE;
}

/* Dislay node tree */
void p_indent( int i ) {
    int j;
    for ( j = 0; j < i; j++ ) {
        printf( " " );
    }
}

void p( struct test_node *tn, int depth ) {
    int i = 0;
    while ( tn[i].type != T_END ) {
        switch ( tn[i].type ) {
            case T_STR:
                p_indent( depth );
                printf( "%s\n", tn[i].key );
            break;
            case T_SEQ:
                p_indent( depth );
                printf( "SEQ\n" );
                p( tn[i].value, depth + ILEN );
            break;
            case T_MAP:
                p_indent( depth );
                printf( "MAP\n" );
                p( tn[i].value, depth + ILEN );
            break;
        }
        i++;
    }
}

void CuStreamCompareX( CuTest* tc, struct test_node *s1, struct test_node *s2 ) {
    int i = 0;
    while ( 1 ) {
        CuAssertIntEquals( tc, s1[i].type, s2[i].type );
        if ( s1[i].type == T_END ) return;
        switch ( s1[i].type ) {
            case T_STR:
                CuAssertStrEquals( tc, s1[i].key, s2[i].key );
            break;
            case T_SEQ:
            case T_MAP:
                CuStreamCompareX( tc, s1[i].value, s2[i].value );
            break;
        }
        i++;
    }
}

void CuStreamCompare( CuTest* tc, char *yaml, struct test_node *stream ) {
    int doc_ct = 0;
    struct test_node *ystream = S_ALLOC_N( struct test_node, doc_ct + 1 );
    CuString *msg;

    /* Set up parser */
    SyckParser *parser = syck_new_parser();
    syck_parser_str_auto( parser, yaml, NULL );
    syck_parser_handler( parser, syck_copy_handler );
    syck_parser_error_handler( parser, NULL );
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 1 );

    /* Parse all streams */
    while ( 1 )
    {
        struct test_node *ydoc;
        SYMID oid = syck_parse( parser );
        if ( parser->eof == 1 ) break;

        /* Add document to stream */
        syck_lookup_sym( parser, oid, (char **)&ydoc );
        ystream[doc_ct] = ydoc[0];
        doc_ct++;
        S_REALLOC_N( ystream, struct test_node, doc_ct + 1 );
    }
    ystream[doc_ct] = end_node;

    /* Traverse the struct and the symbol table side-by-side */
    /* DEBUG: p( stream, 0 ); p( ystream, 0 ); */
    CuStreamCompareX( tc, stream, ystream );

    /* Free the node tables and the parser */
    S_FREE( ystream );
    if ( parser->syms != NULL )
        st_foreach( parser->syms, syck_free_copies, 0 );
    syck_free_parser( parser );
}

/*
 * ACTUAL TESTS FOR THE YAML TESTING SUITE BEGIN HERE
 *   (EVERYTHING PREVIOUS WAS SET UP FOR THE TESTS)
 */

/*
 * Example 2.1: Sequence of scalars
 */
void
TestSyckSpecExample_2_1( CuTest *tc )
{
    struct test_node seq[] = {
        { T_STR, 0, "Mark McGwire" },
        { T_STR, 0, "Sammy Sosa" },
        { T_STR, 0, "Ken Griffey" },
        end_node
    };
    struct test_node stream[] = {
        { T_SEQ, 0, 0, seq },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "---\n"
        "- Mark McGwire\n"
        "- Sammy Sosa\n"
        "- Ken Griffey\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example 2.2: Mapping of scalars to scalars
 */
void
TestSyckSpecExample_2_2( CuTest *tc )
{
    struct test_node map[] = {
        { T_STR, 0, "hr" },
            { T_STR, 0, "65" },
        { T_STR, 0, "avg" },
            { T_STR, 0, "0.278" },
        { T_STR, 0, "rbi" },
            { T_STR, 0, "147" },
        end_node
    };
    struct test_node stream[] = {
        { T_MAP, 0, 0, map },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "---\n"
        "hr: 65\n"
        "avg: 0.278\n"
        "rbi: 147\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example 2.3: Mapping of scalars to sequences
 */
void
TestSyckSpecExample_2_3( CuTest *tc )
{
    struct test_node seq1[] = {
        { T_STR, 0, "Boston Red Sox" },
        { T_STR, 0, "Detroit Tigers" },
        { T_STR, 0, "New York Yankees" },
        end_node
    };
    struct test_node seq2[] = {
        { T_STR, 0, "New York Mets" },
        { T_STR, 0, "Chicago Cubs" },
        { T_STR, 0, "Atlanta Braves" },
        end_node
    };
    struct test_node map[] = {
        { T_STR, 0, "american" },
            { T_SEQ, 0, 0, seq1 },
        { T_STR, 0, "national" },
            { T_SEQ, 0, 0, seq2 },
        end_node
    };
    struct test_node stream[] = {
        { T_MAP, 0, 0, map },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "american:\n"
        "   - Boston Red Sox\n"
        "   - Detroit Tigers\n"
        "   - New York Yankees\n"
        "national:\n"
        "   - New York Mets\n"
        "   - Chicago Cubs\n"
        "   - Atlanta Braves\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example 2.4: Sequence of mappings
 */
void
TestSyckSpecExample_2_4( CuTest *tc )
{
    struct test_node map1[] = {
        { T_STR, 0, "name" },
            { T_STR, 0, "Mark McGwire" },
        { T_STR, 0, "hr" },
            { T_STR, 0, "65" },
        { T_STR, 0, "avg" },
            { T_STR, 0, "0.278" },
        end_node
    };
    struct test_node map2[] = {
        { T_STR, 0, "name" },
            { T_STR, 0, "Sammy Sosa" },
        { T_STR, 0, "hr" },
            { T_STR, 0, "63" },
        { T_STR, 0, "avg" },
            { T_STR, 0, "0.288" },
        end_node
    };
    struct test_node seq[] = {
        { T_MAP, 0, 0, map1 },
        { T_MAP, 0, 0, map2 },
        end_node
    };
    struct test_node stream[] = {
        { T_SEQ, 0, 0, seq },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "- \n"
        "  name: Mark McGwire\n"
        "  hr:   65\n"
        "  avg:  0.278\n"
        "- \n"
        "  name: Sammy Sosa\n"
        "  hr:   63\n"
        "  avg:  0.288\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example Legacy A5: Complex keys
 */
void
TestSyckSpecExample_Legacy_A5( CuTest *tc )
{
    struct test_node seq1[] = {
        { T_STR, 0, "New York Yankees" },
        { T_STR, 0, "Atlanta Braves" },
        end_node
    };
    struct test_node seq2[] = {
        { T_STR, 0, "2001-07-02" },
        { T_STR, 0, "2001-08-12" },
        { T_STR, 0, "2001-08-14" },
        end_node
    };
    struct test_node seq3[] = {
        { T_STR, 0, "Detroit Tigers" },
        { T_STR, 0, "Chicago Cubs" },
        end_node
    };
    struct test_node seq4[] = {
        { T_STR, 0, "2001-07-23" },
        end_node
    };
    struct test_node map[] = {
        { T_SEQ, 0, 0, seq1 },
        { T_SEQ, 0, 0, seq2 },
        { T_SEQ, 0, 0, seq3 },
        { T_SEQ, 0, 0, seq4 },
        end_node
    };
    struct test_node stream[] = {
        { T_MAP, 0, 0, map },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "?\n"
        "    - New York Yankees\n"
        "    - Atlanta Braves\n"
        ":\n"
        "  - 2001-07-02\n"
        "  - 2001-08-12\n"
        "  - 2001-08-14\n"
        "?\n"
        "    - Detroit Tigers\n"
        "    - Chicago Cubs\n"
        ":\n"
        "  - 2001-07-23\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example 2.5: Sequence of sequences
 */
void
TestSyckSpecExample_2_5( CuTest *tc )
{
    struct test_node seq1[] = {
        { T_STR, 0, "name" },
        { T_STR, 0, "hr" },
        { T_STR, 0, "avg" },
        end_node
    };
    struct test_node seq2[] = {
        { T_STR, 0, "Mark McGwire" },
        { T_STR, 0, "65" },
        { T_STR, 0, "0.278" },
        end_node
    };
    struct test_node seq3[] = {
        { T_STR, 0, "Sammy Sosa" },
        { T_STR, 0, "63" },
        { T_STR, 0, "0.288" },
        end_node
    };
    struct test_node seq[] = {
        { T_SEQ, 0, 0, seq1 },
        { T_SEQ, 0, 0, seq2 },
        { T_SEQ, 0, 0, seq3 },
        end_node
    };
    struct test_node stream[] = {
        { T_SEQ, 0, 0, seq },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "- [ name         , hr , avg   ]\n"
        "- [ Mark McGwire , 65 , 0.278 ]\n"
        "- [ Sammy Sosa   , 63 , 0.288 ]\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example 2.6: Mapping of mappings
 */
void
TestSyckSpecExample_2_6( CuTest *tc )
{
    struct test_node map1[] = {
        { T_STR, 0, "hr" },
            { T_STR, 0, "65" },
        { T_STR, 0, "avg" },
            { T_STR, 0, "0.278" },
        end_node
    };
    struct test_node map2[] = {
        { T_STR, 0, "hr" },
            { T_STR, 0, "63" },
        { T_STR, 0, "avg" },
            { T_STR, 0, "0.288" },
        end_node
    };
    struct test_node map[] = {
        { T_STR, 0, "Mark McGwire" },
            { T_MAP, 0, 0, map1 },
        { T_STR, 0, "Sammy Sosa" },
            { T_MAP, 0, 0, map2 },
        end_node
    };
    struct test_node stream[] = {
        { T_MAP, 0, 0, map },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "Mark McGwire: {hr: 65, avg: 0.278}\n"
        "Sammy Sosa: {\n"
        "    hr: 63,\n"
        "    avg: 0.288\n"
        "  }\n",

        /* C structure of validations */
        stream
    );
}

/*
 * Example 2.7: Two documents in a stream each
 *              with a leading comment
 */
void
TestSyckSpecExample_2_7( CuTest *tc )
{
    struct test_node seq1[] = {
        { T_STR, 0, "Mark McGwire" },
        { T_STR, 0, "Sammy Sosa" },
        { T_STR, 0, "Ken Griffey" },
        end_node
    };
    struct test_node seq2[] = {
        { T_STR, 0, "Chicago Cubs" },
        { T_STR, 0, "St Louis Cardinals" },
        end_node
    };
    struct test_node stream[] = {
        { T_SEQ, 0, 0, seq1 },
        { T_SEQ, 0, 0, seq2 },
        end_node
    };

    CuStreamCompare( tc,

        /* YAML document */ 
        "# Ranking of 1998 home runs\n"
        "---\n"
        "- Mark McGwire\n"
        "- Sammy Sosa\n"
        "- Ken Griffey\n"
        "\n"
        "# Team ranking\n"
        "---\n"
        "- Chicago Cubs\n"
        "- St Louis Cardinals\n",

        /* C structure of validations */
        stream
    );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_1 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_2 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_3 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_4 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_Legacy_A5 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_5 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_6 );
    SUITE_ADD_TEST( suite, TestSyckSpecExample_2_7 );
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

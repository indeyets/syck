//
// YTS.c
//
// $Author$
// $Date$
//
// Copyright (C) 2004 why the lucky stiff
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
    if ( n->type_id != NULL ) {
        tn->tag = syck_strndup( n->type_id, strlen( n->type_id ) );
    }

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

void CuStreamCompareX( CuTest* tc, struct test_node *s1, struct test_node *s2 ) {
    int i = 0;
    while ( 1 ) {
        CuAssertIntEquals( tc, s1[i].type, s2[i].type );
        if ( s1[i].type == T_END ) return;
        if ( s1[i].tag != 0 && s2[i].tag != 0 ) CuAssertStrEquals( tc, s1[i].tag, s2[i].tag );
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
        int res = syck_lookup_sym( parser, oid, (char **)&ydoc );
        if (0 == res)
            break;

        ystream[doc_ct] = ydoc[0];
        doc_ct++;
        S_REALLOC_N( ystream, struct test_node, doc_ct + 1 );
    }
    ystream[doc_ct] = end_node;

    /* Traverse the struct and the symbol table side-by-side */
    /* DEBUG: y( stream, 0 ); y( ystream, 0 ); */
    CuStreamCompareX( tc, stream, ystream );

    /* Free the node tables and the parser */
    S_FREE( ystream );
    if ( parser->syms != NULL )
        st_foreach( parser->syms, syck_free_copies, 0 );
    syck_free_parser( parser );
}

/*
 * Setup for testing N->Y->N.
 */
void 
test_output_handler( emitter, str, len )
    SyckEmitter *emitter;
    char *str;
    long len;
{
    CuString *dest = (CuString *)emitter->bonus;
    CuStringAppendLen( dest, str, len );
}

SYMID
build_symbol_table( SyckEmitter *emitter, struct test_node *node ) {
    switch ( node->type ) {
        case T_SEQ:
        case T_MAP:
        {
            int i = 0;
            while ( node->value[i].type != T_END ) {
                build_symbol_table( emitter, &node->value[i] );
                i++;
            }        
        }
        return syck_emitter_mark_node( emitter, (st_data_t)node );

        default: break;
    }
    return 0;
}

void
test_emitter_handler( SyckEmitter *emitter, st_data_t data ) {
    struct test_node *node = (struct test_node *)data;
    switch ( node->type ) {
        case T_STR:
            syck_emit_scalar( emitter, node->tag, scalar_none, 0, 0, 0, node->key, strlen( node->key ) );
        break;
        case T_SEQ:
        {
            int i = 0;
            syck_emit_seq( emitter, node->tag, seq_none );
            while ( node->value[i].type != T_END ) {
                syck_emit_item( emitter, (st_data_t)&node->value[i] );
                i++;
            }        
            syck_emit_end( emitter );
        }
        break;
        case T_MAP:
        {
            int i = 0;
            syck_emit_map( emitter, node->tag, map_none );
            while ( node->value[i].type != T_END ) {
                syck_emit_item( emitter, (st_data_t)&node->value[i] );
                i++;
            }        
            syck_emit_end( emitter );
        }
        break;
    }
}

void CuRoundTrip( CuTest* tc, struct test_node *stream ) {
    int i = 0;
    CuString *cs = CuStringNew();
    SyckEmitter *emitter = syck_new_emitter();

    /* Calculate anchors and tags */
    build_symbol_table( emitter, stream );

    /* Build the stream */
    syck_output_handler( emitter, test_output_handler );
    syck_emitter_handler( emitter, test_emitter_handler );
    emitter->bonus = cs;
    while ( stream[i].type != T_END )
    {
        syck_emit( emitter, (st_data_t)&stream[i] );
        syck_emitter_flush( emitter, 0 );
        i++;
    }

    /* Reload the stream and compare */
    /* printf( "-- output for %s --\n%s\n--- end of output --\n", tc->name, cs->buffer ); */
    CuStreamCompare( tc, cs->buffer, stream );
    CuStringFree( cs );

    syck_free_emitter( emitter );
}

/*
 * ACTUAL TESTS FOR THE YAML TESTING SUITE BEGIN HERE
 *   (EVERYTHING PREVIOUS WAS SET UP FOR THE TESTS)
 */

/*
 * Example : Trailing tab in plains
 */
void
YtsFoldedScalars_7( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "a" },
    { T_STR, 0, "b" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"a: b\t  \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Empty Sequence
 */
void
YtsNullsAndEmpties_0( CuTest *tc )
{
struct test_node seq[] = {
    end_node
};
struct test_node map[] = {
    { T_STR, 0, "empty" },
        { T_SEQ, 0, 0, seq },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"empty: [] \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Empty Mapping
 */
void
YtsNullsAndEmpties_1( CuTest *tc )
{
struct test_node map2[] = {
    end_node
};
struct test_node map1[] = {
    { T_STR, 0, "empty" },
        { T_MAP, 0, 0, map2 },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map1 },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"empty: {} \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Empty Sequence as Entire Document
 */
void
YtsNullsAndEmpties_2( CuTest *tc )
{
struct test_node seq[] = {
    end_node
};
struct test_node stream[] = {
    { T_SEQ, 0, 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- [] \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Empty Mapping as Entire Document
 */
void
YtsNullsAndEmpties_3( CuTest *tc )
{
struct test_node map[] = {
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- {} \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Null as Document
 */
void
YtsNullsAndEmpties_4( CuTest *tc )
{
struct test_node stream[] = {
    { T_STR, 0, "~" },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- ~ \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Empty String
 */
void
YtsNullsAndEmpties_5( CuTest *tc )
{
struct test_node stream[] = {
    { T_STR, 0, "" },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- '' \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.1: Sequence of scalars
 */
void
YtsSpecificationExamples_0( CuTest *tc )
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
"- Mark McGwire \n"
"- Sammy Sosa \n"
"- Ken Griffey \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.2: Mapping of scalars to scalars
 */
void
YtsSpecificationExamples_1( CuTest *tc )
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
"hr:  65 \n"
"avg: 0.278 \n"
"rbi: 147 \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.3: Mapping of scalars to sequences
 */
void
YtsSpecificationExamples_2( CuTest *tc )
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
"american: \n"
"   - Boston Red Sox \n"
"   - Detroit Tigers \n"
"   - New York Yankees \n"
"national: \n"
"   - New York Mets \n"
"   - Chicago Cubs \n"
"   - Atlanta Braves \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.4: Sequence of mappings
 */
void
YtsSpecificationExamples_3( CuTest *tc )
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
"-  \n"
"  name: Mark McGwire \n"
"  hr:   65 \n"
"  avg:  0.278 \n"
"-  \n"
"  name: Sammy Sosa \n"
"  hr:   63 \n"
"  avg:  0.288 \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example legacy_A5: Legacy A5
 */
void
YtsSpecificationExamples_4( CuTest *tc )
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
"? \n"
"    - New York Yankees \n"
"    - Atlanta Braves \n"
": \n"
"  - 2001-07-02 \n"
"  - 2001-08-12 \n"
"  - 2001-08-14 \n"
"? \n"
"    - Detroit Tigers \n"
"    - Chicago Cubs \n"
": \n"
"  - 2001-07-23 \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.5: Sequence of sequences
 */
void
YtsSpecificationExamples_5( CuTest *tc )
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
"- [ name         , hr , avg   ] \n"
"- [ Mark McGwire , 65 , 0.278 ] \n"
"- [ Sammy Sosa   , 63 , 0.288 ] \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.6: Mapping of mappings
 */
void
YtsSpecificationExamples_6( CuTest *tc )
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
"  }\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.7: Two documents in a stream each with a leading comment
 */
void
YtsSpecificationExamples_7( CuTest *tc )
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
"- St Louis Cardinals\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.8: Play by play feed from a game
 */
void
YtsSpecificationExamples_8( CuTest *tc )
{
struct test_node map1[] = {
    { T_STR, 0, "time" },
        { T_STR, 0, "20:03:20" },
    { T_STR, 0, "player" },
        { T_STR, 0, "Sammy Sosa" },
    { T_STR, 0, "action" },
        { T_STR, 0, "strike (miss)" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "time" },
        { T_STR, 0, "20:03:47" },
    { T_STR, 0, "player" },
        { T_STR, 0, "Sammy Sosa" },
    { T_STR, 0, "action" },
        { T_STR, 0, "grand slam" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"---\n"
"time: 20:03:20\n"
"player: Sammy Sosa\n"
"action: strike (miss)\n"
"...\n"
"---\n"
"time: 20:03:47\n"
"player: Sammy Sosa\n"
"action: grand slam\n"
"...\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.9: Single document with two comments
 */
void
YtsSpecificationExamples_9( CuTest *tc )
{
struct test_node seq1[] = {
    { T_STR, 0, "Mark McGwire" },
    { T_STR, 0, "Sammy Sosa" },
    end_node
};
struct test_node seq2[] = {
    { T_STR, 0, "Sammy Sosa" },
    { T_STR, 0, "Ken Griffey" },
    end_node
};
struct test_node map[] = {
    { T_STR, 0, "hr" },
        { T_SEQ, 0, 0, seq1 },
    { T_STR, 0, "rbi" },
        { T_SEQ, 0, 0, seq2 },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"hr: # 1998 hr ranking \n"
"  - Mark McGwire \n"
"  - Sammy Sosa \n"
"rbi: \n"
"  # 1998 rbi ranking \n"
"  - Sammy Sosa \n"
"  - Ken Griffey \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.1: Node for Sammy Sosa appears twice in this document
 */
void
YtsSpecificationExamples_10( CuTest *tc )
{
struct test_node seq1[] = {
    { T_STR, 0, "Mark McGwire" },
    { T_STR, 0, "Sammy Sosa" },
    end_node
};
struct test_node seq2[] = {
    { T_STR, 0, "Sammy Sosa" },
    { T_STR, 0, "Ken Griffey" },
    end_node
};
struct test_node map[] = {
    { T_STR, 0, "hr" },
        { T_SEQ, 0, 0, seq1 },
    { T_STR, 0, "rbi" },
        { T_SEQ, 0, 0, seq2 },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"---\n"
"hr: \n"
"   - Mark McGwire \n"
"   # Following node labeled SS \n"
"   - &SS Sammy Sosa \n"
"rbi: \n"
"   - *SS # Subsequent occurance \n"
"   - Ken Griffey \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.11: Mapping between sequences
 */
void
YtsSpecificationExamples_11( CuTest *tc )
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
    { T_SEQ, 0, 0, seq3 },
    { T_SEQ, 0, 0, seq4 },
    { T_SEQ, 0, 0, seq1 },
    { T_SEQ, 0, 0, seq2 },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"? # PLAY SCHEDULE \n"
"  - Detroit Tigers \n"
"  - Chicago Cubs \n"
":   \n"
"  - 2001-07-23 \n"
"\n"
"? [ New York Yankees, \n"
"    Atlanta Braves ] \n"
": [ 2001-07-02, 2001-08-12,  \n"
"    2001-08-14 ] \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.12: Sequence key shortcut
 */
void
YtsSpecificationExamples_12( CuTest *tc )
{
struct test_node map1[] = {
    { T_STR, 0, "item" },
        { T_STR, 0, "Super Hoop" },
    { T_STR, 0, "quantity" },
        { T_STR, 0, "1" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "item" },
        { T_STR, 0, "Basketball" },
    { T_STR, 0, "quantity" },
        { T_STR, 0, "4" },
    end_node
};
struct test_node map3[] = {
    { T_STR, 0, "item" },
        { T_STR, 0, "Big Shoes" },
    { T_STR, 0, "quantity" },
        { T_STR, 0, "1" },
    end_node
};
struct test_node seq[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    { T_MAP, 0, 0, map3 },
    end_node
};
struct test_node stream[] = {
    { T_SEQ, 0, 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"---\n"
"# products purchased\n"
"- item    : Super Hoop\n"
"  quantity: 1\n"
"- item    : Basketball\n"
"  quantity: 4\n"
"- item    : Big Shoes\n"
"  quantity: 1\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.13: Literal perserves newlines
 */
void
YtsSpecificationExamples_13( CuTest *tc )
{
struct test_node stream[] = {
    { T_STR, 0, "\\//||\\/||\n// ||  ||_\n" },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# ASCII Art\n"
"--- | \n"
"  \\//||\\/||\n"
"  // ||  ||_\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.14: Folded treats newlines as a space
 */
void
YtsSpecificationExamples_14( CuTest *tc )
{
struct test_node stream[] = {
    { T_STR, 0, "Mark McGwire's year was crippled by a knee injury." },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"---\n"
"  Mark McGwire's\n"
"  year was crippled\n"
"  by a knee injury.\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.15: Newlines preserved for indented and blank lines
 */
void
YtsSpecificationExamples_15( CuTest *tc )
{
struct test_node stream[] = {
    { T_STR, 0, "Sammy Sosa completed another fine season with great stats.\n\n  63 Home Runs\n  0.288 Batting Average\n\nWhat a year!\n" },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- > \n"
" Sammy Sosa completed another\n"
" fine season with great stats.\n"
"\n"
"   63 Home Runs\n"
"   0.288 Batting Average\n"
"\n"
" What a year!\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.16: Indentation determines scope
 */
void
YtsSpecificationExamples_16( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "name" },
        { T_STR, 0, "Mark McGwire" },
    { T_STR, 0, "accomplishment" },
        { T_STR, 0, "Mark set a major league home run record in 1998.\n" },
    { T_STR, 0, "stats" },
        { T_STR, 0, "65 Home Runs\n0.278 Batting Average\n" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"name: Mark McGwire \n"
"accomplishment: > \n"
"   Mark set a major league\n"
"   home run record in 1998.\n"
"stats: | \n"
"   65 Home Runs\n"
"   0.278 Batting Average\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.18: Multiline flow scalars
 */
void
YtsSpecificationExamples_18( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "plain" },
        { T_STR, 0, "This unquoted scalar spans many lines." },
    { T_STR, 0, "quoted" },
        { T_STR, 0, "So does this quoted scalar.\n" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"plain:\n"
"  This unquoted scalar\n"
"  spans many lines.\n"
"\n"
"quoted: \"So does this\n"
"  quoted scalar.\\n\"\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.19: Integers
 */
void
YtsSpecificationExamples_19( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "canonical" },
        { T_STR, 0, "12345" },
    { T_STR, 0, "decimal" },
        { T_STR, 0, "+12,345" },
    { T_STR, 0, "sexagecimal" },
        { T_STR, 0, "3:25:45" },
    { T_STR, 0, "octal" },
        { T_STR, 0, "014" },
    { T_STR, 0, "hexadecimal" },
        { T_STR, 0, "0xC" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"canonical: 12345 \n"
"decimal: +12,345 \n"
"sexagecimal: 3:25:45\n"
"octal: 014 \n"
"hexadecimal: 0xC \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.2: Floating point
 */
void
YtsSpecificationExamples_20( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "canonical" },
        { T_STR, 0, "1.23015e+3" },
    { T_STR, 0, "exponential" },
        { T_STR, 0, "12.3015e+02" },
    { T_STR, 0, "sexagecimal" },
        { T_STR, 0, "20:30.15" },
    { T_STR, 0, "fixed" },
        { T_STR, 0, "1,230.15" },
    { T_STR, 0, "negative infinity" },
        { T_STR, 0, "-.inf" },
    { T_STR, 0, "not a number" },
        { T_STR, 0, ".NaN" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"canonical: 1.23015e+3 \n"
"exponential: 12.3015e+02 \n"
"sexagecimal: 20:30.15\n"
"fixed: 1,230.15 \n"
"negative infinity: -.inf\n"
"not a number: .NaN \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.21: Miscellaneous
 */
void
YtsSpecificationExamples_21( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "null" },
        { T_STR, 0, "~" },
    { T_STR, 0, "true" },
        { T_STR, 0, "y" },
    { T_STR, 0, "false" },
        { T_STR, 0, "n" },
    { T_STR, 0, "string" },
        { T_STR, 0, "12345" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"null: ~ \n"
"true: y\n"
"false: n \n"
"string: '12345' \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.22: Timestamps
 */
void
YtsSpecificationExamples_22( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "canonical" },
        { T_STR, 0, "2001-12-15T02:59:43.1Z" },
    { T_STR, 0, "iso8601" },
        { T_STR, 0, "2001-12-14t21:59:43.10-05:00" },
    { T_STR, 0, "spaced" },
        { T_STR, 0, "2001-12-14 21:59:43.10 -05:00" },
    { T_STR, 0, "date" },
        { T_STR, 0, "2002-12-14" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"canonical: 2001-12-15T02:59:43.1Z\n"
"iso8601:  2001-12-14t21:59:43.10-05:00\n"
"spaced:  2001-12-14 21:59:43.10 -05:00\n"
"date:   2002-12-14 # Time is noon UTC\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example legacy D4: legacy Timestamps test
 */
void
YtsSpecificationExamples_23( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "canonical" },
        { T_STR, 0, "2001-12-15T02:59:43.00Z" },
    { T_STR, 0, "iso8601" },
        { T_STR, 0, "2001-02-28t21:59:43.00-05:00" },
    { T_STR, 0, "spaced" },
        { T_STR, 0, "2001-12-14 21:59:43.00 -05:00" },
    { T_STR, 0, "date" },
        { T_STR, 0, "2002-12-14" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"canonical: 2001-12-15T02:59:43.00Z\n"
"iso8601:  2001-02-28t21:59:43.00-05:00\n"
"spaced:  2001-12-14 21:59:43.00 -05:00\n"
"date:   2002-12-14\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.23: Various explicit families
 */
void
YtsSpecificationExamples_24( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "not-date" },
        { T_STR, "tag:yaml.org,2002:str", "2002-04-28" },
    { T_STR, 0, "picture" },
        { T_STR, "tag:yaml.org,2002:binary", "R0lGODlhDAAMAIQAAP//9/X\n17unp5WZmZgAAAOfn515eXv\nPz7Y6OjuDg4J+fn5OTk6enp\n56enmleECcgggoBADs=\n" },
    { T_STR, 0, "application specific tag" },
        { T_STR, "x-private:something", "The semantics of the tag\nabove may be different for\ndifferent documents.\n" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"not-date: !str 2002-04-28\n"
"picture: !binary |\n"
" R0lGODlhDAAMAIQAAP//9/X\n"
" 17unp5WZmZgAAAOfn515eXv\n"
" Pz7Y6OjuDg4J+fn5OTk6enp\n"
" 56enmleECcgggoBADs=\n"
"\n"
"application specific tag: !!something |\n"
" The semantics of the tag\n"
" above may be different for\n"
" different documents.\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.24: Application specific family
 */
void
YtsSpecificationExamples_25( CuTest *tc )
{
struct test_node point1[] = {
    { T_STR, 0, "x" },
        { T_STR, 0, "73" },
    { T_STR, 0, "y" },
        { T_STR, 0, "129" },
    end_node
};
struct test_node point2[] = {
    { T_STR, 0, "x" },
        { T_STR, 0, "89" },
    { T_STR, 0, "y" },
        { T_STR, 0, "102" },
    end_node
};
struct test_node map1[] = {
    { T_STR, 0, "center" },
        { T_MAP, 0, 0, point1 },
    { T_STR, 0, "radius" },
        { T_STR, 0, "7" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "start" },
        { T_MAP, 0, 0, point1 },
    { T_STR, 0, "finish" },
        { T_MAP, 0, 0, point2 },
    end_node
};
struct test_node map3[] = {
    { T_STR, 0, "start" },
        { T_MAP, 0, 0, point1 },
    { T_STR, 0, "color" },
        { T_STR, 0, "0xFFEEBB" },
    { T_STR, 0, "value" },
        { T_STR, 0, "Pretty vector drawing." },
    end_node
};
struct test_node seq[] = {
    { T_MAP, "tag:clarkevans.com,2002:graph/circle", 0, map1 },
    { T_MAP, "tag:clarkevans.com,2002:graph/line", 0, map2 },
    { T_MAP, "tag:clarkevans.com,2002:graph/label", 0, map3 },
    end_node
};
struct test_node stream[] = {
    { T_SEQ, "tag:clarkevans.com,2002:graph/shape", 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# Establish a tag prefix\n"
"--- !clarkevans.com,2002/graph/^shape\n"
"  # Use the prefix: shorthand for\n"
"  # !clarkevans.com,2002/graph/circle\n"
"- !^circle\n"
"  center: &ORIGIN {x: 73, 'y': 129}\n"
"  radius: 7\n"
"- !^line # !clarkevans.com,2002/graph/line\n"
"  start: *ORIGIN\n"
"  finish: { x: 89, 'y': 102 }\n"
"- !^label\n"
"  start: *ORIGIN\n"
"  color: 0xFFEEBB\n"
"  value: Pretty vector drawing.\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.26: Ordered mappings
 */
void
YtsSpecificationExamples_26( CuTest *tc )
{
struct test_node map1[] = {
    { T_STR, 0, "Mark McGwire" },
        { T_STR, 0, "65" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "Sammy Sosa" },
        { T_STR, 0, "63" },
    end_node
};
struct test_node map3[] = {
    { T_STR, 0, "Ken Griffy" },
        { T_STR, 0, "58" },
    end_node
};
struct test_node seq[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    { T_MAP, 0, 0, map3 },
    end_node
};
struct test_node stream[] = {
    { T_SEQ, "tag:yaml.org,2002:omap", 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# ordered maps are represented as\n"
"# a sequence of mappings, with\n"
"# each mapping having one key\n"
"--- !omap\n"
"- Mark McGwire: 65\n"
"- Sammy Sosa: 63\n"
"- Ken Griffy: 58\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.27: Invoice
 */
void
YtsSpecificationExamples_27( CuTest *tc )
{
struct test_node prod1[] = {
    { T_STR, 0, "sku" },
        { T_STR, 0, "BL394D" },
    { T_STR, 0, "quantity" },
        { T_STR, 0, "4" },
    { T_STR, 0, "description" },
        { T_STR, 0, "Basketball" },
    { T_STR, 0, "price" },
        { T_STR, 0, "450.00" },
    end_node
};
struct test_node prod2[] = {
    { T_STR, 0, "sku" },
        { T_STR, 0, "BL4438H" },
    { T_STR, 0, "quantity" },
        { T_STR, 0, "1" },
    { T_STR, 0, "description" },
        { T_STR, 0, "Super Hoop" },
    { T_STR, 0, "price" },
        { T_STR, 0, "2392.00" },
    end_node
};
struct test_node products[] = {
    { T_MAP, 0, 0, prod1 },
    { T_MAP, 0, 0, prod2 },
    end_node
};
struct test_node address[] = {
    { T_STR, 0, "lines" },
        { T_STR, 0, "458 Walkman Dr.\nSuite #292\n" },
    { T_STR, 0, "city" },
        { T_STR, 0, "Royal Oak" },
    { T_STR, 0, "state" },
        { T_STR, 0, "MI" },
    { T_STR, 0, "postal" },
        { T_STR, 0, "48046" },
    end_node
};
struct test_node id001[] = {
    { T_STR, 0, "given" },
        { T_STR, 0, "Chris" },
    { T_STR, 0, "family" },
        { T_STR, 0, "Dumars" },
    { T_STR, 0, "address" },
        { T_MAP, 0, 0, address },
    end_node
};
struct test_node map[] = {
    { T_STR, 0, "invoice" },
        { T_STR, 0, "34843" },
    { T_STR, 0, "date" },
        { T_STR, 0, "2001-01-23" },
    { T_STR, 0, "bill-to" },
        { T_MAP, 0, 0, id001 },
    { T_STR, 0, "ship-to" },
        { T_MAP, 0, 0, id001 },
    { T_STR, 0, "product" },
        { T_SEQ, 0, 0, products },
    { T_STR, 0, "tax" },
        { T_STR, 0, "251.42" }, 
    { T_STR, 0, "total" },
        { T_STR, 0, "4443.52" },
    { T_STR, 0, "comments" },
        { T_STR, 0, "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.\n" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, "tag:clarkevans.com,2002:invoice", 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- !clarkevans.com,2002/^invoice\n"
"invoice: 34843\n"
"date   : 2001-01-23\n"
"bill-to: &id001\n"
"    given  : Chris\n"
"    family : Dumars\n"
"    address:\n"
"        lines: |\n"
"            458 Walkman Dr.\n"
"            Suite #292\n"
"        city    : Royal Oak\n"
"        state   : MI\n"
"        postal  : 48046\n"
"ship-to: *id001\n"
"product:\n"
"    - sku         : BL394D\n"
"      quantity    : 4\n"
"      description : Basketball\n"
"      price       : 450.00\n"
"    - sku         : BL4438H\n"
"      quantity    : 1\n"
"      description : Super Hoop\n"
"      price       : 2392.00\n"
"tax  : 251.42\n"
"total: 4443.52\n"
"comments: >\n"
"  Late afternoon is best.\n"
"  Backup contact is Nancy\n"
"  Billsmer @ 338-4338.\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example 2.28: Log file
 */
void
YtsSpecificationExamples_28( CuTest *tc )
{
struct test_node map1[] = {
    { T_STR, 0, "Time" }, 
        { T_STR, 0, "2001-11-23 15:01:42 -05:00" },
    { T_STR, 0, "User" }, 
        { T_STR, 0, "ed" },
    { T_STR, 0, "Warning" }, 
        { T_STR, 0, "This is an error message for the log file\n" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "Time" }, 
        { T_STR, 0, "2001-11-23 15:02:31 -05:00" },
    { T_STR, 0, "User" }, 
        { T_STR, 0, "ed" },
    { T_STR, 0, "Warning" }, 
        { T_STR, 0, "A slightly different error message.\n" },
    end_node
};
struct test_node file1[] = {
    { T_STR, 0, "file" }, 
        { T_STR, 0, "TopClass.py" },
    { T_STR, 0, "line" }, 
        { T_STR, 0, "23" },
    { T_STR, 0, "code" }, 
        { T_STR, 0, "x = MoreObject(\"345\\n\")\n" },
    end_node
};
struct test_node file2[] = {
    { T_STR, 0, "file" }, 
        { T_STR, 0, "MoreClass.py" },
    { T_STR, 0, "line" }, 
        { T_STR, 0, "58" },
    { T_STR, 0, "code" }, 
        { T_STR, 0, "foo = bar" },
    end_node
};
struct test_node stack[] = {
    { T_MAP, 0, 0, file1 },
    { T_MAP, 0, 0, file2 },
    end_node
};
struct test_node map3[] = {
    { T_STR, 0, "Date" }, 
        { T_STR, 0, "2001-11-23 15:03:17 -05:00" },
    { T_STR, 0, "User" }, 
        { T_STR, 0, "ed" },
    { T_STR, 0, "Fatal" }, 
        { T_STR, 0, "Unknown variable \"bar\"\n" },
    { T_STR, 0, "Stack" }, 
        { T_SEQ, 0, 0, stack },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    { T_MAP, 0, 0, map3 },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"---\n"
"Time: 2001-11-23 15:01:42 -05:00\n"
"User: ed\n"
"Warning: >\n"
"  This is an error message\n"
"  for the log file\n"
"---\n"
"Time: 2001-11-23 15:02:31 -05:00\n"
"User: ed\n"
"Warning: >\n"
"  A slightly different error\n"
"  message.\n"
"---\n"
"Date: 2001-11-23 15:03:17 -05:00\n"
"User: ed\n"
"Fatal: >\n"
"  Unknown variable \"bar\"\n"
"Stack:\n"
"  - file: TopClass.py\n"
"    line: 23\n"
"    code: |\n"
"      x = MoreObject(\"345\\n\")\n"
"  - file: MoreClass.py\n"
"    line: 58\n"
"    code: |-\n"
"      foo = bar\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Throwaway comments
 */
void
YtsSpecificationExamples_29( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "this" },
        { T_STR, 0, "contains three lines of text.\nThe third one starts with a\n# character. This isn't a comment.\n" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"### These are four throwaway comment  ### \n"
"\n"
"### lines (the second line is empty). ### \n"
"this: |   # Comments may trail lines.\n"
"   contains three lines of text.\n"
"   The third one starts with a\n"
"   # character. This isn't a comment.\n"
"\n"
"# These are three throwaway comment\n"
"# lines (the first line is empty).\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Document with a single value
 */
void
YtsSpecificationExamples_30( CuTest *tc )
{
struct test_node stream[] = {
    { T_STR, 0, "This YAML stream contains a single text value. The next stream is a log file - a sequence of log entries. Adding an entry to the log is a simple matter of appending it at the end.\n" },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- > \n"
"This YAML stream contains a single text value.\n"
"The next stream is a log file - a sequence of\n"
"log entries. Adding an entry to the log is a\n"
"simple matter of appending it at the end.\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Document stream
 */
void
YtsSpecificationExamples_31( CuTest *tc )
{
struct test_node map1[] = {
    { T_STR, 0, "at" }, 
        { T_STR, 0, "2001-08-12 09:25:00.00 Z" },
    { T_STR, 0, "type" }, 
        { T_STR, 0, "GET" },
    { T_STR, 0, "HTTP" }, 
        { T_STR, 0, "1.0" },
    { T_STR, 0, "url" }, 
        { T_STR, 0, "/index.html" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "at" }, 
        { T_STR, 0, "2001-08-12 09:25:10.00 Z" },
    { T_STR, 0, "type" }, 
        { T_STR, 0, "GET" },
    { T_STR, 0, "HTTP" }, 
        { T_STR, 0, "1.0" },
    { T_STR, 0, "url" }, 
        { T_STR, 0, "/toc.html" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"--- \n"
"at: 2001-08-12 09:25:00.00 Z \n"
"type: GET \n"
"HTTP: '1.0' \n"
"url: '/index.html' \n"
"--- \n"
"at: 2001-08-12 09:25:10.00 Z \n"
"type: GET \n"
"HTTP: '1.0' \n"
"url: '/toc.html' \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Top level mapping
 */
void
YtsSpecificationExamples_32( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "invoice" }, 
        { T_STR, 0, "34843" },
    { T_STR, 0, "date" }, 
        { T_STR, 0, "2001-01-23" },
    { T_STR, 0, "total" }, 
        { T_STR, 0, "4443.52" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# This stream is an example of a top-level mapping. \n"
"invoice : 34843 \n"
"date    : 2001-01-23 \n"
"total   : 4443.52 \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Single-line documents
 */
void
YtsSpecificationExamples_33( CuTest *tc )
{
struct test_node map[] = {
    end_node
};
struct test_node seq[] = {
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    { T_SEQ, 0, 0, seq },
    { T_STR, 0, "" },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# The following is a sequence of three documents. \n"
"# The first contains an empty mapping, the second \n"
"# an empty sequence, and the last an empty string. \n"
"--- {} \n"
"--- [ ] \n"
"--- '' \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Document with pause
 */
void
YtsSpecificationExamples_34( CuTest *tc )
{
struct test_node map1[] = {
    { T_STR, 0, "sent at" },
        { T_STR, 0, "2002-06-06 11:46:25.10 Z" },
    { T_STR, 0, "payload" },
        { T_STR, 0, "Whatever" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "sent at" },
        { T_STR, 0, "2002-06-06 12:05:53.47 Z" },
    { T_STR, 0, "payload" },
        { T_STR, 0, "Whatever" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# A communication channel based on a YAML stream. \n"
"--- \n"
"sent at: 2002-06-06 11:46:25.10 Z \n"
"payload: Whatever \n"
"# Receiver can process this as soon as the following is sent: \n"
"... \n"
"# Even if the next message is sent long after: \n"
"--- \n"
"sent at: 2002-06-06 12:05:53.47 Z \n"
"payload: Whatever \n"
"... \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Explicit typing
 */
void
YtsSpecificationExamples_35( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "integer" },
        { T_STR, "tag:yaml.org,2002:int", "12" },
    { T_STR, 0, "also int" },
        { T_STR, "tag:yaml.org,2002:int", "12" },
    { T_STR, 0, "string" },
        { T_STR, "tag:yaml.org,2002:str", "12" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"integer: 12 \n"
"also int: ! \"12\" \n"
"string: !str 12 \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Private types
 */
void
YtsSpecificationExamples_36( CuTest *tc )
{
struct test_node pool[] = {
    { T_STR, 0, "number" },
        { T_STR, 0, "8" },
    { T_STR, 0, "color" },
        { T_STR, 0, "black" },
    end_node
};
struct test_node map1[] = {
    { T_STR, 0, "pool" },
        { T_MAP, "x-private:ball", 0, pool },
    end_node
};
struct test_node bearing[] = {
    { T_STR, 0, "material" },
        { T_STR, 0, "steel" },
    end_node
};
struct test_node map2[] = {
    { T_STR, 0, "bearing" },
        { T_MAP, "x-private:ball", 0, bearing },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map1 },
    { T_MAP, 0, 0, map2 },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# Both examples below make use of the 'x-private:ball' \n"
"# type family URI, but with different semantics. \n"
"--- \n"
"pool: !!ball \n"
"  number: 8 \n"
"  color: black \n"
"--- \n"
"bearing: !!ball \n"
"  material: steel \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Type family under yaml.org
 */
void
YtsSpecificationExamples_37( CuTest *tc )
{
struct test_node seq[] = {
    { T_STR, "tag:yaml.org,2002:str", "a Unicode string" },
    end_node
};
struct test_node stream[] = {
    { T_SEQ, 0, 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# The URI is 'tag:yaml.org,2002:str' \n"
"- !str a Unicode string \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Type family under perl.yaml.org
 */
void
YtsSpecificationExamples_38( CuTest *tc )
{
struct test_node map[] = {
    end_node
};
struct test_node seq[] = {
    { T_MAP, "tag:perl.yaml.org,2002:Text::Tabs", 0, map },
    end_node
};
struct test_node stream[] = {
    { T_SEQ, 0, 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# The URI is 'tag:perl.yaml.org,2002:Text::Tabs' \n"
"- !perl/Text::Tabs {} \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Type family under clarkevans.com
 */
void
YtsSpecificationExamples_39( CuTest *tc )
{
struct test_node map[] = {
    end_node
};
struct test_node seq[] = {
    { T_MAP, "tag:clarkevans.com,2003-02:timesheet", 0, map },
    end_node
};
struct test_node stream[] = {
    { T_SEQ, 0, 0, seq },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"# The URI is 'tag:clarkevans.com,2003-02:timesheet' \n"
"- !clarkevans.com,2003-02/timesheet {}\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : URI Escaping
 */
void
YtsSpecificationExamples_40( CuTest *tc )
{
struct test_node same[] = {
    { T_STR, "tag:domain.tld,2002:type0", "value" },
    { T_STR, "tag:domain.tld,2002:type0", "value" },
    end_node
};
struct test_node diff[] = {
    { T_STR, "tag:domain.tld,2002:type%30", "value" },
    { T_STR, "tag:domain.tld,2002:type0", "value" },
    end_node
};
struct test_node map[] = {
    { T_STR, 0, "same" },
        { T_SEQ, 0, 0, same },
    { T_STR, 0, "different" },
        { T_SEQ, 0, 0, diff },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"same: \n"
"  - !domain.tld,2002/type\\x30 value\n"
"  - !domain.tld,2002/type0 value\n"
"different: # As far as the YAML parser is concerned \n"
"  - !domain.tld,2002/type%30 value\n"
"  - !domain.tld,2002/type0 value\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Overriding anchors
 */
void
YtsSpecificationExamples_42( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "anchor" },
        { T_STR, 0, "This scalar has an anchor." },
    { T_STR, 0, "override" },
        { T_STR, 0, "The alias node below is a repeated use of this value.\n" },
    { T_STR, 0, "alias" },
        { T_STR, 0, "The alias node below is a repeated use of this value.\n" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"anchor : &A001 This scalar has an anchor. \n"
"override : &A001 >\n"
" The alias node below is a\n"
" repeated use of this value.\n"
"alias : *A001\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Flow and block formatting
 */
void
YtsSpecificationExamples_43( CuTest *tc )
{
struct test_node empty[] = {
    end_node
};
struct test_node flow[] = {
    { T_STR, 0, "one" },
    { T_STR, 0, "two" },
    { T_STR, 0, "three" },
    { T_STR, 0, "four" },
    { T_STR, 0, "five" },
    end_node
};
struct test_node inblock[] = {
    { T_STR, 0, "Subordinate sequence entry" },
    end_node
};
struct test_node block[] = {
    { T_STR, 0, "First item in top sequence" },
    { T_SEQ, 0, 0, inblock },
    { T_STR, 0, "A folded sequence entry\n" },
    { T_STR, 0, "Sixth item in top sequence" },
    end_node
};
struct test_node map[] = {
    { T_STR, 0, "empty" },
        { T_SEQ, 0, 0, empty },
    { T_STR, 0, "flow" },
        { T_SEQ, 0, 0, flow },
    { T_STR, 0, "block" },
        { T_SEQ, 0, 0, block },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"empty: [] \n"
"flow: [ one, two, three # May span lines, \n"
"         , four,           # indentation is \n"
"           five ]          # mostly ignored. \n"
"block: \n"
" - First item in top sequence \n"
" - \n"
"  - Subordinate sequence entry \n"
" - > \n"
"   A folded sequence entry\n"
" - Sixth item in top sequence \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Literal combinations
 */
void
YtsSpecificationExamples_47( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "empty" },
        { T_STR, 0, "" },
    { T_STR, 0, "literal" },
        { T_STR, 0, "The \\ ' \" characters may be\nfreely used. Leading white\n   space "
             "is significant.\n\nLine breaks are significant.\nThus this value contains one\n"
             "empty line and ends with a\nsingle line break, but does\nnot start with one.\n" },
    { T_STR, 0, "is equal to" },
        { T_STR, 0, "The \\ ' \" characters may be\nfreely used. Leading white\n   space "
             "is significant.\n\nLine breaks are significant.\nThus this value contains one\n"
             "empty line and ends with a\nsingle line break, but does\nnot start with one.\n" },
    { T_STR, 0, "indented and chomped" },
        { T_STR, 0, "  This has no newline." },
    { T_STR, 0, "also written as" },
        { T_STR, 0, "  This has no newline." },
    { T_STR, 0, "both are equal to" },
        { T_STR, 0, "  This has no newline." },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"empty: |\n"
"\n"
"literal: |\n"
" The \\ ' \" characters may be\n"
" freely used. Leading white\n"
"    space is significant.\n"
"\n"
" Line breaks are significant.\n"
" Thus this value contains one\n"
" empty line and ends with a\n"
" single line break, but does\n"
" not start with one.\n"
"\n"
"is equal to: \"The \\\\ ' \\\" characters may \\\n"
" be\\nfreely used. Leading white\\n   space \\\n"
" is significant.\\n\\nLine breaks are \\\n"
" significant.\\nThus this value contains \\\n"
" one\\nempty line and ends with a\\nsingle \\\n"
" line break, but does\\nnot start with one.\\n\"\n"
"\n"
"# Comments may follow a block \n"
"# scalar value. They must be \n"
"# less indented. \n"
"\n"
"# Modifiers may be combined in any order.\n"
"indented and chomped: |2-\n"
"    This has no newline.\n"
"\n"
"also written as: |-2\n"
"    This has no newline.\n"
"\n"
"both are equal to: \"  This has no newline.\"\n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}
/*
 * Example : Timestamp
 */
void
YtsSpecificationExamples_62( CuTest *tc )
{
struct test_node map[] = {
    { T_STR, 0, "canonical" },
        { T_STR, 0, "2001-12-15T02:59:43.1Z" },
    { T_STR, 0, "valid iso8601" },
        { T_STR, 0, "2001-12-14t21:59:43.10-05:00" },
    { T_STR, 0, "space separated" },
        { T_STR, 0, "2001-12-14 21:59:43.10 -05:00" },
    { T_STR, 0, "date (noon UTC)" },
        { T_STR, 0, "2002-12-14" },
    end_node
};
struct test_node stream[] = {
    { T_MAP, 0, 0, map },
    end_node
};

    CuStreamCompare( tc,

        /* YAML document */ 
"canonical:       2001-12-15T02:59:43.1Z \n"
"valid iso8601:   2001-12-14t21:59:43.10-05:00 \n"
"space separated: 2001-12-14 21:59:43.10 -05:00 \n"
"date (noon UTC): 2002-12-14 \n"
        ,

        /* C structure of validations */
        stream
    );

    CuRoundTrip( tc, stream );
}

CuSuite *
SyckGetSuite()
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST( suite, YtsFoldedScalars_7 );
    SUITE_ADD_TEST( suite, YtsNullsAndEmpties_0 );
    SUITE_ADD_TEST( suite, YtsNullsAndEmpties_1 );
    SUITE_ADD_TEST( suite, YtsNullsAndEmpties_2 );
    SUITE_ADD_TEST( suite, YtsNullsAndEmpties_3 );
    SUITE_ADD_TEST( suite, YtsNullsAndEmpties_4 );
    SUITE_ADD_TEST( suite, YtsNullsAndEmpties_5 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_0 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_1 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_2 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_3 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_4 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_5 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_6 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_7 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_8 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_9 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_10 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_11 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_12 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_13 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_14 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_15 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_16 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_18 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_19 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_20 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_21 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_22 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_23 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_24 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_25 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_26 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_27 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_28 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_29 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_30 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_31 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_32 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_33 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_34 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_35 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_36 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_37 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_38 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_39 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_40 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_42 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_43 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_47 );
    SUITE_ADD_TEST( suite, YtsSpecificationExamples_62 );
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

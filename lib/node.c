//
// node.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"

//
// Node allocation functions
//
struct SyckNode *
syck_alloc_node( enum syck_kind_tag type )
{
    struct SyckNode *s;

    s = ALLOC( struct SyckNode );
    s->kind = type;

    return s;
}

struct SyckNode *
syck_alloc_map()
{
    struct SyckNode *n;
    struct SyckMap *m;

    m = ALLOC( struct SyckMap );
    m->idx = 0;
    m->capa = ALLOC_CT;
    m->keys = ALLOC_N( SYMID, m->capa );
    m->values = ALLOC_N( SYMID, m->capa );

    n = syck_alloc_node( syck_map_kind );
    n->data.pairs = m;
    
    return n;
}

struct SyckNode *
syck_alloc_seq()
{
    struct SyckNode *n;
    struct SyckSeq *s;

    s = ALLOC( struct SyckSeq );
    s->idx = 0;
    s->capa = ALLOC_CT;
    s->items = ALLOC_N( SYMID, s->capa );

    n = syck_alloc_node( syck_seq_kind );
    n->data.list = s;

    return n;
}

struct SyckNode *
syck_alloc_str()
{
    return syck_alloc_node( syck_str_kind );
}

struct SyckNode *
syck_new_str( char *str )
{
    struct SyckNode *n;

    n = syck_alloc_str();
    n->data.str = str;

    return n;
}

char *
syck_str_read( struct SyckNode *n )
{
    ASSERT( n != NULL );
    return n->data.str;
}

struct SyckNode *
syck_new_map( SYMID key, SYMID value )
{
    struct SyckNode *n;

    n = syck_alloc_map();
    syck_map_add( n, key, value );

    return n;
}

void
syck_map_add( struct SyckNode *map, SYMID key, SYMID value )
{
    struct SyckMap *m;
    long idx;

    ASSERT( map != NULL );
    ASSERT( map->data.pairs != NULL );
    
    m = map->data.pairs;
    idx = m->idx;
    m->idx += 1;
    if ( m->idx > m->capa )
    {
        m->capa += ALLOC_CT;
        REALLOC_N( m->keys, SYMID, m->capa );
        REALLOC_N( m->values, SYMID, m->capa );
    }
    m->keys[idx] = key;
    m->values[idx] = value;
}

long
syck_map_count( struct SyckNode *map )
{
    ASSERT( map != NULL );
    ASSERT( map->data.pairs != NULL );
    return map->data.pairs->idx;
}

SYMID
syck_map_read( struct SyckNode *map, enum map_part p, long idx )
{
    struct SyckMap *m;

    ASSERT( map != NULL );
    m = map->data.pairs;
    ASSERT( m != NULL );
    if ( p == map_key )
    {
        return m->keys[idx];
    }
    else
    {
        return m->values[idx];
    }
}

struct SyckNode *
syck_new_seq( SYMID value )
{
    struct SyckNode *n;

    n = syck_alloc_seq();
    syck_seq_add( n, value );

    return n;
}

void
syck_seq_add( struct SyckNode *arr, SYMID value )
{
    struct SyckSeq *s;
    long idx;

    ASSERT( arr != NULL );
    ASSERT( arr->data.list != NULL );
    
    s = arr->data.list;
    idx = s->idx;
    s->idx += 1;
    if ( s->idx > s->capa )
    {
        s->capa += ALLOC_CT;
        REALLOC_N( s->items, SYMID, s->capa );
    }
    s->items[idx] = value;
}

long
syck_seq_count( struct SyckNode *seq )
{
    ASSERT( seq != NULL );
    ASSERT( seq->data.list != NULL );
    return seq->data.list->idx;
}

SYMID
syck_seq_read( struct SyckNode *seq, long idx )
{
    struct SyckSeq *s;

    ASSERT( seq != NULL );
    s = seq->data.list;
    ASSERT( s != NULL );
    return s->items[idx];
}

void
syck_free_members( struct SyckNode *n )
{
    int i;
    switch ( n->kind  )
    {
        case syck_seq_kind:
            free( n->data.list->items );
            free( n->data.list );
        break;

        case syck_map_kind:
            free( n->data.pairs->keys );
            free( n->data.pairs->values );
            free( n->data.pairs );
        break;
    }
}


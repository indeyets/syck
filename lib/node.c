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
alloc_node( enum syck_kind_tag type )
{
    struct SyckNode *s;

    s = ALLOC( struct SyckNode );
    s->kind = type;

    return s;
}

struct SyckNode *
alloc_map_node()
{
    struct SyckNode *n;
    struct SyckMap *m;

    m = ALLOC( struct SyckMap );
    m->idx = 0;
    m->capa = ALLOC_CT;
    m->keys = ALLOC_N( SYMID, m->capa );
    m->values = ALLOC_N( SYMID, m->capa );

    n = alloc_node( map_kind );
    n->data.pairs = m;
    
    return n;
}

struct SyckNode *
alloc_seq_node()
{
    struct SyckNode *n;
    struct SyckSeq *s;

    s = ALLOC( struct SyckSeq );
    s->idx = 0;
    s->capa = ALLOC_CT;
    s->items = ALLOC_N( SYMID, s->capa );

    n = alloc_node( seq_kind );
    n->data.list = s;

    return n;
}

struct SyckNode *
alloc_str_node()
{
    return alloc_node( str_kind );
}

struct SyckNode *
new_str_node( char *str )
{
    struct SyckNode *n;

    n = alloc_str_node();
    n->data.str = str;

    return n;
}

char *
read_str_node( struct SyckNode *n )
{
    ASSERT( n != NULL );
    return n->data.str;
}

struct SyckNode *
new_map_node( SYMID key, SYMID value )
{
    struct SyckNode *n;

    n = alloc_map_node();
    add_map_pair( n, key, value );

    return n;
}

void
add_map_pair( struct SyckNode *map, SYMID key, SYMID value )
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
read_map_count( struct SyckNode *map )
{
    ASSERT( map != NULL );
    ASSERT( map->data.pairs != NULL );
    return map->data.pairs->idx;
}

SYMID
read_map_node( struct SyckNode *map, enum map_part p, long idx )
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
new_seq_node( SYMID value )
{
    struct SyckNode *n;

    n = alloc_seq_node();
    add_seq_item( n, value );

    return n;
}

void
add_seq_item( struct SyckNode *arr, SYMID value )
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
read_seq_count( struct SyckNode *seq )
{
    ASSERT( seq != NULL );
    ASSERT( seq->data.list != NULL );
    return seq->data.list->idx;
}

SYMID
read_seq_node( struct SyckNode *seq, long idx )
{
    struct SyckSeq *s;

    ASSERT( seq != NULL );
    s = seq->data.list;
    ASSERT( s != NULL );
    return s->items[idx];
}

void
free_node_members( struct SyckNode *n )
{
    int i;
    switch ( n->kind  )
    {
        case seq_kind:
            free( n->data.list->items );
            free( n->data.list );
        break;

        case map_kind:
            free( n->data.pairs->keys );
            free( n->data.pairs->values );
            free( n->data.pairs );
        break;
    }
}


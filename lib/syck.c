//
// syck.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"

#define SYCK_YAML_MAJOR 1
#define SYCK_YAML_MINOR 0

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
    m->capa = 0;

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
    s->capa = 0;

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
    //assert( n != NULL );
    return n->data.str;
}

struct SyckNode *
new_map_node( struct SyckNode *key, struct SyckNode *value )
{
    struct SyckNode *n;

    n = alloc_map_node();
    add_map_pair( n, key, value );

    return n;
}

void
add_map_pair( struct SyckNode *map, struct SyckNode *key, struct SyckNode *value )
{
    struct SyckMap *m;
    long idx;

    //assert( map != NULL );
    //assert( map->data.pairs != NULL );
    
    m = map->data.pairs;
    idx = m->capa;
    m->capa += 1;
    REALLOC_N( m->keys, struct SyckNode *, m->capa );
    REALLOC_N( m->values, struct SyckNode *, m->capa );
    m->keys[idx] = key;
    m->values[idx] = value;
}

struct SyckNode *
read_map_node( struct SyckNode *map, enum map_part p, long idx )
{
    struct SyckMap *m;

    //assert( map != NULL );
    m = map->data.pairs;
    //assert( m != NULL );
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
new_seq_node( struct SyckNode *value )
{
    struct SyckNode *n;

    n = alloc_seq_node();
    add_seq_item( n, value );

    return n;
}

void
add_seq_item( struct SyckNode *arr, struct SyckNode *value )
{
    struct SyckSeq *s;
    long idx;

    //assert( arr != NULL );
    //assert( arr->data.list != NULL );
    
    s = arr->data.list;
    idx = s->capa;
    s->capa += 1;
    REALLOC_N( s->items, struct SyckNode *, s->capa );
    s->items[idx] = value;
}

struct SyckNode *
read_seq_node( struct SyckNode *seq, long idx )
{
    struct SyckSeq *s;

    //assert( seq != NULL );
    s = seq->data.list;
    //assert( s != NULL );
    return s->items[idx];
}

//static char *
//syckp_read( prsr, len )
//    struct SyckParser *prsr;
//    long len;
//{
//    char *str;
//    
//    if ( prsr->fp )
//    {
//        str = 

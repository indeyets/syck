//
// handler.h
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"

SYMID 
syck_hdlr_add_node( SyckParser *p, SyckNode *n )
{
    SYMID id;
    if ( ! n->id ) n->id = (p->handler)( p, n );
    id = n->id;

    if ( n->anchor == NULL ) syck_free_node( n );
    return id;
}

SyckNode *
syck_hdlr_add_anchor( SyckParser *p, char *a, SyckNode *n )
{
    n->anchor = a;
    st_insert( p->anchors, a, n );
    return n;
}

SyckNode *
syck_hdlr_add_alias( SyckParser *p, char *a )
{
    SyckNode *n;

    if ( st_lookup( p->anchors, a, &n ) )
    {
        return n;
    }

    return syck_new_str( "..." );
}

SyckNode *
syck_add_transfer( char *uri, SyckNode *n )
{
    n->type_id = uri;
}

int 
syck_try_implicit( SyckNode *n )
{
    return 1;
}

void 
syck_fold_format( char *fold, SyckNode *n )
{
}


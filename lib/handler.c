//
// handler.h
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "st.h"
#include "syck.h"

SYMID 
syck_hdlr_add_node( SyckParser *p, SyckNode *n )
{
    if ( ! n->id ) n->id = (p->handler)( n );
    return n->id;
}

SyckNode *
syck_hdlr_add_anchor( SyckParser *p, const char *a, SyckNode *n )
{
    st_insert( p->anchors, a, n );
    //st_add_direct( p->anchors, a, n );
    return n;
}

SyckNode *
syck_hdlr_add_alias( SyckParser *p, const char *a )
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


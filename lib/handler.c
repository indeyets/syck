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
    return (p->handler)( n );
}

SyckNode *
syck_hdlr_add_anchor( SyckParser *p, char *a, SyckNode *n )
{
    return n;
}

SyckNode *
syck_hdlr_add_alias( SyckParser *p, char *a )
{
    return syck_new_str( "ALIAS" );
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


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

void
syck_add_transfer( char *uri, SyckNode *n, int taguri )
{
    char *comma = NULL;
    char *slash = uri;
    char *domain = NULL;

    if ( taguri == 0 )
    {
        n->type_id = uri;
        return;
    }

    if ( uri[0] == '!' )
    {
        syck_xprivate( n, uri + 1, strlen( uri ) - 1 );
        S_FREE( uri );
        return;
    }

    while ( *(++slash) != '\0' )
    {
        if ( *slash == '/' )
            break;

        if ( *slash == ',' )
            comma = slash;
    }

    if ( *slash == '\0' )
    {
        syck_taguri( n, "yaml.org,2002", uri, strlen( uri ) );
    }
    else if ( comma == NULL )
    {
        domain = S_ALLOC_N( char, ( slash - uri ) + 15 );
        domain[0] = '\0';
        strncat( domain, uri, slash - uri );
        strcat( domain, ".yaml.org,2002" );
        syck_taguri( n, domain, slash + 1, strlen( uri ) - ( slash - uri + 1 ) );
        S_FREE( domain );
    }
    else
    {
        domain = S_ALLOC_N( char, slash - uri );
        domain[0] = '\0';
        strncat( domain, uri, slash - uri );
        syck_taguri( n, domain, slash + 1, strlen( uri ) - ( slash - uri + 1 ) );
        S_FREE( domain );
    }
    S_FREE( uri );
}

void
syck_xprivate( SyckNode *n, char *type_id, int type_len )
{
    if ( n->type_id != NULL )
        S_FREE( n->type_id );

    n->type_id = S_ALLOC_N( char, type_len + 14 );
    n->type_id[0] = '\0';
    strcat( n->type_id, "x-private:" );
    strncat( n->type_id, type_id, type_len );
}

void
syck_taguri( SyckNode *n, char *domain, char *type_id, int type_len )
{
    if ( n->type_id != NULL )
        S_FREE( n->type_id );

    n->type_id = S_ALLOC_N( char, strlen( domain ) + type_len + 14 );
    n->type_id[0] = '\0';
    strcat( n->type_id, "taguri:" );
    strcat( n->type_id, domain );
    strcat( n->type_id, ":" );
    strncat( n->type_id, type_id, type_len );
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


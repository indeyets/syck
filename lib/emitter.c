/*
 * emitter.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 */
#include <stdio.h>
#include <string.h>

#include "syck.h"

#define DEFAULT_ANCHOR_FORMAT "id%03d"

/*
 * Allocate an emitter
 */
SyckEmitter *
syck_new_emitter()
{
    SyckEmitter *e;
    e = S_ALLOC( SyckEmitter );
    e->headless = 0;
    e->seq_map = 0;
    e->use_header = 0;
    e->use_version = 0;
    e->sort_keys = 0;
    e->anchor_format = NULL;
    e->explicit_typing = 0;
    e->best_width = 80;
    e->block_style = block_arbitrary;
    e->stage = doc_open;
    e->indent = 2;
    e->level = -1;
    e->ignore_id = 0;
    e->anchors = NULL;
    e->markers = NULL;
    e->bufsize = SYCK_BUFFERSIZE;
    e->buffer = NULL;
    e->marker = NULL;
    e->handler = NULL;
    e->bonus = NULL;
    return e;
}

int
syck_st_free_anchors( char *key, char *name, char *arg )
{
    S_FREE( arg );
    return ST_CONTINUE;
}

void
syck_emitter_st_free( SyckEmitter *e )
{
    /*
     * Free the anchor tables
     */
    if ( e->anchors != NULL )
    {
        st_foreach( e->anchors, syck_st_free_anchors, 0 );
        st_free_table( e->anchors );
        e->anchors = NULL;
    }

    /*
     * Free the markers tables
     */
    if ( e->markers != NULL )
    {
        st_free_table( e->markers );
        e->markers = NULL;
    }
}

void
syck_emitter_ignore_id( SyckEmitter *e, SYMID id )
{
    e->ignore_id = id;
}

void
syck_emitter_handler( SyckEmitter *e, SyckOutputHandler hdlr )
{
    e->handler = hdlr;
}

void
syck_free_emitter( SyckEmitter *e )
{
    /*
     * Free tables
     */
    syck_emitter_st_free( e );
    if ( e->buffer != NULL )
    {
        S_FREE( e->buffer );
    }
    S_FREE( e );
}

void
syck_emitter_clear( SyckEmitter *e )
{
    if ( e->buffer == NULL )
    {
        e->buffer = S_ALLOC_N( char, e->bufsize );
        S_MEMZERO( e->buffer, char, e->bufsize );
    }
    e->buffer[0] = '\0';
    e->marker = e->buffer;
}

/*
 * Raw write to the emitter buffer.
 */
void
syck_emitter_write( SyckEmitter *e, char *str, long len )
{
    long at;
    ASSERT( str != NULL )
    if ( e->buffer == NULL )
    {
        syck_emitter_clear( e );
    }
    
    /*
     * Flush if at end of buffer
     */
    at = e->marker - e->buffer;
    if ( len + at > e->bufsize )
    {
        syck_emitter_flush( e );
    }

    /*
     * Write to buffer
     */
    S_MEMCPY( e->marker, str, char, len );
    e->marker += len;
}

/*
 * Write a chunk of data out.
 */
void
syck_emitter_flush( SyckEmitter *e )
{
    if ( ( e->stage == doc_open && ( e->headless == 0 || e->use_header == 1 ) ) || 
         e->stage == doc_need_header )
    {
        if ( e->use_version == 1 )
        {
            char *header = S_ALLOC_N( char, 64 );
            S_MEMZERO( header, char, 64 );
            sprintf( header, "--- %YAML:%d.%d ", SYCK_YAML_MAJOR, SYCK_YAML_MINOR );
            (e->handler)( e, header, strlen( header ) );
            S_FREE( header );
        }
        else
        {
            (e->handler)( e, "--- ", 4 );
        }
        e->stage = doc_processing;
    }
    (e->handler)( e, e->buffer, e->marker - e->buffer );
    e->marker = e->buffer;
}

/*
 * Emit a simple, unquoted string.
 */
void
syck_emitter_simple( SyckEmitter *e, char *str, long len )
{
    e->seq_map = 0;
    syck_emitter_write( e, str, len );
}

/*
 * call on start of an object's marshalling
 * (handles anchors, returns an alias)
 */
char *
syck_emitter_start_obj( SyckEmitter *e, SYMID oid )
{
    char *anchor_name = NULL;

    e->level++;
    if ( oid != e->ignore_id )
    {
        /*
         * Look for anchors
         */
        long ptr;

        if ( e->markers == NULL )
        {
            e->markers = st_init_numtable();
        }

        if ( ! st_lookup( e->markers, (st_data_t)oid, (st_data_t *)&ptr ) )
        {
            if ( e->anchors == NULL )
            {
                e->anchors = st_init_numtable();
            }

            if ( ! st_lookup( e->anchors, (st_data_t)oid, (st_data_t *)&anchor_name ) )
            {
                int idx = 0;
                /*
                 * Second time hitting this object, let's give it an anchor
                 */
                idx = e->anchors->num_entries + 1;

                /*
                 * Create the anchor tag
                 */
                if ( idx > 0 )
                {
                    char *anc = ( e->anchor_format == NULL ? DEFAULT_ANCHOR_FORMAT : e->anchor_format );
                    char *aname = S_ALLOC_N( char, strlen( anc ) + 10 );
                    S_MEMZERO( aname, char, strlen( anc ) + 10 );
                    sprintf( aname, anc, idx );
                    st_insert( e->anchors, (st_data_t)oid, (st_data_t)aname );
                }

            }
        }
        else
        {
            /*
             * Store all markers
             */
            ptr = e->marker - e->buffer;
            st_insert( e->markers, (st_data_t)oid, (st_data_t)ptr );
        }
    }

    return anchor_name;
}

/*
 * call on completion of an object's marshalling
 */
void
syck_emitter_end_obj( SyckEmitter *e )
{
    e->level--;
}


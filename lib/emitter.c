/*
 * emitter.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 * 
 * All Base64 code from Ruby's pack.c.
 * Ruby is Copyright (C) 1993-2003 Yukihiro Matsumoto 
 */
#include <stdio.h>
#include <string.h>

#include "syck.h"

#define DEFAULT_ANCHOR_FORMAT "id%03d"

static char b64_table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Built-in base64 (from Ruby's pack.c)
 */
char *
syck_base64enc( char *s, long len )
{
    long i = 0;
    int padding = '=';
    char *buff = S_ALLOCA_N(char, len * 4 / 3 + 6);

    while (len >= 3) {
        buff[i++] = b64_table[077 & (*s >> 2)];
        buff[i++] = b64_table[077 & (((*s << 4) & 060) | ((s[1] >> 4) & 017))];
        buff[i++] = b64_table[077 & (((s[1] << 2) & 074) | ((s[2] >> 6) & 03))];
        buff[i++] = b64_table[077 & s[2]];
        s += 3;
        len -= 3;
    }
    if (len == 2) {
        buff[i++] = b64_table[077 & (*s >> 2)];
        buff[i++] = b64_table[077 & (((*s << 4) & 060) | ((s[1] >> 4) & 017))];
        buff[i++] = b64_table[077 & (((s[1] << 2) & 074) | (('\0' >> 6) & 03))];
        buff[i++] = padding;
    }
    else if (len == 1) {
        buff[i++] = b64_table[077 & (*s >> 2)];
        buff[i++] = b64_table[077 & (((*s << 4) & 060) | (('\0' >> 4) & 017))];
        buff[i++] = padding;
        buff[i++] = padding;
    }
    buff[i++] = '\n';
    return buff;
}

char *
syck_base64dec( char *s, long len )
{
    int a = -1,b = -1,c = 0,d;
    static int first = 1;
    static int b64_xtable[256];
    char *ptr = syck_strndup( s, len );
    char *end = ptr;
    char *send = s + len;

    if (first) {
        int i;
        first = 0;

        for (i = 0; i < 256; i++) {
        b64_xtable[i] = -1;
        }
        for (i = 0; i < 64; i++) {
        b64_xtable[(int)b64_table[i]] = i;
        }
    }
    while (s < send) {
        while (s[0] == '\r' || s[0] == '\n') { s++; }
        if ((a = b64_xtable[(int)s[0]]) == -1) break;
        if ((b = b64_xtable[(int)s[1]]) == -1) break;
        if ((c = b64_xtable[(int)s[2]]) == -1) break;
        if ((d = b64_xtable[(int)s[3]]) == -1) break;
        *end++ = a << 2 | b >> 4;
        *end++ = b << 4 | c >> 2;
        *end++ = c << 6 | d;
        s += 4;
    }
    if (a != -1 && b != -1) {
        if (s + 2 < send && s[2] == '=')
        *end++ = a << 2 | b >> 4;
        if (c != -1 && s + 3 < send && s[3] == '=') {
        *end++ = a << 2 | b >> 4;
        *end++ = b << 4 | c >> 2;
        }
    }
    *end = '\0';
    /*RSTRING(buf)->len = ptr - RSTRING(buf)->ptr;*/
    return ptr;
}

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
    e->anchors = NULL;
    e->markers = NULL;
    e->bufsize = SYCK_BUFFERSIZE;
    e->buffer = NULL;
    e->marker = NULL;
    e->bufpos = 0;
    e->emitter_handler = NULL;
    e->output_handler = NULL;
    e->bonus = NULL;
    return e;
}

int
syck_st_free_anchors( char *key, char *name, char *arg )
{
    S_FREE( name );
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
syck_emitter_handler( SyckEmitter *e, SyckEmitterHandler hdlr )
{
    e->emitter_handler = hdlr;
}

void
syck_output_handler( SyckEmitter *e, SyckOutputHandler hdlr )
{
    e->output_handler = hdlr;
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
    e->bufpos = 0;
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
    if ( len + at >= e->bufsize )
    {
        syck_emitter_flush( e, 0 );
	for (;;) {
	    long rest = e->bufsize - (e->marker - e->buffer);
	    if (len <= rest) break;
	    S_MEMCPY( e->marker, str, char, rest );
	    e->marker += rest;
	    str += rest;
	    len -= rest;
	    syck_emitter_flush( e, 0 );
	}
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
syck_emitter_flush( SyckEmitter *e, long check_room )
{
    /*
     * Check for enough space in the buffer for check_room length.
     */
    if ( check_room > 0 )
    {
        if ( e->bufsize > ( e->marker - e->buffer ) + check_room )
        {
            return;
        }
    }
    else
    {
        check_room = e->bufsize;
    }

    /*
     * Determine headers.
     */
    if ( ( e->stage == doc_open && ( e->headless == 0 || e->use_header == 1 ) ) || 
         e->stage == doc_need_header )
    {
        if ( e->use_version == 1 )
        {
            char *header = S_ALLOC_N( char, 64 );
            S_MEMZERO( header, char, 64 );
            sprintf( header, "--- %%YAML:%d.%d ", SYCK_YAML_MAJOR, SYCK_YAML_MINOR );
            (e->output_handler)( e, header, strlen( header ) );
            S_FREE( header );
        }
        else
        {
            (e->output_handler)( e, "--- ", 4 );
        }
        e->stage = doc_processing;
    }

    /*
     * Commit buffer.
     */
    if ( check_room > e->marker - e->buffer )
    {
        check_room = e->marker - e->buffer;
    }
    (e->output_handler)( e, e->buffer, check_room );
    e->bufpos += check_room;
    e->marker -= check_room;
}

/*
 * Start emitting from the given node, 
 */
void
syck_emit( SyckEmitter *e, char *n )
{
    SYMID oid;
    char *anchor_name = NULL;
    
    /* Look for anchor */
    if ( e->anchors != NULL &&
        st_lookup( e->markers, (st_data_t)n, (st_data_t *)&oid ) &&
        st_lookup( e->anchors, (st_data_t)oid, (st_data_t *)&anchor_name ) )
    {
        char *an = S_ALLOC_N( char, strlen( anchor_name ) + 2 );
        sprintf( an, "&%s ", anchor_name );
        syck_emitter_write( e, an, strlen( an ) );
    }
    (e->emitter_handler)( e, n );
}

void syck_emit_scalar( SyckEmitter *e, char *tag, char *str, long len )
{
    e->seq_map = 0;
    syck_emitter_write( e, str, len );
}

void syck_emit_seq( SyckEmitter *e, char *tag )
{
    syck_emitter_write( e, "SEQ", 3 );
}

void syck_emit_map( SyckEmitter *e, char *tag )
{
    syck_emitter_write( e, "MAP!!!", 6 );
}

void syck_emit_end( SyckEmitter *e )
{
}

/*
 * Fill markers table with emitter nodes in the
 * soon-to-be-emitted tree.
 */
SYMID
syck_emitter_mark_node( SyckEmitter *e, char *n )
{
    SYMID oid = 0;
    char *anchor_name = NULL;

    /*
     * Ensure markers table is initialized.
     */
    if ( e->markers == NULL )
    {
        e->markers = st_init_numtable();
    }

    /*
     * Markers table initially marks the string position of the
     * object.  Doesn't yet create an anchor, simply notes the
     * position.
     */
    if ( ! st_lookup( e->markers, (st_data_t)n, (st_data_t *)&oid ) )
    {
        /*
         * Store all markers
         */
        oid = e->markers->num_entries + 1;
        st_insert( e->markers, (st_data_t)n, (st_data_t)oid );
    }
    else
    {
        if ( e->anchors == NULL )
        {
            e->anchors = st_init_numtable();
        }

        if ( ! st_lookup( e->anchors, (st_data_t)oid, (st_data_t *)&anchor_name ) )
        {
            int idx = 0;
            char *anc = ( e->anchor_format == NULL ? DEFAULT_ANCHOR_FORMAT : e->anchor_format );

            /*
             * Second time hitting this object, let's give it an anchor
             */
            idx = e->anchors->num_entries + 1;
            anchor_name = S_ALLOC_N( char, strlen( anc ) + 10 );
            S_MEMZERO( anchor_name, char, strlen( anc ) + 10 );
            sprintf( anchor_name, anc, idx );

            /*
             * Insert into anchors table
             */
            st_insert( e->anchors, (st_data_t)oid, (st_data_t)anchor_name );
        }
    }
    return oid;
}


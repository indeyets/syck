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
    char *buff = S_ALLOC_N(char, len * 4 / 3 + 6);

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
    e->anchored = NULL;
    e->bufsize = SYCK_BUFFERSIZE;
    e->buffer = NULL;
    e->marker = NULL;
    e->bufpos = 0;
    e->emitter_handler = NULL;
    e->output_handler = NULL;
    e->lvl_idx = 0;
    e->lvl_capa = ALLOC_CT;
    e->levels = S_ALLOC_N( SyckLevel, e->lvl_capa ); 
    syck_emitter_reset_levels( e );
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

    if ( e->anchored != NULL )
    {
        st_free_table( e->anchored );
        e->anchored = NULL;
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

SyckLevel *
syck_emitter_current_level( SyckEmitter *e )
{
    return &e->levels[e->lvl_idx-1];
}

SyckLevel *
syck_emitter_parent_level( SyckEmitter *e )
{
    return &e->levels[e->lvl_idx-2];
}

void
syck_emitter_pop_level( SyckEmitter *e )
{
    ASSERT( e != NULL );

    /* The root level should never be popped */
    if ( e->lvl_idx <= 1 ) return;

    e->lvl_idx -= 1;
    free( e->levels[e->lvl_idx].domain );
}

void 
syck_emitter_add_level( SyckEmitter *e, int len, enum syck_level_status status )
{
    ASSERT( e != NULL );
    if ( e->lvl_idx + 1 > e->lvl_capa )
    {
        e->lvl_capa += ALLOC_CT;
        S_REALLOC_N( e->levels, SyckLevel, e->lvl_capa );
    }

    ASSERT( len > e->levels[e->lvl_idx-1].spaces );
    e->levels[e->lvl_idx].spaces = len;
    e->levels[e->lvl_idx].ncount = 0;
    e->levels[e->lvl_idx].domain = syck_strndup( e->levels[e->lvl_idx-1].domain, strlen( e->levels[e->lvl_idx-1].domain ) );
    e->levels[e->lvl_idx].status = status;
    e->lvl_idx += 1;
}

void
syck_emitter_reset_levels( SyckEmitter *e )
{
    while ( e->lvl_idx > 1 )
    {
        syck_emitter_pop_level( e );
    }

    if ( e->lvl_idx < 1 )
    {
        e->lvl_idx = 1;
        e->levels[0].spaces = -1;
        e->levels[0].ncount = 0;
        e->levels[0].domain = syck_strndup( "", 0 );
    }
    e->levels[0].status = syck_lvl_header;
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
    syck_emitter_reset_levels( e );
    S_FREE( e->levels[0].domain );
    S_FREE( e->levels );
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
    e->marker[0] = '\0';
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
 * Start emitting from the given node, check for anchoring and then
 * issue the callback to the emitter handler.
 */
void
syck_emit( SyckEmitter *e, char *n )
{
    SYMID oid;
    char *anchor_name = NULL;
    int indent = 0, x = 0;
    SyckLevel *lvl = syck_emitter_current_level( e );
    
    /* Add new level */
    if ( lvl->spaces >= 0 ) {
        indent = lvl->spaces + e->indent;
    }
    syck_emitter_add_level( e, indent, syck_lvl_open );

    /* Look for anchor */
    if ( e->anchors != NULL &&
        st_lookup( e->markers, (st_data_t)n, (st_data_t *)&oid ) &&
        st_lookup( e->anchors, (st_data_t)oid, (st_data_t *)&anchor_name ) )
    {
        if ( e->anchored == NULL )
        {
            e->anchored = st_init_numtable();
        }

        if ( ! st_lookup( e->anchored, (st_data_t)anchor_name, (st_data_t *)&x ) )
        {
            char *an = S_ALLOC_N( char, strlen( anchor_name ) + 3 );
            sprintf( an, "&%s ", anchor_name );
            syck_emitter_write( e, an, strlen( anchor_name ) + 2 );
            free( an );

            x = 1;
            st_insert( e->anchored, (st_data_t)anchor_name, (st_data_t)x );
        }
        else
        {
            char *an = S_ALLOC_N( char, strlen( anchor_name ) + 2 );
            sprintf( an, "*%s", anchor_name );
            syck_emitter_write( e, an, strlen( anchor_name ) + 1 );
            free( an );

            goto end_emit;
        }
    }

    (e->emitter_handler)( e, n );

    /* Pop the level */
end_emit:
    syck_emitter_pop_level( e );
    if ( e->lvl_idx == 1 ) {
        syck_emitter_write( e, "\n", 1 );
        e->stage = doc_open;
    }
}

void syck_emit_tag( SyckEmitter *e, char *tag, char *ignore )
{
    if ( tag == NULL ) return;
    if ( ignore != NULL && strcmp( tag, ignore ) == 0 && e->explicit_typing == 0 ) return;

    /* global types */
    if ( strncmp( tag, "tag:", 4 ) == 0 ) {
        int taglen = strlen( tag );
        syck_emitter_write( e, "!", 1 );
        if ( strncmp( tag + 4, YAML_DOMAIN, strlen( YAML_DOMAIN ) ) == 0 ) {
            int skip = 4 + strlen( YAML_DOMAIN ) + 1;
            syck_emitter_write( e, tag + skip, taglen - skip );
        } else {
            char *subd = tag + 4;
            while ( *subd != ':' && *subd != '\0' ) subd++;
            if ( *subd == ':' ) {
                if ( subd - tag > ( strlen( YAML_DOMAIN ) + 5 ) &&
                     strncmp( subd - strlen( YAML_DOMAIN ), YAML_DOMAIN, strlen( YAML_DOMAIN ) ) == 0 ) {
                    syck_emitter_write( e, tag + 4, ( subd - strlen( YAML_DOMAIN ) - ( tag + 4 ) ) );
                    syck_emitter_write( e, "/", 1 );
                    syck_emitter_write( e, subd + 1, ( tag + taglen ) - ( subd + 1 ) );
                } else {
                    syck_emitter_write( e, tag + 4, subd - ( tag + 4 ) );
                    syck_emitter_write( e, "/", 1 );
                    syck_emitter_write( e, subd + 1, ( tag + taglen ) - ( subd + 1 ) );
                }
            } else {
                /* TODO: Invalid tag (no colon after domain) */
            }
        }
        syck_emitter_write( e, " ", 1 );

    /* private types */
    } else if ( strncmp( tag, "x-private:", 10 ) == 0 ) {
        syck_emitter_write( e, "!!", 2 );
        syck_emitter_write( e, tag + 10, strlen( tag ) - 10 );
        syck_emitter_write( e, " ", 1 );
    }
}

void syck_emit_indent( SyckEmitter *e )
{
    int i;
    SyckLevel *lvl = syck_emitter_current_level( e );
    char *spcs = S_ALLOC_N( char, lvl->spaces + 2 );

    spcs[0] = '\n'; spcs[lvl->spaces + 1] = '\0';
    for ( i = 0; i < lvl->spaces; i++ ) spcs[i+1] = ' ';
    syck_emitter_write( e, spcs, lvl->spaces + 1 );
    free( spcs );
}

void syck_emit_scalar( SyckEmitter *e, char *tag, enum block_styles force_style, int force_indent,
                       char keep_nl, char *str, long len )
{
    int mark = 0, end = 0;
    SyckLevel *lvl = syck_emitter_current_level( e );
    char *implicit = syck_match_implicit( str, len );
    implicit = syck_taguri( YAML_DOMAIN, implicit, strlen( implicit ) );
    syck_emit_tag( e, tag, implicit );
    S_FREE( implicit );

    /* Determine block style */
    if ( force_style == block_arbitrary ) {
        force_style = e->block_style;
    }
    /* TODO: if arbitrary, sniff a good block style. */
    if ( force_style == block_arbitrary ) {
        force_style = block_literal;
    }

    if ( force_indent == 0 ) {
        force_indent = e->indent;
    }

    /* Write the text node */
    if ( strchr( str, '\n' ) ) {
        syck_emit_literal( e, str, len );
    } else {
        syck_emitter_write( e, str, len );
    }
}

void syck_emit_literal( SyckEmitter *e, char *str, long len )
{
    char *mark = str;
    char *start = str;
    char *end = str;
    syck_emitter_write( e, "|", 1 );
    syck_emit_indent( e );
    while ( mark < str + len ) {
        if ( *mark == '\n' ) {
            end = mark;
            if ( *start != ' ' && *start != '\n' && *end != '\n' && *end != ' ' ) end += 1;
            syck_emitter_write( e, start, end - start );
            syck_emit_indent( e );
            start = mark + 1;
        }
        mark++;
    }
    end = str + len;
    if ( start < end ) {
        syck_emitter_write( e, start, end - start );
    }
}

void syck_emit_folded( SyckEmitter *e, int width, char *str, long len )
{
    char *mark = str;
    char *start = str;
    char *end = str;
    syck_emitter_write( e, ">", 1 );
    syck_emit_indent( e );
    while ( mark < str + len ) {
        switch ( *mark ) {
            case '\n':
                end = mark;
                if ( *start != ' ' && *start != '\n' && *end != '\n' && *end != ' ' ) end += 1;
                syck_emitter_write( e, start, end - start );
                syck_emit_indent( e );
                start = mark + 1;
            break;

            case ' ':
                if ( *start != ' ' ) {
                    if ( mark - start > width ) {
                        if ( start >= end ) {
                            end = mark;
                        }
                        syck_emitter_write( e, start, end - start );
                        syck_emit_indent( e );
                        start = end + 1;
                        end = end + 1;
                        mark = end;
                    } else {
                        end = mark;
                    }
                }
            break;
        }
        mark++;
    }
    end = str + len;
    if ( start < end ) {
        syck_emitter_write( e, start, end - start );
    }
}

void syck_emit_seq( SyckEmitter *e, char *tag )
{
    SyckLevel *lvl = syck_emitter_current_level( e );
    syck_emit_tag( e, tag, "tag:yaml.org,2002:seq" );
    lvl->status = syck_lvl_seq;
}

void syck_emit_map( SyckEmitter *e, char *tag )
{
    SyckLevel *lvl = syck_emitter_current_level( e );
    syck_emit_tag( e, tag, "tag:yaml.org,2002:map" );
    lvl->status = syck_lvl_map;
}

void syck_emit_item( SyckEmitter *e, char *n )
{
    SyckLevel *lvl = syck_emitter_current_level( e );
    switch ( lvl->status )
    {
        case syck_lvl_seq:
        {
            SyckLevel *parent = syck_emitter_parent_level( e );

            /* seq-in-map shortcut */
            if ( parent->status == syck_lvl_map && lvl->ncount == 0 ) {
                /* complex key */
                if ( parent->ncount % 2 == 1 ) {
                    syck_emitter_write( e, "?", 1 );
                    parent->status = syck_lvl_mapx;
                } else {
                    lvl->spaces = parent->spaces;
                }
            }

            /* seq-in-seq shortcut */
            else if ( parent->status == syck_lvl_seq && lvl->ncount == 0 ) {
                int spcs = ( lvl->spaces - parent->spaces ) - 2;
                if ( spcs >= 0 ) {
                    int i = 0;
                    for ( i = 0; i < spcs; i++ ) {
                        syck_emitter_write( e, " ", 1 );
                    }
                    syck_emitter_write( e, "- ", 2 );
                    break;
                }
            }

            syck_emit_indent( e );
            syck_emitter_write( e, "- ", 2 );
        }
        break;

        case syck_lvl_map:
        {
            SyckLevel *parent = syck_emitter_parent_level( e );

            /* map-in-seq shortcut */
            if ( parent->status == syck_lvl_seq && lvl->ncount == 0 ) {
                int spcs = ( lvl->spaces - parent->spaces ) - 2;
                if ( spcs >= 0 ) {
                    int i = 0;
                    for ( i = 0; i < spcs; i++ ) {
                        syck_emitter_write( e, " ", 1 );
                    }
                    break;
                }
            }

            if ( lvl->ncount % 2 == 0 ) {
                syck_emit_indent( e );
            } else {
                syck_emitter_write( e, ": ", 2 );
            }
        }
        break;

        case syck_lvl_mapx:
        {
            if ( lvl->ncount % 2 == 0 ) {
                syck_emit_indent( e );
                lvl->status = syck_lvl_map;
            } else {
                syck_emitter_write( e, ": ", 2 );
            }
        }
        break;
    }
    lvl->ncount++;

    syck_emit( e, n );
}

void syck_emit_end( SyckEmitter *e )
{
    SyckLevel *lvl = syck_emitter_current_level( e );
    SyckLevel *parent = syck_emitter_parent_level( e );
    switch ( lvl->status )
    {
        case syck_lvl_seq:
            if ( lvl->ncount == 0 ) {
                syck_emitter_write( e, "[]\n", 3 );
            } else if ( parent->status == syck_lvl_mapx ) {
                syck_emitter_write( e, "\n", 1 );
            }
        break;

        case syck_lvl_map:
            if ( lvl->ncount == 0 ) {
                syck_emitter_write( e, "{}\n", 3 );
            } else if ( lvl->ncount % 2 == 1 ) {
                syck_emitter_write( e, ":\n", 1 );
            } else if ( parent->status == syck_lvl_mapx ) {
                syck_emitter_write( e, "\n", 1 );
            }
        break;
    }
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


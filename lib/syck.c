//
// syck.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//
#include <stdio.h>
#include <string.h>

#include "syck.h"

#define SYCK_YAML_MAJOR 1
#define SYCK_YAML_MINOR 0

//
// Custom assert
//
void 
syck_assert( char *file_name, unsigned line_num )
{
    fflush( NULL );
    fprintf( stderr, "\nAssertion failed: %s, line %u\n",
             file_name, line_num );
    fflush( stderr );
    abort();
}

char *
syck_strndup( char *buf, long len )
{
    char *new = S_ALLOC_N( char, len + 1 );
    memset( new, 0, len + 1 );
    memcpy( new, buf, len );
}

//
// Default IO functions
//
int
syck_io_file_read( char *buf, SyckIoFile *file, int max_size )
{
    int len = 0;
    return len;
}

int
syck_io_str_read( char *buf, SyckIoStr *str, int max_size )
{
    char *beg;
    int len = 0;

    ASSERT( str != NULL );
    beg = str->ptr;
    if ( max_size >= 0 )
    {
        str->ptr += max_size;
        if ( str->ptr > str->end )
        {
            str->ptr = str->end;
        }
    }
    else
    {
        // Use exact string length
        while ( str->ptr < str->end ) {
            if (*(str->ptr++) == '\n') break;
        }
    }
    if ( beg < str->ptr )
    {
        len = str->ptr - beg;
        memcpy( buf, beg, len );
    }
    return len;
}

//
// Allocate the parser
//
SyckParser *
syck_new_parser()
{
    SyckParser *p;
    p = S_ALLOC( SyckParser );
    p->anchors = st_init_strtable();
    return p;
}

void
syck_free_parser( SyckParser *p )
{
    st_free_table( p->anchors );
    free_any_io( p );
    free( p );
}

void
syck_parser_handler( SyckParser *p, SyckNodeHandler hdlr )
{
    ASSERT( p != NULL );
    p->handler = hdlr;
}

void
syck_parser_file( SyckParser *p, FILE *fp, SyckIoFileRead read )
{
    ASSERT( p != NULL );
    free_any_io( p );
    p->io_type = syck_io_file;
    p->io.file = S_ALLOC( SyckIoFile );
    p->io.file->ptr = fp;
    if ( read != NULL )
    {
        p->io.file->read = read;
    }
    else
    {
        p->io.file->read = syck_io_file_read;
    }
}

void
syck_parser_str( SyckParser *p, char *ptr, long len, SyckIoStrRead read )
{
    ASSERT( p != NULL );
    free_any_io( p );
    p->io_type = syck_io_str;
    p->io.str = S_ALLOC( SyckIoStr );
    p->io.str->ptr = ptr;
    p->io.str->end = ptr + len;
    if ( read != NULL )
    {
        p->io.str->read = read;
    }
    else
    {
        p->io.str->read = syck_io_str_read;
    }
}

void
syck_parser_str_auto( SyckParser *p, char *ptr, SyckIoStrRead read )
{
    syck_parser_str( p, ptr, strlen( ptr ), read );
}

void
free_any_io( SyckParser *p )
{
    ASSERT( p != NULL );
    switch ( p->io_type )
    {
        case syck_io_str:
            if ( p->io.str != NULL ) 
            {
                free( p->io.str );
                p->io.str = NULL;
            }
        break;

        case syck_io_file:
            if ( p->io.file != NULL ) 
            {
                free( p->io.file );
                p->io.file = NULL;
            }
        break;
    }
}

int
syck_parser_readline( char *buf, SyckParser *p )
{
    ASSERT( p != NULL );
    switch ( p->io_type )
    {
        case syck_io_str:
        return (p->io.str->read)( buf, p->io.str, -1 );

        case syck_io_file:
        return (p->io.file->read)( buf, p->io.file, -1 );
    }
}

int
syck_parser_read( char *buf, SyckParser *p, int len )
{
    ASSERT( p != NULL );
    switch ( p->io_type )
    {
        case syck_io_str:
        return (p->io.str->read)( buf, p->io.str, len );

        case syck_io_file:
        return (p->io.file->read)( buf, p->io.file, len );
    }
}

SYMID
syck_parse( SyckParser *p )
{
    char *line;

    ASSERT( p != NULL );
    syck_parser_init( p, 0 );
    yyrestart();
    yyparse( p );
    return p->root;
}


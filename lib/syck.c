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

//
// Default IO functions
//
char *
syck_io_file_read( SyckFile *file )
{
}

char *
syck_io_str_read( SyckStr *str )
{
    char *beg;

    ASSERT( str != NULL );
    beg = str->ptr;
    while ( str->ptr < str->end ) {
        if (*(str->ptr++) == '\n') break;
    }
    if ( beg < str->ptr )
    {
        return strndup( beg, str->ptr - beg );
    }
    return NULL;
}

//
// Allocate the parser
//
SyckParser *
syck_new_parser()
{
    SyckParser *p;
    p = ALLOC( SyckParser );
    return p;
}

void
syck_parser_handler( SyckParser *p, SyckNodeHandler hdlr )
{
    ASSERT( p != NULL );
    p->handler = hdlr;
}

void
syck_parser_file( SyckParser *p, FILE *fp, SyckFileRead read )
{
    ASSERT( p != NULL );
    free_any_io( p );
    p->io_type = syck_io_file;
    p->io.file = ALLOC( SyckFile );
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
syck_parser_str( SyckParser *p, char *ptr, long len, SyckStrRead read )
{
    ASSERT( p != NULL );
    free_any_io( p );
    p->io_type = syck_io_str;
    p->io.str = ALLOC( SyckStr );
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
syck_parser_str_auto( SyckParser *p, char *ptr, SyckStrRead read )
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
            free( p->io.str );
        break;

        case syck_io_file:
            free( p->io.file );
        break;
    }
}

char *
syck_parser_readline( SyckParser *p )
{
    ASSERT( p != NULL );
    switch ( p->io_type )
    {
        case syck_io_str:
        return (p->io.str->read)( p->io.str );

        case syck_io_file:
        return (p->io.file->read)( p->io.file );
    }
}

SYMID
syck_parse( SyckParser *p )
{
    char *line;

    ASSERT( p != NULL );
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

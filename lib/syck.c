//
// syck.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//
#include <stdio.h>

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
    p->handler = hdlr;
}

SYMID
syck_parse( SyckParser *p, char *doc )
{
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

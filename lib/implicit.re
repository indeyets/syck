//
// implicit.re
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"

#define YYCTYPE     char
#define YYCURSOR    cursor
#define YYMARKER    marker
#define YYLIMIT     limit
#define YYFILL(n)

#define TAG_IMPLICIT( tid ) \
    if ( taguri == 1 ) \
    { \
        syck_taguri( n, "yaml.org,2002", tid, strlen( tid ) ); \
    } else { \
        n->type_id = syck_strndup( tid, strlen( tid ) ); \
    } \
    return;

void
try_tag_implicit( SyckNode *n, int taguri )
{
    char *cursor, *limit, *marker;
    if ( n->kind != syck_str_kind )
        return;

    cursor = n->data.str->ptr;
    limit = cursor + n->data.str->len;

/*!re2c

WORDC = [A-Za-z0-9_-\.]+ ;
ENDSPC = [ \n]+ ;
NULL = [\000] ;
ANY = [\001-\377] ;
DIGIT = [0-9] ;
DIGITSC = [0-9,] ;
YEAR = DIGIT DIGIT DIGIT DIGIT ;
MON = DIGIT DIGIT ;
SIGN = [-+] ;
HEX = [0-9a-fA-F,] ;
OCT = [0-7,] ;
INTHEX = SIGN? "0x" HEX+ ; 
INTOCT = SIGN? "0" OCT+ ;
INTCANON = SIGN? ( "0" | [1-9] DIGITSC* ) ;
NULLTYPE = ( "~" | "null" | "Null" | "NULL" ) ;
BOOLYES = ( "true" | "True" | "TRUE" | "yes" | "Yes" | "YES" | "on" | "On" | "ON" ) ;
BOOLNO = ( "false" | "False" | "FALSE" | "no" | "No" | "NO" | "off" | "Off" | "OFF" ) ;
TIMEZ = ( "Z" | [-+] DIGIT DIGIT ( ":" DIGIT DIGIT )? ) ;
TIMEYMD = YEAR "-" MON "-" MON ;
TIMEISO = YEAR "-" MON "-" MON [Tt] MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? TIMEZ ;
TIMESPACED = YEAR "-" MON "-" MON [ \t]+ MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? [ \t]+ TIMEZ ;
TIMECANON = YEAR "-" MON "-" MON "T" MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? "Z" ;

NULLTYPE NULL       {   TAG_IMPLICIT( "null" ); }

BOOLYES NULL        {   TAG_IMPLICIT( "bool#yes" ); }

BOOLNO NULL         {   TAG_IMPLICIT( "bool#no" ); }

INTHEX NULL         {   TAG_IMPLICIT( "int#hex" ); }

INTOCT NULL         {   TAG_IMPLICIT( "int#oct" ); }

INTCANON NULL       {   TAG_IMPLICIT( "int" ); }

TIMEYMD NULL        {   TAG_IMPLICIT( "timestamp#ymd" ); }

TIMEISO NULL        {   TAG_IMPLICIT( "timestamp#iso8601" ); }

TIMESPACED NULL     {   TAG_IMPLICIT( "timestamp#spaced" ); }

TIMECANON NULL      {   TAG_IMPLICIT( "timestamp" ); }

ANY                 {   TAG_IMPLICIT( "str" ); }

*/

}


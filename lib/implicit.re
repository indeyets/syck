/*
 * implicit.re
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 */

#include "syck.h"

#define YYCTYPE     char
#define YYCURSOR    cursor
#define YYMARKER    marker
#define YYLIMIT     limit
#define YYFILL(n)

void
try_tag_implicit( SyckNode *n, int taguri )
{
    char *tid;
    switch ( n->kind )
    {
        case syck_str_kind:
            tid = syck_match_implicit( n->data.str->ptr, n->data.str->len );
        break;

        case syck_seq_kind:
            tid = "seq";
        break;

        case syck_map_kind:
            tid = "map";
        break;
    }
    if ( taguri == 1 )
    {
        n->type_id = syck_taguri( YAML_DOMAIN, tid, strlen( tid ) );
    } else {
        n->type_id = syck_strndup( tid, strlen( tid ) );
    }
}

char *syck_match_implicit( char *str, size_t len )
{
    char *cursor, *limit, *marker;
    cursor = str;
    limit = str + len;

/*!re2c

WORDC = [A-Za-z0-9_-\.]+ ;
ENDSPC = [ \n]+ ;
NULL = [\000] ;
ANY = [\001-\377] ;
DIGIT = [0-9] ;
DIGITSC = [0-9,] ;
DIGITSP = [0-9.] ;
YEAR = DIGIT DIGIT DIGIT DIGIT ;
MON = DIGIT DIGIT ;
SIGN = [-+] ;
HEX = [0-9a-fA-F,] ;
OCT = [0-7,] ;
INTHEX = SIGN? "0x" HEX+ ; 
INTOCT = SIGN? "0" OCT+ ;
INTCANON = SIGN? ( "0" | [1-9] DIGITSC* ) ;
FLOATFIX = SIGN? DIGIT DIGITSC* "." DIGITSC* ;
FLOATEXP = SIGN? DIGIT DIGITSC* "." DIGITSP* [eE] SIGN DIGIT+ ;
INF = ( "inf" | "Inf" | "INF" ) ;
FLOATINF = [+]? "." INF ;
FLOATNEGINF = [-] "." INF ;
FLOATNAN = "." ( "nan" | "NaN" | "NAN" ) ;
NULLTYPE = ( "~" | "null" | "Null" | "NULL" ) ;
BOOLYES = ( "true" | "True" | "TRUE" | "yes" | "Yes" | "YES" | "on" | "On" | "ON" ) ;
BOOLNO = ( "false" | "False" | "FALSE" | "no" | "No" | "NO" | "off" | "Off" | "OFF" ) ;
TIMEZ = ( "Z" | [-+] DIGIT DIGIT ( ":" DIGIT DIGIT )? ) ;
TIMEYMD = YEAR "-" MON "-" MON ;
TIMEISO = YEAR "-" MON "-" MON [Tt] MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? TIMEZ ;
TIMESPACED = YEAR "-" MON "-" MON [ \t]+ MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? [ \t]+ TIMEZ ;
TIMECANON = YEAR "-" MON "-" MON "T" MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? "Z" ;

NULLTYPE NULL       {   return "null"; }

BOOLYES NULL        {   return "bool#yes"; }

BOOLNO NULL         {   return "bool#no"; }

INTHEX NULL         {   return "int#hex"; }

INTOCT NULL         {   return "int#oct"; }

INTCANON NULL       {   return "int"; }

FLOATFIX NULL       {   return "float#fix"; }

FLOATEXP NULL       {   return "float#exp"; }

FLOATINF NULL       {   return "float#inf"; }

FLOATNEGINF NULL    {   return "float#neginf"; }

FLOATNAN NULL       {   return "float#nan"; }

TIMEYMD NULL        {   return "timestamp#ymd"; }

TIMEISO NULL        {   return "timestamp#iso8601"; }

TIMESPACED NULL     {   return "timestamp#spaced"; }

TIMECANON NULL      {   return "timestamp"; }

ANY                 {   return "str"; }

*/

}

char *
syck_type_id_to_uri( char *type_id )
{
    char *cursor, *limit, *marker;

    cursor = type_id;
    limit = type_id + strlen( type_id );

/*!re2c

TAG = "taguri" ;
XPRIVATE = "xprivate" ;
WD = [A-Za-z0-9_] ;
WDD = [A-Za-z0-9_-] ;
DNSCOMPRE = WD ( WDD* WD )? ;
DNSNAMERE = ( ( DNSCOMPRE "." )+ DNSCOMPRE | DNSCOMPRE ) ;
TAGDATE = YEAR ( "-" MON )? ( "-" MON )? ;

TAG ":" DNSNAMERE "," TAGDATE ":"    {   return type_id; }

XPRIVATE ":"    {   return type_id; }

"!"             {   return syck_xprivate( type_id + 1, strlen( type_id ) - 1 ); }

DNSNAMERE "/"   {   char *domain = S_ALLOC_N( char, ( YYCURSOR - type_id ) + 15 );
                    char *uri;

                    domain[0] = '\0';
                    strncat( domain, type_id, ( YYCURSOR - type_id ) - 1 );
                    strcat( domain, "." );
                    strcat( domain, YAML_DOMAIN );
                    uri = syck_taguri( domain, YYCURSOR, YYLIMIT - YYCURSOR );

                    S_FREE( domain );
                    return uri;
                }

DNSNAMERE "," TAGDATE "/"  {   char *domain = S_ALLOC_N( char, YYCURSOR - type_id );
                               char *uri;

                               domain[0] = '\0';
                               strncat( domain, type_id, ( YYCURSOR - type_id ) - 1 );
                               uri = syck_taguri( domain, YYCURSOR, YYLIMIT - YYCURSOR );

                               S_FREE( domain );
                               return uri;
                            }

ANY             {   return syck_taguri( YAML_DOMAIN, type_id, strlen( type_id ) ); }

*/

}


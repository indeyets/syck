/*
 * implicit.re
 *
 * $Author: indeyets $
 * $Date: 2007-10-24 23:56:48 +0400 (ср, 24 окт 2007) $
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
    char *tid = "";
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
    if ( n->type_id != NULL ) S_FREE( n->type_id );
    if ( taguri == 1 )
    {
        n->type_id = syck_taguri( YAML_DOMAIN, tid, strlen( tid ) );
    } else {
        n->type_id = syck_strndup( tid, strlen( tid ) );
    }
}

char *syck_match_implicit( const char *str, size_t len )
{
    const char *cursor, *limit, *marker;
    cursor = str;
    limit = str + len;

/*!re2c

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
INTSIXTY = SIGN? DIGIT DIGITSC* ( ":" [0-5]? DIGIT )+ ;
INTCANON = SIGN? ( "0" | [1-9] DIGITSC* ) ;
FLOATFIX = SIGN? DIGIT DIGITSC* "." DIGITSC* ;
FLOATEXP = SIGN? DIGIT DIGITSC* "." DIGITSP* [eE] SIGN DIGIT+ ;
FLOATSIXTY = SIGN? DIGIT DIGITSC* ( ":" [0-5]? DIGIT )+ "." DIGITSC* ;
INF = ( "inf" | "Inf" | "INF" ) ;
FLOATINF = [+]? "." INF ;
FLOATNEGINF = [-] "." INF ;
FLOATNAN = "." ( "nan" | "NaN" | "NAN" ) ;
NULLTYPE = ( "~" | "null" | "Null" | "NULL" )? ;
BOOLYES = ( "yes" | "Yes" | "YES" | "true" | "True" | "TRUE" | "on" | "On" | "ON" ) ;
BOOLNO = ( "no" | "No" | "NO" | "false" | "False" | "FALSE" | "off" | "Off" | "OFF" ) ;
TIMEZ = ( "Z" | [-+] DIGIT DIGIT ( ":" DIGIT DIGIT )? ) ;
TIMEYMD = YEAR "-" MON "-" MON ;
TIMEISO = YEAR "-" MON "-" MON [Tt] MON ":" MON ":" MON ( "." DIGIT* )? TIMEZ ;
TIMESPACED = YEAR "-" MON "-" MON [ \t]+ MON ":" MON ":" MON ( "." DIGIT* )? [ \t]+ TIMEZ ;
TIMECANON = YEAR "-" MON "-" MON "T" MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? "Z" ;
MERGE = "<<" ;
DEFAULTKEY = "=" ;

NULLTYPE NULL       {   return "null"; }

BOOLYES NULL        {   return "bool#yes"; }

BOOLNO NULL         {   return "bool#no"; }

INTHEX NULL         {   return "int#hex"; }

INTOCT NULL         {   return "int#oct"; }

INTSIXTY NULL       {   return "int#base60"; }

INTCANON NULL       {   return "int"; }

FLOATFIX NULL       {   return "float#fix"; }

FLOATEXP NULL       {   return "float#exp"; }

FLOATSIXTY NULL     {   return "float#base60"; }

FLOATINF NULL       {   return "float#inf"; }

FLOATNEGINF NULL    {   return "float#neginf"; }

FLOATNAN NULL       {   return "float#nan"; }

TIMEYMD NULL        {   return "timestamp#ymd"; }

TIMEISO NULL        {   return "timestamp#iso8601"; }

TIMESPACED NULL     {   return "timestamp#spaced"; }

TIMECANON NULL      {   return "timestamp"; }

DEFAULTKEY NULL     {   return "default"; }

MERGE NULL          {   return "merge"; }

ANY                 {   return "str"; }

*/

}

/* Remove ending fragment and compare types */
int
syck_tagcmp( const char *tag1, const char *tag2 )
{
    if ( tag1 == tag2 ) return 1;
    if ( tag1 == NULL || tag2 == NULL ) return 0;
    else {
        int i;
        char *othorpe;
        char *tmp1 = syck_strndup( tag1, strlen( tag1 ) );
        char *tmp2 = syck_strndup( tag2, strlen( tag2 ) );
        othorpe = strstr( tmp1, "#" );
		if ( othorpe != NULL ) {
            othorpe[0] = '\0';
        }
        othorpe = strstr( tmp2, "#" );
		if ( othorpe != NULL ) {
            othorpe[0] = '\0';
        }
        i = strcmp( tmp1, tmp2 );
        S_FREE( tmp1 ); S_FREE( tmp2 );
        return i;
    }
}

char *
syck_type_id_to_uri( char *type_id )
{
    char *cursor, *limit, *marker;

    cursor = type_id;
    limit = type_id + strlen( type_id );

/*!re2c

TAG = "tag" ;
XPRIVATE = "x-private" ;
WD = [A-Za-z0-9_] ;
WDD = [A-Za-z0-9_-] ;
DNSCOMPRE = WD ( WDD* WD )? ;
DNSNAMERE = ( ( DNSCOMPRE "." )+ DNSCOMPRE | DNSCOMPRE ) ;
TAGDATE = YEAR ( "-" MON )? ( "-" MON )? ;

TAG ":" DNSNAMERE "," TAGDATE ":"    {   return syck_strndup( type_id, strlen( type_id ) ); }

XPRIVATE ":"    {   return syck_strndup( type_id, strlen( type_id ) ); }

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

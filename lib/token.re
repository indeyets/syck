//
// token.re
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"
#include "gram.h"

//
// Allocate quoted strings in chunks
//
#define QUOTELEN    1024

//
// They do my bidding...
//
#define YYCTYPE     char
#define YYCURSOR    parser->cursor
#define YYMARKER    parser->marker
#define YYLIMIT     parser->limit
#define YYTOKEN     parser->token
#define YYTOKTMP    parser->toktmp
#define YYLINEPTR   parser->lineptr
#define YYLINE      parser->linect
#define YYFILL(n)   syck_parser_read(parser);

//
// Repositions the cursor at `n' offset from the token start.
// Only works in `Header' and `Document' sections.
//
#define YYPOS(n)    YYCURSOR = YYTOKEN + n;

//
// Track line numbers
//
#define NEWLINE(ptr)    YYLINE++; YYLINEPTR = ptr + 1;

//
// I like seeing the level operations as macros...
//
#define ADD_LEVEL(len)  syck_parser_add_level( parser, len );
#define CURRENT_LEVEL() syck_parser_current_level( parser );

//
// Nice little macro to ensure we're IOPENed to the current level.
// * Only use this macro in the "Document" *
//
#define ENSURE_IOPEN(last_lvl, to_len, reset) \
        if ( last_lvl->spaces < to_len ) \
        { \
            ADD_LEVEL( to_len ); \
            if ( reset == 1 ) YYPOS(0); \
            return IOPEN; \
        } 

//
// Nice little macro to ensure closure of levels.
// * Only use this macro in the "Document" *
//
#define ENSURE_IEND(last_lvl, to_len) \
        if ( last_lvl->spaces > to_len ) \
        { \
            syck_parser_pop_level( parser ); \
            YYPOS(0); \
            return IEND; \
        }

//
// Concatenates quoted string items and manages allocation
// to the quoted string
//
#define QUOTECAT(s, c, i, l) \
        { \
            if ( i + 1 >= c ) \
            { \
                c += QUOTELEN; \
                S_REALLOC_N( s, char, c ); \
            } \
            s[i++] = l; \
            s[i] = '\0'; \
        }

//
// Argjh!  I hate globals!  Here for yyerror() only!
//
SyckParser *syck_parser_ptr = NULL;

//
// My own re-entrant yylex() using re2c.
// You really get used to the limited regexp.
// It's really nice to not rely on backtracking and such.
//
int
yylex( YYSTYPE *yylval, SyckParser *parser )
{
    syck_parser_ptr = parser;
    if ( YYCURSOR == NULL ) syck_parser_read( parser );

/*!re2c

WORDC = [A-Za-z0-9_-\.]+ ;
LF = [\n]+ ;
INDENT = LF [ ]* ;
ENDSPC = [ \n]+ ;
NULL = [\000] ;
ANY = [\001-\377] ;
ODELIMS = [\{\[] ;
CDELIMS = [\}\]] ;
INLINEX = ( CDELIMS | "," ENDSPC ) ;
ALLX = ( ":" ENDSPC ) ;
DIR = "#" WORDC ":" WORDC ;

DIGITS = [0-9] ;
SIGN = [-+] ;
HEX = [0-9a-fA-F,] ;
OCT = [0-7,] ;
INTHEX = SIGN? "0x" HEX+ ; 
INTOCT = SIGN? "0" OCT+ ;
INTCANON = SIGN? DIGITS ( DIGITS | "," )* ;
NULLTYPE = ( "~" | "null" | "Null" | "NULL" ) ;
BOOLYES = ( "true" | "True" | "TRUE" | "yes" | "Yes" | "YES" | "on" | "On" | "ON" ) ;
BOOLNO = ( "false" | "False" | "FALSE" | "no" | "No" | "NO" | "off" | "Off" | "OFF" ) ;

*/

    if ( YYLINEPTR != YYCURSOR )
        goto Document;

Header:

    YYTOKEN = YYCURSOR;

/*!re2c

"---" ENDSPC        {   goto Directive; }

ANY                 {   YYPOS(0);
                        goto Document; 
                    }

*/

Document:

    YYTOKEN = YYCURSOR;

/*!re2c

INDENT              {   // Isolate spaces
                        int indt_len;
                        SyckLevel *lvl;
                        char *indent = YYTOKEN;
                        NEWLINE(indent);
                        while ( indent < YYCURSOR ) 
                        { 
                            if ( *(++indent) != '\n' ) 
                                break; 
                            else
                                NEWLINE(indent);
                        }

                        // Calculate indent length
                        lvl = CURRENT_LEVEL();
                        indt_len = 0;
                        if ( *indent == ' ' ) indt_len = YYCURSOR - indent;

                        // Check for open indent
#ifdef REDEBUG
                        printf( "CALLING CURRENT AT INDENT: %d\n", indt_len );
                        printf( "CURSOR: %s\n", YYCURSOR );
#endif
                        ENSURE_IEND(lvl, indt_len);
                        ENSURE_IOPEN(lvl, indt_len, 0);
                        return INDENT;
                    }

ODELIMS             {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IOPEN(lvl, 0, 1);
                        lvl = CURRENT_LEVEL();
                        lvl->status = syck_lvl_inline;
                        return YYTOKEN[0]; 
                    }

CDELIMS             {   SyckLevel *lvl = CURRENT_LEVEL();
                        lvl->status = syck_lvl_implicit;
                        return YYTOKEN[0]; 
                    }

[:,] ENDSPC         {   YYPOS(1); 
                        return YYTOKEN[0]; 
                    }

[-?] ENDSPC         {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IOPEN(lvl, YYTOKEN - YYLINEPTR, 1);
                        YYPOS(1); 
                        return YYTOKEN[0]; 
                    }

"&" WORDC           {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return ANCHOR;
                    }

"*" WORDC           {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return ALIAS;
                    }

"!" WORDC           {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return TRANSFER;
                    }

"'"                 {   goto SingleQuote; 
                    }

"\""                {   goto DoubleQuote; 
                    }

[ ]+                {   goto Document; }

NULL                {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IEND(lvl, -1);
                        return 0; 
                    }

ANY                 {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IOPEN(lvl, 0, 1);
                        goto Plain; 
                    }

*/

Directive:
    {
        YYTOKTMP = YYCURSOR;

/*!re2c

DIR                 {   ; }

ANY                 {   YYCURSOR = YYTOKTMP;
                        return DOCSEP;
                    }
*/

    }

Plain:
    {
        SyckLevel *plvl;
        YYCURSOR = YYTOKEN;
        plvl = CURRENT_LEVEL();

Plain2: 
        YYTOKTMP = YYCURSOR;

/*!re2c

ALLX                {   YYCURSOR = YYTOKTMP;
                        goto Implicit; }

INLINEX             {   if ( plvl->status != syck_lvl_inline ) goto Plain2;
                        YYCURSOR = YYTOKTMP;
                        goto Implicit; 
                    }

( LF | NULL )       {   YYCURSOR = YYTOKTMP;
                        goto Implicit; 
                    }

ANY                 {   goto Plain2; }

*/
    }

Implicit:
    {
        char cursch = YYCURSOR[0];
        YYCURSOR[0] = '\0';
        YYTOKTMP = YYCURSOR;
        YYCURSOR = YYTOKEN;

Implicit2:

#define TAG_IMPLICIT( tid ) \
    YYCURSOR = YYTOKTMP; \
    yylval->nodeData = syck_new_str2( YYTOKEN, YYCURSOR - YYTOKEN ); \
    yylval->nodeData->type_id = tid; \
    YYCURSOR[0] = cursch; \
    return PLAIN;

/*!re2c

NULLTYPE NULL       {   TAG_IMPLICIT( "null" ); }

BOOLYES NULL        {   TAG_IMPLICIT( "bool#yes" ); }

BOOLNO NULL         {   TAG_IMPLICIT( "bool#no" ); }

INTHEX NULL         {   TAG_IMPLICIT( "int#hex" ); }

INTOCT NULL         {   TAG_IMPLICIT( "int#oct" ); }

INTCANON NULL       {   TAG_IMPLICIT( "int" ); }

ANY                 {   TAG_IMPLICIT( "str" ); }

*/
    }

SingleQuote:
    {
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );

SingleQuote2:

/*!re2c

"''"                {   QUOTECAT(qstr, qcapa, qidx, '\'');
                        goto SingleQuote2; 
                    }
"'"                 {   SyckNode *n = syck_alloc_str();
                        n->data.str->ptr = qstr;
                        n->data.str->len = qidx;
                        yylval->nodeData = n;
                        return PLAIN; 
                    }
ANY                 {   QUOTECAT(qstr, qcapa, qidx, *(YYCURSOR - 1)); 
                        goto SingleQuote2; 
                    }

*/

    }


DoubleQuote:
    {
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );

DoubleQuote2:

/*!re2c

"\\\""              {   QUOTECAT(qstr, qcapa, qidx, '"');
                        goto DoubleQuote2; 
                    }

"\""                {   SyckNode *n = syck_alloc_str();
                        n->data.str->ptr = qstr;
                        n->data.str->len = qidx;
                        yylval->nodeData = n;
                        return PLAIN; 
                    }

ANY                 {   QUOTECAT(qstr, qcapa, qidx, *(YYCURSOR - 1)); 
                        goto DoubleQuote2; 
                    }

*/

    }

}

int 
yywrap()
{
    return 1;
}

void 
yyerror( char *msg )
{
    if ( syck_parser_ptr->error_handler == NULL )
        syck_parser_ptr->error_handler = syck_default_error_handler;

    (syck_parser_ptr->error_handler)(syck_parser_ptr, msg);
}


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
#define YYFILL(n)   syck_parser_read(parser)

//
// Repositions the cursor at `n' offset from the token start.
// Only works in `Header' and `Document' sections.
//
#define YYPOS(n)    YYCURSOR = YYTOKEN + n

//
// Track line numbers
//
#define NEWLINE(ptr)    YYLINE++; YYLINEPTR = ptr + 1

//
// I like seeing the level operations as macros...
//
#define ADD_LEVEL(len)  syck_parser_add_level( parser, len )
#define POP_LEVEL()     syck_parser_pop_level( parser )
#define CURRENT_LEVEL() syck_parser_current_level( parser )

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

#define RETURN_IMPLICIT() \
    { \
        YYCURSOR = YYTOKTMP; \
        yylval->nodeData = syck_new_str2( YYTOKEN, YYCURSOR - YYTOKEN ); \
        if ( parser->implicit_typing == 1 ) \
        { \
            try_tag_implicit( yylval->nodeData, parser->taguri_expansion ); \
        } \
        return PLAIN; \
    }

#define GOBBLE_UP_INDENT( ict, start ) \
    char *indent = start; \
    NEWLINE(indent); \
    while ( indent < YYCURSOR ) \
    { \
        if ( *(++indent) == '\n' ) \
        { \
            NEWLINE(indent); \
        } \
    } \
    ict = 0; \
    if ( *YYCURSOR == '\0' ) \
    { \
        ict = -1; \
        start = YYCURSOR; \
    } \
    else if ( *YYLINEPTR == ' ' ) \
    { \
        ict = YYCURSOR - YYLINEPTR; \
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

WORDC = [A-Za-z0-9_-\.] ;
LF = [\n]+ ;
ENDSPC = [ \n] ;
INDENT = LF ENDSPC* ;
NULL = [\000] ;
ANY = [\001-\377] ;
ODELIMS = [\{\[] ;
CDELIMS = [\}\]] ;
INLINEX = ( CDELIMS | "," ENDSPC ) ;
ALLX = ( ":" ENDSPC ) ;
DIR = "%" WORDC+ ":" WORDC+ ;
BLOCK = [>|] [-+0-9]* LF ; 

*/

    if ( YYLINEPTR != YYCURSOR )
    {
        goto Document;
    }

Header:

    YYTOKEN = YYCURSOR;

/*!re2c

"---" ENDSPC        {   SyckLevel *lvl = CURRENT_LEVEL();
                        if ( lvl->status == syck_lvl_header )
                        {
                            YYPOS(3);
                            goto Directive; 
                        }
                        else
                        {
                            ENSURE_IEND(lvl, -1);
                            YYPOS(0);
                            return 0; 
                        }
                    }

INDENT              {   int indt_len;
                        GOBBLE_UP_INDENT( indt_len, YYTOKEN );
                        goto Header; }

ANY                 {   YYPOS(0);
                        goto Document; 
                    }

*/

Document:
    {
        SyckLevel *lvl = CURRENT_LEVEL();
        if ( lvl->status == syck_lvl_header )
        {
            lvl->status = syck_lvl_implicit;
        }

        YYTOKEN = YYCURSOR;

/*!re2c

INDENT              {   // Isolate spaces
                        int indt_len;
                        GOBBLE_UP_INDENT( indt_len, YYTOKEN );
                        lvl = CURRENT_LEVEL();

                        // Check for open indent
                        ENSURE_IEND(lvl, indt_len);
                        ENSURE_IOPEN(lvl, indt_len, 0);
                        return INDENT;
                    }

ODELIMS             {   ENSURE_IOPEN(lvl, 0, 1);
                        lvl = CURRENT_LEVEL();
                        lvl->status = syck_lvl_inline;
                        return YYTOKEN[0]; 
                    }

CDELIMS             {   lvl->status = syck_lvl_implicit;
                        return YYTOKEN[0]; 
                    }

[:,] ENDSPC         {   YYPOS(1); 
                        return YYTOKEN[0]; 
                    }

[-?] ENDSPC         {   ENSURE_IOPEN(lvl, YYTOKEN - YYLINEPTR, 1);
                        YYPOS(1); 
                        return YYTOKEN[0]; 
                    }

"&" WORDC+          {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return ANCHOR;
                    }

"*" WORDC+          {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return ALIAS;
                    }

"!"                 {   goto TransferMethod; }

"'"                 {   goto SingleQuote; 
                    }

"\""                {   goto DoubleQuote; 
                    }

BLOCK               {   YYCURSOR--;
                        goto ScalarBlock; 
                    }

[ ]+                {   goto Document; }

NULL                {   ENSURE_IEND(lvl, -1);
                        return 0; 
                    }

ANY                 {   ENSURE_IOPEN(lvl, 0, 1);
                        goto Plain; 
                    }

*/
    }

Directive:
    {
        YYTOKTMP = YYCURSOR;

/*!re2c

DIR                 {   goto Directive; }

[ ]+                {   goto Directive; }

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
                        RETURN_IMPLICIT();
                    }

INLINEX             {   if ( plvl->status != syck_lvl_inline ) goto Plain2;
                        YYCURSOR = YYTOKTMP;
                        RETURN_IMPLICIT();
                    }

( LF | NULL )       {   YYCURSOR = YYTOKTMP;
                        RETURN_IMPLICIT();
                    }

ANY                 {   goto Plain2; }

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

( "'" | NULL )      {   SyckNode *n = syck_alloc_str();
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

( "\"" | NULL )     {   SyckNode *n = syck_alloc_str();
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

TransferMethod:
    {
        YYTOKTMP = YYCURSOR;

/*!re2c

ENDSPC              {   SyckLevel *lvl;
                        YYCURSOR = YYTOKTMP;
                        if ( YYCURSOR == YYTOKEN + 1 )
                        {
                            return ITRANSFER;
                        }

                        lvl = CURRENT_LEVEL();
                        if ( *(YYTOKEN + 1) == '^' )
                        {
                            yylval->name = S_ALLOC_N( char, YYCURSOR - YYTOKEN + strlen( lvl->domain ) );
                            yylval->name[0] = '\0';
                            strcat( yylval->name, lvl->domain );
                            strncat( yylval->name, YYTOKEN + 2, YYCURSOR - YYTOKEN - 2 );
                        }
                        else
                        {
                            char *carat = YYTOKEN + 1;
                            while ( (++carat) < YYCURSOR )
                            {
                                if ( *carat == '^' )
                                    break;
                            }

                            if ( carat < YYCURSOR )
                            {
                                lvl->domain = syck_strndup( YYTOKEN + 1, carat - YYTOKEN - 1 );
                                yylval->name = S_ALLOC_N( char, YYCURSOR - carat + strlen( lvl->domain ) );
                                yylval->name[0] = '\0';
                                strcat( yylval->name, lvl->domain );
                                strncat( yylval->name, carat + 1, YYCURSOR - carat - 1 );
                            }
                            else
                            {
                                yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                            }
                        }
                        return TRANSFER; 
                    }

ANY                 {   goto TransferMethod; }

*/
    }

ScalarBlock:
    {
        YYTOKTMP = YYCURSOR;

/*!re2c

INDENT              {   int indt_len;
                        SyckLevel *lvl;
                        GOBBLE_UP_INDENT( indt_len, YYTOKTMP );
                        lvl = CURRENT_LEVEL();

                        if ( indt_len > lvl->spaces && lvl->status != syck_lvl_block )
                        {
                            ADD_LEVEL( indt_len );
                            lvl = CURRENT_LEVEL();
                            lvl->status = syck_lvl_block;
                        }
                        else if ( indt_len < lvl->spaces )
                        {
                            POP_LEVEL();
                            YYCURSOR = YYTOKTMP;
                            yylval->nodeData = syck_new_str2( YYTOKEN, YYCURSOR - YYTOKEN );  
                            return BLOCK;
                        }
                        goto ScalarBlock;
                    }


NULL                {   POP_LEVEL();
                        YYCURSOR = YYTOKTMP;
                        yylval->nodeData = syck_new_str2( YYTOKEN, YYCURSOR - YYTOKEN );  
                        return BLOCK; 
                    }

ANY                 {   goto ScalarBlock; }

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


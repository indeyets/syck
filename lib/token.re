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

enum {
    STATE_PLAIN = 0,
    STATE_SINGLE_QUOTE,
    STATE_DOUBLE_QUOTE
};

#define QUOTELEN    1024
#define YYCTYPE     char
#define YYCURSOR    parser->cursor
#define YYMARKER    parser->marker
#define YYLIMIT     parser->limit
#define YYTOKEN     parser->token
#define YYLINEPTR   parser->lineptr
#define YYLINE      parser->linect
#define YYFILL(n)   syck_parser_read(parser);
#define YYPOS(n)    YYCURSOR = YYTOKEN + n;

#define NEWLINE(ptr)    YYLINE++; YYLINEPTR = ptr + 1;
#define ADD_LEVEL(len)  syck_parser_add_level( parser, len );
#define CURRENT_LEVEL() syck_parser_current_level( parser );

#define ENSURE_IOPEN(last_lvl, to_len, reset) \
        if ( last_lvl->spaces < to_len ) \
        { \
            ADD_LEVEL( to_len ); \
            if ( reset == 1 ) YYPOS(0); \
            return IOPEN; \
        } 

#define ENSURE_IEND(last_lvl, to_len) \
        if ( last_lvl->spaces > to_len ) \
        { \
            syck_parser_pop_level( parser ); \
            YYPOS(0); \
            return IEND; \
        }

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

SyckParser *syck_parser_ptr = NULL;

int
yylex( YYSTYPE *yylval, SyckParser *parser )
{
    int state;
    syck_parser_ptr = parser;
    if ( parser->cursor == NULL ) syck_parser_read( parser );

/*!re2c

WORD = [A-Za-z0-9_]+ ;
LF = [\n]+ ;
INDENT = LF [ ]* ;
ENDSPC = [ \n]+ ;
NULL = [\000] ;
ANY = [\001-\377] ;
ODELIMS = [\{\[] ;
CDELIMS = [\}\]] ;
INLINEX = ( CDELIMS | "," ENDSPC ) ;
ALLX = ( ":" ENDSPC ) ;

*/

Document:

    YYTOKEN = YYCURSOR;
    state = STATE_PLAIN;

#ifdef REDEBUG
    printf( "TOKEN: %s\n", YYTOKEN );
#endif

/*!re2c

"---" ENDSPC        {   YYPOS(3);
                        return DOCSEP; 
                    }

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
                        ENSURE_IOPEN(lvl, parser->token - parser->lineptr, 1);
                        YYPOS(1); 
                        return YYTOKEN[0]; 
                    }

"&" WORD            {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return ANCHOR;
                    }

"*" WORD            {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return ALIAS;
                    }

"!" WORD            {   yylval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
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
                        goto Plain; }

*/

Plain:
    {
        SyckLevel *plvl;
        char *plaintok;
        YYCURSOR = YYTOKEN;
        plvl = CURRENT_LEVEL();

Plain2: 
        plaintok = YYCURSOR;

/*!re2c

ALLX                {   YYCURSOR = plaintok;
                        goto Implicit; }

INLINEX             {   if ( plvl->status != syck_lvl_inline ) goto Plain2;
                        YYCURSOR = plaintok;
                        goto Implicit; 
                    }

( LF | NULL )       {   YYCURSOR = plaintok;
                        goto Implicit; 
                    }

ANY                 {   goto Plain2; }

*/
    }

Implicit:
    {
        char *impend = YYCURSOR;
        YYCURSOR = YYTOKEN;

Implicit2:

/*!re2c

ANY                 {   YYCURSOR = impend;
                        yylval->nodeData = syck_new_str2( YYTOKEN, YYCURSOR - YYTOKEN );
                        return PLAIN;
                    }
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


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

#define YYCTYPE     char
#define YYCURSOR    parser->cursor
#define YYMARKER    parser->marker
#define YYLIMIT     parser->limit
#define YYTOKEN     parser->token
#define YYFILL(n)   syck_parser_read(parser);
#define YYPOS(n)    YYCURSOR = YYTOKEN + n;

#define ADD_LEVEL(len) syck_parser_add_level( parser, len );
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

int
yylex( YYSTYPE *yylval, SyckParser *parser )
{
    if ( parser->cursor == NULL ) syck_parser_read( parser );

/*!re2c

WORD = [A-Za-z0-9_]+ ;
LF = [\n]+ ;
INDENT = LF [ ]* ;
ENDSPC = [ \n]+ ;
NULL = [\000] ;
ANY = [\001-\377] ;
DELIMS = [\{\}\[\]] ;

*/

Implicit:

    YYTOKEN = YYCURSOR;
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
                        while ( indent < YYCURSOR ) { if ( *(++indent) != '\n' ) break; }

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

DELIMS              {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IOPEN(lvl, 0, 1);
                        return YYTOKEN[0]; }

[-:,?] ENDSPC       {   YYPOS(1); 
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

WORD                {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IOPEN(lvl, 0, 1);
                        yylval->nodeData = syck_new_str2( YYTOKEN, YYCURSOR - YYTOKEN );
                        return PLAIN;
                    }

[ ]+                {   goto Implicit; }

NULL                {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_IEND(lvl, -1);
                        return 0; 
                    }

ANY                 {   printf( "Unrecognized character: %s\n", YYTOKEN ); }

*/

}

int 
yywrap()
{
    return 1;
}

void 
yyerror( char *msg )
{
    printf( "Parse error: %s\n", msg );
}


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
#define YYFILL(n)   syck_parser_read(parser);

#define ADD_LEVEL(len) syck_parser_add_level( parser, len );
#define CURRENT_LEVEL() syck_parser_current_level( parser );

int
yylex( YYSTYPE *yylval, SyckParser *parser )
{
    if ( parser->cursor == NULL ) syck_parser_read( parser );

/*!re2c

WORD = [A-Za-z0-9_]+ ;
LF = [\n] ;
INDENT = LF [ ]* ;
ENDSPC = [ \n]+ ;
NULL = [\000] ;
ANY = [\001-\377] ;
DELIMS = [\{\}\[\]] ;

*/

Implicit:

    parser->token = YYCURSOR;
#ifdef REDEBUG
    printf( "TOKEN: %s\n", parser->token );
#endif

/*!re2c

"---" ENDSPC    { return DOCSEP; }

INDENT      {   // Isolate spaces
                int indt_len;
                SyckLevel *lvl;
                char *indent = parser->token;
                char *endent = YYCURSOR;
                while ( indent < endent ) { if ( *(indent++) != '\n' ) break; }

                // Indent open?
                lvl = CURRENT_LEVEL();
                indt_len = endent - indent;
                if ( lvl->spaces < indt_len )
                {
                    ADD_LEVEL( indt_len );
                    return IOPEN;
                } 
                return INDENT;
            }

DELIMS          { return parser->token[0]; }
[-:,?] ENDSPC   { return parser->token[0]; }

"&" WORD            {   yylval->name = syck_strndup( parser->token + 1, YYCURSOR - parser->token - 1 );
                        return ANCHOR;
                    }

"*" WORD            {   yylval->name = syck_strndup( parser->token + 1, YYCURSOR - parser->token - 1 );
                        return ALIAS;
                    }

"!" WORD            {   yylval->name = syck_strndup( parser->token + 1, YYCURSOR - parser->token - 1 );
                        return TRANSFER;
                    }

WORD                {  yylval->nodeData = syck_new_str2( parser->token, YYCURSOR - parser->token );
                       return PLAIN;
                    }

[ ]+                {   goto Implicit; }

NULL                {   return 0; }

ANY                 {   printf( "Unrecognized character: %s\n", parser->token ); }

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


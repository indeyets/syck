%{
//
// token.l
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "syck.h"
#include "gram.h"

static SyckParser *syck_lex_parser = NULL;

int yydebug;

#define YY_INPUT(buf,result,max_size) result = syck_lex_read(buf, syck_lex_parser, max_size)

int
syck_lex_read( char *buf, SyckParser *p, int max_size )
{
    int len = syck_parser_read(buf, syck_lex_parser, max_size); 
}

void
syck_parser_init(SyckParser *p, int debug)
{
    syck_lex_parser = p;
    yydebug = debug;
}

%}

WORD [[:alnum:]_]
NONSPC [^[:blank:]]

%%

^"---"([ ]+)        return DOCSEP;
^"---"$             return DOCSEP;

"{"         return '{';
"}"         return '}';
"["         return '[';
"]"         return ']';

[-:,?]([ ]+)    return yytext[0];
[-:,?]$         return yytext[0];

"&"{WORD}+          {   yylval.name = syck_strndup( yytext + 1, yyleng - 1 );
                        return ANCHOR;
                    }

"*"{WORD}+          {   yylval.name = syck_strndup( yytext + 1, yyleng - 1 );
                        return ALIAS;
                    }

"!"{NONSPC}+        {   yylval.name = syck_strndup( yytext + 1, yyleng - 1 );
                        return TRANSFER;
                    }

{WORD}+             {   yylval.nodeData = syck_new_str2( yytext, yyleng );
                        return PLAIN;
                    }

([[:blank:]]+)

.                   printf( "Unrecognized character: %s\n", yytext );

%%


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

/*
 * bytecode.re
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 */
#include "syck.h"
#include "gram.h"

#define QUOTELEN 128

/*
 * They do my bidding...
 */
#define YYCTYPE     char
#define YYCURSOR    parser->cursor
#define YYMARKER    parser->marker
#define YYLIMIT     parser->limit
#define YYTOKEN     parser->token
#define YYTOKTMP    parser->toktmp
#define YYLINEPTR   parser->lineptr
#define YYLINECTPTR parser->linectptr
#define YYLINE      parser->linect
#define YYFILL(n)   syck_parser_read(parser)

extern SyckParser *syck_parser_ptr;

char *get_inline( SyckParser *parser );

/*
 * Repositions the cursor at `n' offset from the token start.
 * Only works in `Header' and `Document' sections.
 */
#define YYPOS(n)    YYCURSOR = YYTOKEN + n

/*
 * I like seeing the level operations as macros...
 */
#define ADD_LEVEL(len, status)  syck_parser_add_level( parser, len, status )
#define POP_LEVEL()     syck_parser_pop_level( parser )
#define CURRENT_LEVEL() syck_parser_current_level( parser )

/*
 * Adding levels in bytecode requires us to make sure
 * we've got all our tokens worked out.
 */
#define ADD_BYTE_LEVEL(lvl, len, s ) \
        switch ( lvl->status ) \
        { \
            case syck_lvl_seq: \
                lvl->ncount++; \
                ADD_LEVEL(len, syck_lvl_open); \
                YYPOS(0); \
            return '-'; \
        \
            case syck_lvl_open: \
                lvl->status = s; \
            break; \
        \
            default: \
                ADD_LEVEL(len, s); \
            break; \
        }

/*
 * Nice little macro to ensure we're YAML_IOPENed to the current level.
 * * Only use this macro in the "Document" section *
 */
#define ENSURE_YAML_IOPEN(last_lvl, lvl_type, to_len, reset) \
        if ( last_lvl->spaces < to_len ) \
        { \
            if ( last_lvl->status == syck_lvl_inline ) \
            { \
                goto Document; \
            } \
            else \
            { \
                ADD_LEVEL( to_len, lvl_type ); \
                if ( reset == 1 ) YYPOS(0); \
                return YAML_IOPEN; \
            } \
        } 

/*
 * Nice little macro to ensure closure of levels.
 * * Only use this macro in the "Document" section *
 */
#define ENSURE_YAML_IEND(last_lvl, to_len) \
        if ( last_lvl->spaces > to_len ) \
        { \
            syck_parser_pop_level( parser ); \
            YYPOS(0); \
            return YAML_IEND; \
        }

/*
 * Concatenates string items and manages allocation
 * to the string
 */
#define CAT(s, c, i, l) \
        { \
            if ( s == NULL ) \
            { \
                s = S_ALLOC_N( char, QUOTELEN ); \
                c = QUOTELEN; \
            } \
            if ( i + 1 >= c ) \
            { \
                c += QUOTELEN; \
                S_REALLOC_N( s, char, c ); \
            } \
            s[i++] = l; \
            s[i] = '\0'; \
        }

/*
 * Parser for standard YAML Bytecode [UTF-8]
 */
int
sycklex_bytecode_utf8( YYSTYPE *sycklval, SyckParser *parser )
{
    SyckLevel *lvl;
    int doc_level = 0;
    syck_parser_ptr = parser;
    if ( YYCURSOR == NULL ) 
    {
        syck_parser_read( parser );
    }

/*!re2c

LF = ( "\n" | "\r\n" ) ;
NULL = [\000] ;
ANY = [\001-\377] ;
YWORDC = [A-Za-z0-9_-] ;
YWORDP = [A-Za-z0-9_-\.] ;

DOC = "D" LF ;
DIR = "V" YWORDP+ ":" YWORDP+ LF ;
PAU = "P" LF ;
MAP = "M" LF ;
SEQ = "Q" LF ;
END = "E" LF ;
SCA = "S" ;
SCC = "C" ;
NNL = "N" ;
NUL = "Z" ;
ALI = "A" ;
REF = "R" ;

*/

    lvl = CURRENT_LEVEL();
    if ( lvl->status == syck_lvl_doc )
    {
        goto Document;
    }

Header:

    YYTOKEN = YYCURSOR;

/*!re2c

DOC     {   if ( lvl->status == syck_lvl_header )
            {
                goto Directive;
            }
            else
            {
                ENSURE_YAML_IEND(lvl, -1);
                YYPOS(0);
                return 0;
            }
        }

ANY     {   YYPOS(0);
            goto Document;
        }

*/

    lvl->status = syck_lvl_doc;

Document:
    {
        lvl = CURRENT_LEVEL();
        if ( lvl->status == syck_lvl_header )
        {
            lvl->status = syck_lvl_doc;
        }

        YYTOKEN = YYCURSOR;

/*!re2c

MAP     {   ADD_BYTE_LEVEL(lvl, lvl->spaces + 1, syck_lvl_map); 
            return YAML_IOPEN;
        }

SEQ     {   ADD_BYTE_LEVEL(lvl, lvl->spaces + 1, syck_lvl_seq);
            return YAML_IOPEN;
        }

END     {   POP_LEVEL();
            return YAML_IEND;
        }

SCA     {   ADD_BYTE_LEVEL(lvl, lvl->spaces + 1, syck_lvl_str); 
            goto Scalar;
        }

ALI     {   ADD_BYTE_LEVEL(lvl, lvl->spaces + 1, syck_lvl_doc);
            sycklval->name = get_inline( parser );
            return YAML_ALIAS;
        }

REF     {   ADD_BYTE_LEVEL(lvl, lvl->spaces + 1, syck_lvl_doc);
            sycklval->name = get_inline( parser );
            syck_hdlr_remove_anchor( parser, sycklval->name );
            POP_LEVEL();
            return YAML_ANCHOR;
        }

LF      {   if ( lvl->status == syck_lvl_seq )
            {
                return YAML_INDENT; 
            }
            else if ( lvl->status == syck_lvl_map )
            {
                lvl->ncount++;
                if ( lvl->ncount % 2 == 1 ) return ':';
                else                        return YAML_INDENT;
            }
            goto Document;
        }

NULL    {   ENSURE_YAML_IEND(lvl, -1);
            YYPOS(0);
            return 0;
        }

*/

    }

Directive:
    {
        YYTOKTMP = YYCURSOR;

/*!re2c

DIR        {   goto Directive; }

ANY        {   YYCURSOR = YYTOKTMP;
               return YAML_DOCSEP;
           }
*/

    }

Scalar:
    {
    int idx = 0;
    int cap = 0;
    char *str = NULL;
    char *tok;

Scalar2:
    tok = YYCURSOR;

/*!re2c

LF SCC  {   goto Scalar2; }

LF NUL  {   CAT(str, cap, idx, '\0');
            goto Scalar2; 
        }

LF      {   YYCURSOR = tok;
            goto ScalarEnd; 
        }

NULL    {   YYCURSOR = tok;
            goto ScalarEnd;
        }

ANY     {   CAT(str, cap, idx, tok[0]);
            goto Scalar2; 
        }

*/

ScalarEnd:
        {
            SyckNode *n = syck_alloc_str();
            n->data.str->ptr = str;
            n->data.str->len = idx;
            sycklval->nodeData = n;
            POP_LEVEL();
            return YAML_PLAIN;
        }
    }

}

char *
get_inline( SyckParser *parser )
{
    int idx = 0;
    int cap = 0;
    char *str = NULL;
    char *tok;

Inline:
    {
        tok = YYCURSOR;

/*!re2c

LF          {   return str; }

NULL        {   YYCURSOR = tok;
                return str;
            }

ANY         {   CAT(str, cap, idx, tok[0]);
                goto Inline; 
            }

*/

    }

}


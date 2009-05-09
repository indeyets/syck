/*
 * token.re
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 */
#include "syck.h"

#if GRAM_FILES_HAVE_TAB_SUFFIX
#include "gram.tab.h"
#else
#include "gram.h"
#endif

/*
 * Allocate quoted strings in chunks
 */
#define QUOTELEN    1024

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

/*
 * Repositions the cursor at `n' offset from the token start.
 * Only works in `Header' and `Document' sections.
 */
#define YYPOS(n)    YYCURSOR = YYTOKEN + n

/*
 * Track line numbers
 */
#define NEWLINE(ptr)    YYLINEPTR = ptr + newline_len(ptr); if ( YYLINEPTR > YYLINECTPTR ) { YYLINE++; YYLINECTPTR = YYLINEPTR; }

/*
 * I like seeing the level operations as macros...
 */
#define ADD_LEVEL(len, status)  syck_parser_add_level( parser, len, status )
#define POP_LEVEL()     syck_parser_pop_level( parser )
#define CURRENT_LEVEL() syck_parser_current_level( parser )

/*
 * Force a token next time around sycklex()
 */
#define FORCE_NEXT_TOKEN(tok)    parser->force_token = tok;

/*
 * Nice little macro to ensure we're YAML_IOPENed to the current level.
 * * Only use this macro in the "Document" section *
 */
#define ENSURE_YAML_IOPEN(last_lvl, to_len, reset) \
        if ( last_lvl->spaces < to_len ) \
        { \
            if ( last_lvl->status == syck_lvl_iseq || last_lvl->status == syck_lvl_imap ) \
            { \
                goto Document; \
            } \
            else \
            { \
                ADD_LEVEL( to_len, syck_lvl_doc ); \
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
 * Concatenates quoted string items and manages allocation
 * to the quoted string
 */
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

#define QUOTECATS(s, c, i, cs, cl) \
        { \
            while ( i + cl >= c ) \
            { \
                c += QUOTELEN; \
                S_REALLOC_N( s, char, c ); \
            } \
            S_MEMCPY( s + i, cs, char, cl ); \
            i += cl; \
            s[i] = '\0'; \
        }

/*
 * Tags a plain scalar with a transfer method
 * * Use only in "Plain" section *
 */
#define RETURN_IMPLICIT() \
    { \
        SyckNode *n = syck_alloc_str(); \
        YYCURSOR = YYTOKEN; \
        n->data.str->ptr = qstr; \
        n->data.str->len = qidx; \
        n->data.str->style = scalar_plain; \
        sycklval->nodeData = n; \
        if ( parser->implicit_typing == 1 ) \
        { \
            try_tag_implicit( sycklval->nodeData, parser->taguri_expansion ); \
        } \
        return YAML_PLAIN; \
    }

/* concat the inline characters to the plain scalar */
#define PLAIN_NOT_INL() \
    if ( *(YYCURSOR - 1) == ' ' || is_newline( YYCURSOR - 1 ) ) \
    { \
        YYCURSOR--; \
    } \
    QUOTECATS(qstr, qcapa, qidx, YYTOKEN, YYCURSOR - YYTOKEN); \
    goto Plain2;

/* trim spaces off the end in case of indent */
#define PLAIN_IS_INL() \
    char *walker = qstr + qidx - 1; \
    while ( walker > qstr && ( *walker == '\n' || *walker == ' ' || *walker == '\t' ) ) \
    { \
        qidx--; \
        walker[0] = '\0'; \
        walker--; \
    }

/*
 * Keep or chomp block?
 * * Use only in "ScalarBlock" section *
 */
#define RETURN_YAML_BLOCK() \
    { \
        SyckNode *n = syck_alloc_str(); \
        if ( ((SyckParser *)parser)->taguri_expansion == 1 ) \
        { \
            n->type_id = syck_taguri( YAML_DOMAIN, "str", 3 ); \
        } \
        else \
        { \
            n->type_id = syck_strndup( "str", 3 ); \
        } \
        n->data.str->ptr = qstr; \
        n->data.str->len = qidx; \
        if ( blockType == BLOCK_LIT ) { \
            n->data.str->style = scalar_literal; \
        } else { \
            n->data.str->style = scalar_fold; \
        } \
        if ( qidx > 0 ) \
        { \
            if ( nlDoWhat != NL_KEEP ) \
            { \
                char *fc = n->data.str->ptr + n->data.str->len - 1; \
                while ( is_newline( fc ) ) fc--; \
                if ( nlDoWhat != NL_CHOMP && fc < n->data.str->ptr + n->data.str->len - 1 ) \
                    fc += 1; \
                n->data.str->len = fc - n->data.str->ptr + 1; \
            } \
        } \
        sycklval->nodeData = n; \
        return YAML_BLOCK; \
    }

/*
 * Handles newlines, calculates indent
 */
#define GOBBLE_UP_YAML_INDENT( ict, start ) \
    char *indent = start; \
    NEWLINE(indent); \
    while ( indent < YYCURSOR ) \
    { \
        if ( *indent == '\t' ) \
        { \
            syckerror("TAB found in your indentation, please remove"); \
            return 0; \
        } \
        else if ( is_newline( ++indent ) ) \
        { \
            NEWLINE(indent); \
        } \
    } \
    ict = 0; \
    if ( *YYCURSOR == '\0' ) \
    { \
        ict = -1; \
        start = YYCURSOR - 1; \
    } \
    else if ( *YYLINEPTR == ' ' ) \
    { \
        ict = YYCURSOR - YYLINEPTR; \
    }

/*
 * If an indent exists at the current level, back up.
 */
#define GET_TRUE_YAML_INDENT(indt_len) \
    { \
        SyckLevel *lvl_deep = CURRENT_LEVEL(); \
        indt_len = lvl_deep->spaces; \
        if ( lvl_deep->status == syck_lvl_seq || ( indt_len == YYCURSOR - YYLINEPTR && lvl_deep->status != syck_lvl_map )  ) \
        { \
            SyckLevel *lvl_over; \
            parser->lvl_idx--; \
            lvl_over = CURRENT_LEVEL(); \
            indt_len = lvl_over->spaces; \
            parser->lvl_idx++; \
        } \
    }

/*
 * Argjh!  I hate globals!  Here for syckerror() only!
 */
SyckParser *syck_parser_ptr = NULL;

/*
 * Accessory funcs later in this file.
 */
void eat_comments( SyckParser * );
char escape_seq( char );
int is_newline( char *ptr );
int newline_len( char *ptr );
int sycklex_yaml_utf8( YYSTYPE *, SyckParser * );
int sycklex_bytecode_utf8( YYSTYPE *, SyckParser * );
int syckwrap();

/*
 * My own re-entrant sycklex() using re2c.
 * You really get used to the limited regexp.
 * It's really nice to not rely on backtracking and such.
 */
int
sycklex( YYSTYPE *sycklval, SyckParser *parser )
{
    switch ( parser->input_type )
    {
        case syck_yaml_utf8:
        return sycklex_yaml_utf8( sycklval, parser );

        case syck_yaml_utf16:
            syckerror( "UTF-16 is not currently supported in Syck.\nPlease contribute code to help this happen!" );
        break;

        case syck_yaml_utf32:
            syckerror( "UTF-32 is not currently supported in Syck.\nPlease contribute code to help this happen!" );
        break;

        case syck_bytecode_utf8:
        return sycklex_bytecode_utf8( sycklval, parser );
    }
    return YAML_DOCSEP;
}

/*
 * Parser for standard YAML [UTF-8]
 */
int
sycklex_yaml_utf8( YYSTYPE *sycklval, SyckParser *parser )
{
    int doc_level = 0;
    syck_parser_ptr = parser;
    if ( YYCURSOR == NULL ) 
    {
        syck_parser_read( parser );
    }

    if ( parser->force_token != 0 )
    {
        int t = parser->force_token;
        parser->force_token = 0;
        return t;
    }

/*!re2c

YWORDC = [A-Za-z0-9_-] ;
YWORDP = [A-Za-z0-9_-\.] ;
LF = ( "\n" | "\r\n" ) ;
SPC = " " ;
TAB = "\t" ;
SPCTAB = ( SPC | TAB ) ;
ENDSPC = ( SPC+ | LF ) ;
YINDENT = LF TAB* ( SPC | LF )* ;
NULL = [\000] ;
ANY = [\001-\377] ;
ISEQO = "[" ;
ISEQC = "]" ;
IMAPO = "{" ;
IMAPC = "}" ;
CDELIMS = ( ISEQC | IMAPC ) ;
ICOMMA = ( "," ENDSPC ) ;
ALLX = ( ":" ENDSPC ) ;
DIR = "%" YWORDP+ ":" YWORDP+ ;
YBLOCK = [>|] [-+0-9]* ENDSPC ; 
HEX = [0-9A-Fa-f] ;
ESCSEQ = ["\\abefnrtv0] ;

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
                            ENSURE_YAML_IEND(lvl, -1);
                            YYPOS(0);
                            return 0; 
                        }
                    }

"..." ENDSPC        {   SyckLevel *lvl = CURRENT_LEVEL();
                        if ( lvl->status == syck_lvl_header )
                        {
                            goto Header; 
                        }
                        else
                        {
                            ENSURE_YAML_IEND(lvl, -1);
                            YYPOS(0);
                            return 0; 
                        }
                        return 0; 
                    }

"#"                 {   eat_comments( parser ); 
                        goto Header;
                    }

NULL                {   SyckLevel *lvl = CURRENT_LEVEL();
                        ENSURE_YAML_IEND(lvl, -1);
                        YYPOS(0);
                        return 0; 
                    }

YINDENT             {   GOBBLE_UP_YAML_INDENT( doc_level, YYTOKEN );
                        goto Header; 
                    }

SPCTAB+             {   doc_level = YYCURSOR - YYLINEPTR;
                        goto Header;
                    }

ANY                 {   YYPOS(0);
                        goto Document; 
                    }

*/

Document:
    {
        SyckLevel *lvl = CURRENT_LEVEL();
        if ( lvl->status == syck_lvl_header )
        {
            lvl->status = syck_lvl_doc;
        }

        YYTOKEN = YYCURSOR;

/*!re2c

YINDENT             {   /* Isolate spaces */
                        int indt_len;
                        GOBBLE_UP_YAML_INDENT( indt_len, YYTOKEN );
                        lvl = CURRENT_LEVEL();
                        doc_level = 0;

                        /* XXX: Comment lookahead */
                        if ( *YYCURSOR == '#' )
                        {
                            goto Document;
                        }

                        /* Ignore indentation inside inlines */
                        if ( lvl->status == syck_lvl_iseq || lvl->status == syck_lvl_imap )
                        {
                            goto Document;
                        }

                        /* Check for open indent */
                        ENSURE_YAML_IEND(lvl, indt_len);
                        ENSURE_YAML_IOPEN(lvl, indt_len, 0);
                        if ( indt_len == -1 )
                        {
                            return 0;
                        }
                        return YAML_INDENT;
                    }

ISEQO               {   ENSURE_YAML_IOPEN(lvl, doc_level, 1);
                        lvl = CURRENT_LEVEL();
                        ADD_LEVEL(lvl->spaces + 1, syck_lvl_iseq);
                        return YYTOKEN[0]; 
                    }

IMAPO               {   ENSURE_YAML_IOPEN(lvl, doc_level, 1);
                        lvl = CURRENT_LEVEL();
                        ADD_LEVEL(lvl->spaces + 1, syck_lvl_imap);
                        return YYTOKEN[0]; 
                    }

CDELIMS             {   POP_LEVEL();
                        return YYTOKEN[0]; 
                    }

[:,] ENDSPC         {   if ( *YYTOKEN == ':' && lvl->status != syck_lvl_imap && lvl->status != syck_lvl_iseq )
                        {
                            lvl->status = syck_lvl_map;
                        }
                        YYPOS(1); 
                        return YYTOKEN[0]; 
                    }

[-?] ENDSPC         {   ENSURE_YAML_IOPEN(lvl, YYTOKEN - YYLINEPTR, 1);
                        FORCE_NEXT_TOKEN(YAML_IOPEN);
                        if ( *YYCURSOR == '#' || is_newline( YYCURSOR ) || is_newline( YYCURSOR - 1 ) )
                        {
                            YYCURSOR--; 
                            ADD_LEVEL((YYTOKEN + 1) - YYLINEPTR, syck_lvl_seq);
                        }
                        else /* spaces followed by content uses the space as indentation */
                        {
                            ADD_LEVEL(YYCURSOR - YYLINEPTR, syck_lvl_seq);
                        }
                        return YYTOKEN[0]; 
                    }

"&" YWORDC+         {   sycklval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );

                        /*
                         * Remove previous anchors of the same name.  Since the parser will likely
                         * construct deeper nodes first, we want those nodes to be placed in the
                         * queue for matching at a higher level of indentation.
                         */
                        syck_hdlr_remove_anchor(parser, sycklval->name);
                        return YAML_ANCHOR;
                    }

"*" YWORDC+         {   ENSURE_YAML_IOPEN(lvl, doc_level, 1);
                        sycklval->name = syck_strndup( YYTOKEN + 1, YYCURSOR - YYTOKEN - 1 );
                        return YAML_ALIAS;
                    }

"!"                 {   goto TransferMethod; }

"'"                 {   ENSURE_YAML_IOPEN(lvl, doc_level, 1);
                        goto SingleQuote; }

"\""                {   ENSURE_YAML_IOPEN(lvl, doc_level, 1);
                        goto DoubleQuote; }

YBLOCK              {   if ( is_newline( YYCURSOR - 1 ) ) 
                        {
                            YYCURSOR--;
                        }
                        goto ScalarBlock; 
                    }

"#"                 {   eat_comments( parser ); 
                        goto Document;
                    }

SPCTAB+             {   goto Document; }

NULL                {   ENSURE_YAML_IEND(lvl, -1);
                        YYPOS(0);
                        return 0; 
                    }

ANY                 {   ENSURE_YAML_IOPEN(lvl, doc_level, 1);
                        goto Plain; 
                    }

*/
    }

Directive:
    {
        YYTOKTMP = YYCURSOR;

/*!re2c

DIR                 {   goto Directive; }

SPCTAB+             {   goto Directive; }

ANY                 {   YYCURSOR = YYTOKTMP;
                        return YAML_DOCSEP;
                    }
*/

    }

Plain:
    {
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );
        SyckLevel *plvl;
        int parentIndent;

        YYCURSOR = YYTOKEN;
        plvl = CURRENT_LEVEL();
        GET_TRUE_YAML_INDENT(parentIndent);

Plain2: 
        YYTOKEN = YYCURSOR;

Plain3:

/*!re2c

YINDENT             {   int indt_len, nl_count = 0;
                        SyckLevel *lvl;
                        char *tok = YYTOKEN;
                        GOBBLE_UP_YAML_INDENT( indt_len, tok );
                        lvl = CURRENT_LEVEL();

                        if ( indt_len <= parentIndent )
                        {
                            RETURN_IMPLICIT();
                        }

                        while ( YYTOKEN < YYCURSOR )
                        {
                            int nl_len = newline_len( YYTOKEN++ );
                            if ( nl_len )
                            {
                                nl_count++;
                                YYTOKEN += nl_len - 1;
                            }
                        }
                        if ( nl_count <= 1 )
                        {
                            QUOTECAT(qstr, qcapa, qidx, ' ');
                        }
                        else
                        {
                            int i;
                            for ( i = 0; i < nl_count - 1; i++ )
                            {
                                QUOTECAT(qstr, qcapa, qidx, '\n');
                            }
                        }

                        goto Plain2; 
                    }

ALLX                {   RETURN_IMPLICIT(); }

ICOMMA              {   if ( plvl->status != syck_lvl_iseq && plvl->status != syck_lvl_imap )
                        {
                            PLAIN_NOT_INL();
                        }
                        else
                        {
                            PLAIN_IS_INL();
                        }
                        RETURN_IMPLICIT();
                    }

IMAPC               {   if ( plvl->status != syck_lvl_imap )
                        {
                            PLAIN_NOT_INL();
                        }
                        else
                        {
                            PLAIN_IS_INL();
                        }
                        RETURN_IMPLICIT();
                    }

ISEQC               {   if ( plvl->status != syck_lvl_iseq )
                        {
                            PLAIN_NOT_INL();
                        }
                        else
                        {
                            PLAIN_IS_INL();
                        }
                        RETURN_IMPLICIT();
                    }

" #"                {   eat_comments( parser ); 
                        RETURN_IMPLICIT();
                    }

NULL                {   RETURN_IMPLICIT(); }

SPCTAB              {   if ( qidx == 0 ) 
                        {
                            goto Plain2;
                        }
                        else
                        {
                            goto Plain3; 
                        }
                    }

ANY                 {   QUOTECATS(qstr, qcapa, qidx, YYTOKEN, YYCURSOR - YYTOKEN);
                        goto Plain2;
                    }

*/
    }

SingleQuote:
    {
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );

SingleQuote2:
        YYTOKEN = YYCURSOR;

/*!re2c

YINDENT             {   int indt_len;
                        int nl_count = 0;
                        SyckLevel *lvl;
                        GOBBLE_UP_YAML_INDENT( indt_len, YYTOKEN );
                        lvl = CURRENT_LEVEL();

                        if ( lvl->status != syck_lvl_str )
                        {
                            ADD_LEVEL( indt_len, syck_lvl_str );
                        }
                        else if ( indt_len < lvl->spaces )
                        {
                            /* Error! */
                        }

                        while ( YYTOKEN < YYCURSOR )
                        {
                            int nl_len = newline_len( YYTOKEN++ );
                            if ( nl_len )
                            {
                                nl_count++;
                                YYTOKEN += nl_len - 1;
                            }
                        }
                        if ( nl_count <= 1 )
                        {
                            QUOTECAT(qstr, qcapa, qidx, ' ');
                        }
                        else
                        {
                            int i;
                            for ( i = 0; i < nl_count - 1; i++ )
                            {
                                QUOTECAT(qstr, qcapa, qidx, '\n');
                            }
                        }

                        goto SingleQuote2; 
                    }

"''"                {   QUOTECAT(qstr, qcapa, qidx, '\'');
                        goto SingleQuote2; 
                    }

( "'" | NULL )      {   SyckLevel *lvl;
                        SyckNode *n = syck_alloc_str();
                        lvl = CURRENT_LEVEL();

                        if ( lvl->status == syck_lvl_str )
                        {
                            POP_LEVEL();
                        }
                        if ( ((SyckParser *)parser)->taguri_expansion == 1 )
                        {
                            n->type_id = syck_taguri( YAML_DOMAIN, "str", 3 );
                        }
                        else
                        {
                            n->type_id = syck_strndup( "str", 3 );
                        }
                        n->data.str->ptr = qstr;
                        n->data.str->len = qidx;
                        n->data.str->style = scalar_1quote;
                        sycklval->nodeData = n;
                        return YAML_PLAIN; 
                    }

ANY                 {   QUOTECAT(qstr, qcapa, qidx, *(YYCURSOR - 1)); 
                        goto SingleQuote2; 
                    }

*/

    }


DoubleQuote:
    {
        int keep_nl = 1;
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );

DoubleQuote2:
        YYTOKEN = YYCURSOR;


/*!re2c

YINDENT             {   int indt_len;
                        int nl_count = 0;
                        SyckLevel *lvl;
                        GOBBLE_UP_YAML_INDENT( indt_len, YYTOKEN );
                        lvl = CURRENT_LEVEL();

                        if ( lvl->status != syck_lvl_str )
                        {
                            ADD_LEVEL( indt_len, syck_lvl_str );
                        }
                        else if ( indt_len < lvl->spaces )
                        {
                            /* FIXME */
                        }

                        if ( keep_nl == 1 )
                        {
                            while ( YYTOKEN < YYCURSOR )
                            {
                                int nl_len = newline_len( YYTOKEN++ );
                                if ( nl_len )
                                {
                                    nl_count++;
                                    YYTOKEN += nl_len - 1;
                                }
                            }
                            if ( nl_count <= 1 )
                            {
                                QUOTECAT(qstr, qcapa, qidx, ' ');
                            }
                            else
                            {
                                int i;
                                for ( i = 0; i < nl_count - 1; i++ )
                                {
                                    QUOTECAT(qstr, qcapa, qidx, '\n');
                                }
                            }
                        }

                        keep_nl = 1;
                        goto DoubleQuote2; 
                    }

"\\" ESCSEQ         {   char ch = *( YYCURSOR - 1 );
                        QUOTECAT(qstr, qcapa, qidx, escape_seq( ch ));
                        goto DoubleQuote2; 
                    }

"\\x" HEX HEX       {   long ch;
                        char *chr_text = syck_strndup( YYTOKEN, 4 );
                        chr_text[0] = '0';
                        ch = strtol( chr_text, NULL, 16 );
                        free( chr_text );
                        QUOTECAT(qstr, qcapa, qidx, ch);
                        goto DoubleQuote2; 
                    }

"\\" SPC* LF        {   keep_nl = 0;
                        YYCURSOR--;
                        goto DoubleQuote2; 
                    }

( "\"" | NULL )     {   SyckLevel *lvl;
                        SyckNode *n = syck_alloc_str();
                        lvl = CURRENT_LEVEL();

                        if ( lvl->status == syck_lvl_str )
                        {
                            POP_LEVEL();
                        }
                        if ( ((SyckParser *)parser)->taguri_expansion == 1 )
                        {
                            n->type_id = syck_taguri( YAML_DOMAIN, "str", 3 );
                        }
                        else
                        {
                            n->type_id = syck_strndup( "str", 3 );
                        }
                        n->data.str->ptr = qstr;
                        n->data.str->len = qidx;
                        n->data.str->style = scalar_2quote;
                        sycklval->nodeData = n;
                        return YAML_PLAIN; 
                    }

ANY                 {   QUOTECAT(qstr, qcapa, qidx, *(YYCURSOR - 1)); 
                        goto DoubleQuote2; 
                    }

*/
    }

TransferMethod:
    {
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );

TransferMethod2:
        YYTOKTMP = YYCURSOR;

/*!re2c

( ENDSPC | NULL )   {   SyckLevel *lvl;
                        YYCURSOR = YYTOKTMP;
                        if ( YYCURSOR == YYTOKEN + 1 )
                        {
                            free( qstr );
                            return YAML_ITRANSFER;
                        }

                        lvl = CURRENT_LEVEL();

                        /*
                         * URL Prefixing
                         */
                        if ( *qstr == '^' )
                        {
                            sycklval->name = S_ALLOC_N( char, qidx + strlen( lvl->domain ) );
                            sycklval->name[0] = '\0';
                            strcat( sycklval->name, lvl->domain );
                            strncat( sycklval->name, qstr + 1, qidx - 1 );
                            free( qstr );
                        }
                        else
                        {
                            char *carat = qstr;
                            char *qend = qstr + qidx;
                            while ( (++carat) < qend )
                            {
                                if ( *carat == '^' )
                                    break;
                            }

                            if ( carat < qend )
                            {
                                free( lvl->domain );
                                lvl->domain = syck_strndup( qstr, carat - qstr );
                                sycklval->name = S_ALLOC_N( char, ( qend - carat ) + strlen( lvl->domain ) );
                                sycklval->name[0] = '\0';
                                strcat( sycklval->name, lvl->domain );
                                strncat( sycklval->name, carat + 1, ( qend - carat ) - 1 );
                                free( qstr );
                            }
                            else
                            {
                                sycklval->name = qstr;
                            }
                        }

                        return YAML_TRANSFER; 
                    }

/*
 * URL Escapes
 */
"\\" ESCSEQ          {  char ch = *( YYCURSOR - 1 );
                        QUOTECAT(qstr, qcapa, qidx, escape_seq( ch ));
                        goto TransferMethod2;
                    }

"\\x" HEX HEX       {   long ch;
                        char *chr_text = syck_strndup( YYTOKTMP, 4 );
                        chr_text[0] = '0';
                        ch = strtol( chr_text, NULL, 16 );
                        free( chr_text );
                        QUOTECAT(qstr, qcapa, qidx, ch);
                        goto TransferMethod2;
                    }

ANY                 {   QUOTECAT(qstr, qcapa, qidx, *(YYCURSOR - 1)); 
                        goto TransferMethod2;
                    }


*/
    }

ScalarBlock:
    {
        int qidx = 0;
        int qcapa = 100;
        char *qstr = S_ALLOC_N( char, qcapa );
        int blockType = 0;
        int nlDoWhat = 0;
        int lastIndent = 0;
        int forceIndent = -1;
        char *yyt = YYTOKEN;
        SyckLevel *lvl = CURRENT_LEVEL();
        int parentIndent = -1;

        switch ( *yyt )
        {
            case '|': blockType = BLOCK_LIT; break;
            case '>': blockType = BLOCK_FOLD; break;
        }

        while ( ++yyt <= YYCURSOR )
        {
            if ( *yyt == '-' )
            {
                nlDoWhat = NL_CHOMP;
            }
            else if ( *yyt == '+' )
            {
                nlDoWhat = NL_KEEP;
            }
            else if ( isdigit( *yyt ) )
            {
                forceIndent = strtol( yyt, NULL, 10 );
            }
        }

        qstr[0] = '\0';
        YYTOKEN = YYCURSOR;

ScalarBlock2:
        YYTOKEN = YYCURSOR;

/*!re2c

YINDENT             {   char *pacer;
                        char *tok = YYTOKEN;
                        int indt_len = 0, nl_count = 0, fold_nl = 0, nl_begin = 0;
                        GOBBLE_UP_YAML_INDENT( indt_len, tok );
                        lvl = CURRENT_LEVEL();

                        if ( lvl->status != syck_lvl_block )
                        {
                            GET_TRUE_YAML_INDENT(parentIndent);
                            if ( forceIndent > 0 ) forceIndent += parentIndent;
                            if ( indt_len > parentIndent )
                            {
                                int new_spaces = forceIndent > 0 ? forceIndent : indt_len;
                                ADD_LEVEL( new_spaces, syck_lvl_block );
                                lastIndent = indt_len - new_spaces;
                                nl_begin = 1;
                                lvl = CURRENT_LEVEL();
                            }
                            else
                            {
                                YYCURSOR = YYTOKEN;
                                RETURN_YAML_BLOCK();
                            }
                        }

                        /*
                         * Fold only in the event of two lines being on the leftmost
                         * indentation.
                         */
                        if ( blockType == BLOCK_FOLD && lastIndent == 0 && ( indt_len - lvl->spaces ) == 0 )
                        {
                            fold_nl = 1;
                        }

                        pacer = YYTOKEN;
                        while ( pacer < YYCURSOR )
                        {
                            int nl_len = newline_len( pacer++ );
                            if ( nl_len )
                            {
                                nl_count++;
                                pacer += nl_len - 1;
                            }
                        }

                        if ( fold_nl == 1 || nl_begin == 1 )
                        {
                            nl_count--;
                        }

                        if ( nl_count < 1 && nl_begin == 0 )
                        {
                            QUOTECAT(qstr, qcapa, qidx, ' ');
                        }
                        else
                        {
                            int i;
                            for ( i = 0; i < nl_count; i++ )
                            {
                                QUOTECAT(qstr, qcapa, qidx, '\n');
                            }
                        }

                        lastIndent = indt_len - lvl->spaces;
                        YYCURSOR -= lastIndent;

                        if ( indt_len < lvl->spaces )
                        {
                            POP_LEVEL();
                            YYCURSOR = YYTOKEN;
                            RETURN_YAML_BLOCK();
                        }
                        goto ScalarBlock2;
                    }


"#"                 {   lvl = CURRENT_LEVEL();
                        if ( lvl->status != syck_lvl_block )
                        {
                            eat_comments( parser );
                            YYTOKEN = YYCURSOR;
                        }
                        else
                        {
                            QUOTECAT(qstr, qcapa, qidx, *YYTOKEN);
                        }
                        goto ScalarBlock2;
                    }
              

NULL                {   YYCURSOR--;
                        POP_LEVEL();
                        RETURN_YAML_BLOCK(); 
                    }

"---" ENDSPC        {   if ( YYTOKEN == YYLINEPTR )
                        {
                            if ( blockType == BLOCK_FOLD && qidx > 0 )
                            {
                                qidx -= 1;
                            }
                            QUOTECAT(qstr, qcapa, qidx, '\n');
                            POP_LEVEL();
                            YYCURSOR = YYTOKEN;
                            RETURN_YAML_BLOCK();
                        }
                        else
                        {
                            QUOTECAT(qstr, qcapa, qidx, *YYTOKEN);
                            YYCURSOR = YYTOKEN + 1;
                            goto ScalarBlock2;
                        }
                    }

ANY                 {   QUOTECAT(qstr, qcapa, qidx, *YYTOKEN);
                        goto ScalarBlock2;
                    }


*/
    }

    return 0;

}

void
eat_comments( SyckParser *parser )
{
Comment:
    {
        YYTOKEN = YYCURSOR;

/*!re2c

( LF+ | NULL )      {   YYCURSOR = YYTOKEN;
                        return;
                    }

ANY                 {   goto Comment; 
                    }

*/

    }

}

char
escape_seq( char ch )
{
    switch ( ch )
    {
        case '0': return '\0';
        case 'a': return 7;
        case 'b': return '\010';
        case 'e': return '\033';
        case 'f': return '\014';
        case 'n': return '\n';
        case 'r': return '\015';
        case 't': return '\t';
        case 'v': return '\013';
        default: return ch;
    }
}

int
is_newline( char *ptr )
{
    return newline_len( ptr );
}

int
newline_len( char *ptr )
{
    if ( *ptr == '\n' )
        return 1;
    
    if ( *ptr == '\r' && *( ptr + 1 ) == '\n' )
        return 2;

    return 0;
}

int 
syckwrap()
{
    return 1;
}

void 
syckerror( const char *msg )
{
    if ( syck_parser_ptr->error_handler == NULL )
        syck_parser_ptr->error_handler = syck_default_error_handler;

    syck_parser_ptr->root = syck_parser_ptr->root_on_error;
    (syck_parser_ptr->error_handler)(syck_parser_ptr, msg);
}


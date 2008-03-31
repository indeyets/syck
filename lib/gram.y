/*
 * gram.y
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 */

%start doc
%pure-parser


%{

#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#ifndef YYSTACK_USE_ALLOCA
#define YYSTACK_USE_ALLOCA 0
#endif

#include "syck.h"
#include "sycklex.h"

void apply_seq_in_map( SyckParser *parser, SyckNode *n );

#define YYPARSE_PARAM   parser
#define YYLEX_PARAM     parser

#define NULL_NODE(parser, node) \
        SyckNode *node = syck_new_str( "", scalar_plain ); \
        if ( ((SyckParser *)parser)->taguri_expansion == 1 ) \
        { \
            node->type_id = syck_taguri( YAML_DOMAIN, "null", 4 ); \
        } \
        else \
        { \
            node->type_id = syck_strndup( "null", 4 ); \
        }
%}

%union {
    SYMID nodeId;
    SyckNode *nodeData;
    char *name;
};

%token <name>       YAML_ANCHOR YAML_ALIAS YAML_TRANSFER YAML_TAGURI YAML_ITRANSFER
%token <nodeData>   YAML_WORD YAML_PLAIN YAML_BLOCK
%token              YAML_DOCSEP YAML_IOPEN YAML_INDENT YAML_IEND

%type <nodeId>      doc basic_seq
%type <nodeData>    atom word_rep ind_rep struct_rep atom_or_empty empty
%type <nodeData>    implicit_seq inline_seq implicit_map inline_map inline_seq_atom inline_map_atom
%type <nodeData>    top_imp_seq in_implicit_seq in_inline_seq basic_mapping complex_key complex_value
%type <nodeData>    top_imp_map in_implicit_map in_inline_map complex_mapping

%left               '-' ':'
%left               '[' ']' '{' '}' ',' '?'

%%

doc     : atom
        {
           ((SyckParser *)parser)->root = syck_hdlr_add_node( (SyckParser *)parser, $1 );
        }
        | YAML_DOCSEP atom_or_empty
        {
           ((SyckParser *)parser)->root = syck_hdlr_add_node( (SyckParser *)parser, $2 );
        }
        |
        {
           ((SyckParser *)parser)->eof = 1;
        }
        ;

atom	: word_rep
        | ind_rep
        ;

ind_rep : struct_rep
        | YAML_TRANSFER ind_rep
        { 
            syck_add_transfer( $1, $2, ((SyckParser *)parser)->taguri_expansion );
            $$ = $2;
        }
        | YAML_TAGURI ind_rep
        {
            syck_add_transfer( $1, $2, 0 );
            $$ = $2;
        }
        | YAML_ANCHOR ind_rep
        { 
           /*
            * _Anchors_: The language binding must keep a separate symbol table
            * for anchors.  The actual ID in the symbol table is returned to the
            * higher nodes, though.
            */
           $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $2 );
        }
        | indent_open ind_rep indent_flex_end
        {
           $$ = $2;
        }
        ;

atom_or_empty   : atom
                | empty
                ;

empty           : indent_open empty indent_end
                {
                    $$ = $2;
                }
                |
                {
                    NULL_NODE( parser, n );
                    $$ = n;
                }
                | YAML_ITRANSFER empty
                { 
                   if ( ((SyckParser *)parser)->implicit_typing == 1 )
                   {
                      try_tag_implicit( $2, ((SyckParser *)parser)->taguri_expansion );
                   }
                   $$ = $2;
                }
                | YAML_TRANSFER empty
                { 
                    syck_add_transfer( $1, $2, ((SyckParser *)parser)->taguri_expansion );
                    $$ = $2;
                }
                | YAML_TAGURI empty
                {
                    syck_add_transfer( $1, $2, 0 );
                    $$ = $2;
                }
                | YAML_ANCHOR empty
                { 
                   /*
                    * _Anchors_: The language binding must keep a separate symbol table
                    * for anchors.  The actual ID in the symbol table is returned to the
                    * higher nodes, though.
                    */
                   $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $2 );
                }
                ;

/*
 * Indentation abstractions
 */
indent_open     : YAML_IOPEN
                | indent_open YAML_INDENT
                ;
                
indent_end      : YAML_IEND
                ;

indent_sep      : YAML_INDENT
                ;

indent_flex_end : YAML_IEND
                | indent_sep indent_flex_end
                ;

/*
 * Words are broken out to distinguish them
 * as keys in implicit maps and valid elements
 * for the inline structures
 */
word_rep	: YAML_TRANSFER word_rep						
            { 
               syck_add_transfer( $1, $2, ((SyckParser *)parser)->taguri_expansion );
               $$ = $2;
            } 
            | YAML_TAGURI word_rep
            { 
               syck_add_transfer( $1, $2, 0 );
               $$ = $2;
            } 
            | YAML_ITRANSFER word_rep						
            { 
               if ( ((SyckParser *)parser)->implicit_typing == 1 )
               {
                  try_tag_implicit( $2, ((SyckParser *)parser)->taguri_expansion );
               }
               $$ = $2;
            }
            | YAML_ANCHOR word_rep
            { 
               $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $2 );
            }
            | YAML_ALIAS										
            {
               /*
                * _Aliases_: The anchor symbol table is scanned for the anchor name.
                * The anchor's ID in the language's symbol table is returned.
                */
               $$ = syck_hdlr_get_anchor( (SyckParser *)parser, $1 );
            }
			| YAML_WORD
            { 
               SyckNode *n = $1;
               if ( ((SyckParser *)parser)->taguri_expansion == 1 )
               {
                   n->type_id = syck_taguri( YAML_DOMAIN, "str", 3 );
               }
               else
               {
                   n->type_id = syck_strndup( "str", 3 );
               }
               $$ = n;
            }
            | YAML_PLAIN
            | indent_open word_rep indent_flex_end
            {
               $$ = $2;
            }
            ;

/*
 * Any of these structures can be used as
 * complex keys
 */
struct_rep	: YAML_BLOCK
			| implicit_seq
			| inline_seq
			| implicit_map
			| inline_map
            ;

/*
 * Implicit sequence 
 */
implicit_seq	: indent_open top_imp_seq indent_end
                { 
                    $$ = $2;
                }
                | indent_open in_implicit_seq indent_end
                { 
                    $$ = $2;
                }
                ;

basic_seq       : '-' atom_or_empty             
                { 
                    $$ = syck_hdlr_add_node( (SyckParser *)parser, $2 );
                }
                ;

top_imp_seq     : YAML_TRANSFER indent_sep in_implicit_seq
                { 
                    syck_add_transfer( $1, $3, ((SyckParser *)parser)->taguri_expansion );
                    $$ = $3;
                }
                | YAML_TRANSFER top_imp_seq
                { 
                    syck_add_transfer( $1, $2, ((SyckParser *)parser)->taguri_expansion );
                    $$ = $2;
                }
                | YAML_TAGURI indent_sep in_implicit_seq
                { 
                    syck_add_transfer( $1, $3, 0 );
                    $$ = $3;
                }
                | YAML_TAGURI top_imp_seq
                { 
                    syck_add_transfer( $1, $2, 0 );
                    $$ = $2;
                }
                | YAML_ANCHOR indent_sep in_implicit_seq
                { 
                    $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $3 );
                }
                | YAML_ANCHOR top_imp_seq
                { 
                    $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $2 );
                }
                ;

in_implicit_seq : basic_seq
                {
                    $$ = syck_new_seq( $1 );
                }
				| in_implicit_seq indent_sep basic_seq
				{ 
                    syck_seq_add( $1, $3 );
                    $$ = $1;
				}
				| in_implicit_seq indent_sep
				{ 
                    $$ = $1;
				}
                ;

/*
 * Inline sequences
 */
inline_seq		: '[' in_inline_seq ']'
                { 
                    $$ = $2;
                }
				| '[' ']'
                { 
                    $$ = syck_alloc_seq();
                }
                ;

in_inline_seq   : inline_seq_atom
                {
                    $$ = syck_new_seq( syck_hdlr_add_node( (SyckParser *)parser, $1 ) );
                }
                | in_inline_seq ',' inline_seq_atom
				{ 
                    syck_seq_add( $1, syck_hdlr_add_node( (SyckParser *)parser, $3 ) );
                    $$ = $1;
				}
                ;

inline_seq_atom : atom
                | basic_mapping
                ;

/*
 * Implicit maps
 */
implicit_map	: indent_open top_imp_map indent_end
                { 
                    apply_seq_in_map( (SyckParser *)parser, $2 );
                    $$ = $2;
                }
                | indent_open in_implicit_map indent_end
                { 
                    apply_seq_in_map( (SyckParser *)parser, $2 );
                    $$ = $2;
                }
                ;

top_imp_map     : YAML_TRANSFER indent_sep in_implicit_map
                { 
                    syck_add_transfer( $1, $3, ((SyckParser *)parser)->taguri_expansion );
                    $$ = $3;
                }
                | YAML_TRANSFER top_imp_map
                { 
                    syck_add_transfer( $1, $2, ((SyckParser *)parser)->taguri_expansion );
                    $$ = $2;
                }
                | YAML_TAGURI indent_sep in_implicit_map
                { 
                    syck_add_transfer( $1, $3, 0 );
                    $$ = $3;
                }
                | YAML_TAGURI top_imp_map
                { 
                    syck_add_transfer( $1, $2, 0 );
                    $$ = $2;
                }
                | YAML_ANCHOR indent_sep in_implicit_map
                { 
                    $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $3 );
                }
                | YAML_ANCHOR top_imp_map
                { 
                    $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $2 );
                }
                ;

complex_key     : word_rep
                | '?' atom indent_sep
                {
                    $$ = $2;
                }
                ;

complex_value   : atom_or_empty
                ;

/* Default needs to be added to SyckSeq i think...
				| '=' ':' atom
				{
					result = [ :DEFAULT, val[2] ]
				}
*/

complex_mapping : complex_key ':' complex_value
                {
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $1 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, $3 ) );
                }
/*
				| '?' atom
                {
                    NULL_NODE( parser, n );
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $2 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, n ) );
                }
*/
                ;

in_implicit_map : complex_mapping
				| in_implicit_map indent_sep basic_seq
                {
                    if ( $1->shortcut == NULL )
                    {
                        $1->shortcut = syck_new_seq( $3 );
                    }
                    else
                    {
                        syck_seq_add( $1->shortcut, $3 );
                    }
                    $$ = $1;
                }
				| in_implicit_map indent_sep complex_mapping
                {
                    apply_seq_in_map( (SyckParser *)parser, $1 );
                    syck_map_update( $1, $3 );
                    syck_free_node( $3 );
                    $3 = NULL;
                    $$ = $1;
                }
				| in_implicit_map indent_sep
                {
                    $$ = $1;
                }
                ;

/*
 * Inline maps
 */
basic_mapping	: atom ':' atom_or_empty
                {
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $1 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, $3 ) );
                }
                ;

inline_map		: '{' in_inline_map '}'
                {
                    $$ = $2;
                }
          		| '{' '}'
                {
                    $$ = syck_alloc_map();
                }
                ;
         
in_inline_map	: inline_map_atom
				| in_inline_map ',' inline_map_atom
				{
                    syck_map_update( $1, $3 );
                    syck_free_node( $3 );
                    $3 = NULL;
                    $$ = $1;
				}
                ;

inline_map_atom : atom
                {
                    NULL_NODE( parser, n );
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $1 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, n ) );
                }
                | basic_mapping
                ;

%%

void
apply_seq_in_map( SyckParser *parser, SyckNode *n )
{
    long map_len;
    if ( n->shortcut == NULL )
    {
        return;
    }

    map_len = syck_map_count( n );
    syck_map_assign( n, map_value, map_len - 1,
        syck_hdlr_add_node( parser, n->shortcut ) );

    n->shortcut = NULL;
}


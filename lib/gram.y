//
// gram.y
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

%start doc

%{

#include "syck.h"

#define YYPARSE_PARAM parser

%}

%union {
    SYMID nodeId;
    SyckNode *nodeData;
    char *name;
};

%token <name>       ANCHOR ALIAS TRANSFER FOLD
%token <nodeData>   WORD PLAIN FSTART RAWTEXT
%token              IOPEN INDENT IEND

%type <nodeId>      doc basic_seq
%type <nodeData>    atom word_rep struct_rep atom_or_empty
%type <nodeData>    scalar_block implicit_seq inline_seq implicit_map inline_map
%type <nodeData>    in_implicit_seq in_inline_seq basic_mapping
%type <nodeData>    in_implicit_map in_inline_map complex_mapping

%left               '+' '-' '[' ']' '{' '}' ':' ',' '?'

%%

doc     : atom
        {
           $$ = syck_hdlr_add_node( (SyckParser *)parser, $1 );
        }

atom	: word_rep
	 	| struct_rep
	 	| ANCHOR atom								
        { 
           /*
            * _Anchors_: The language binding must keep a separate symbol table
            * for anchors.  The actual ID in the symbol table is returned to the
            * higher nodes, though.
            */
           $$ = syck_hdlr_add_anchor( (SyckParser *)parser, $1, $2 );
        }
		| ALIAS										
        {
           /*
            * _Aliases_: The anchor symbol table is scanned for the anchor name.
            * The anchor's ID in the language's symbol table is returned.
            */
           $$ = syck_hdlr_add_alias( (SyckParser *)parser, $1 );
        }

atom_or_empty   : atom
                |
                {
                   $$ = syck_new_str( "" ); 
                }

//
// Words are broken out to distinguish them
// as keys in implicit maps and valid elements
// for the inline structures
//
word_rep	: TRANSFER word_rep						
            { 
               $$ = syck_add_transfer( $1, $2 );
            }
			| WORD
            { 
               SyckNode *n = $1;
               n->type_id = "str";
               $$ = n;
            }
            | PLAIN
            {
                $$ = $1;
            }
			| '+'									
            { 
                $$ = syck_new_str( "+" );
            }
			| '-'
            { 
                $$ = syck_new_str( "-" );
            }

//
// Any of these structures can be used as
// complex keys
//
struct_rep	: TRANSFER struct_rep
            { 
                $$ = syck_add_transfer( $1, $2 );
            }
			| scalar_block
			| implicit_seq
			| inline_seq
			| implicit_map
			| inline_map

//
// Folding and indentation is handled in the tokenizer.
// We handle chomping here, though.
//
scalar_block	: FOLD IOPEN RAWTEXT IEND			
			 	{ 
                    SyckNode *n = $3;
                    syck_fold_format( $1, $3 );
                    n->type_id = "str";
                    $$ = n;
				}
			    | FOLD FSTART
                { 
                    SyckNode *n = $2;
                    n->type_id = "str";
                    $$ = n;
                }

//
// Implicit sequence 
//
implicit_seq	: IOPEN in_implicit_seq	IEND	
                { 
                    $$ = $2;
                }

basic_seq       : '-' atom_or_empty             
                { 
                    $$ = syck_hdlr_add_node( (SyckParser *)parser, $2 );
                }
/* Still need to rethink this...
				| '-' seq_map_short
                { 
                    $$ = syck_hdlr_add_node( (SyckParser *)parser, $2 );
                }
*/

in_implicit_seq : basic_seq
                {
                    $$ = syck_new_seq( $1 );
                }
				| in_implicit_seq INDENT basic_seq
				{ 
                    syck_seq_add( $1, $3 );
                    $$ = $1;
				}

//
// The sequence-mapping shortcut
//
/* This needs to be rethought based on the symbol table
seq_map_short	: simple_mapping
			  	{
                    $$ = syck_hdlr_add_node( (SyckParser *)parser, $1 );
				}
				| simple_mapping implicit_map	
                { 
                    syck_map_update( $1, $2 );
                    $$ = syck_hdlr_add_node( (SyckParser *)parser, $1 );
                }
*/

//
// Inline sequences
//
inline_seq		: '[' in_inline_seq ']'
                { 
                    $$ = $2;
                }
				| '[' ']'
                { 
                    $$ = syck_alloc_seq();
                }

in_inline_seq   : atom
                {
                    $$ = syck_new_seq( syck_hdlr_add_node( (SyckParser *)parser, $1 ) );
                }
                | in_inline_seq ',' atom
				{ 
                    syck_seq_add( $1, syck_hdlr_add_node( (SyckParser *)parser, $3 ) );
                    $$ = $1;
				}

//
// Implicit maps
//
implicit_map	: IOPEN in_implicit_map IEND
                { 
                    $$ = $2;
                }

/* Default needs to be added to SyckSeq i think...
simple_mapping	: word_rep ':' atom
                {
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $1 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, $3 ) );
                }
				| '=' ':' atom
				{
					result = [ :DEFAULT, val[2] ]
				}
*/

basic_mapping	: word_rep ':' atom_or_empty
                {
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $1 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, $3 ) );
                }
/* Default needs to be added to SyckSeq i think...
				| '=' ':' atom
				{
					result = [ :DEFAULT, val[2] ]
				}
*/

complex_mapping : basic_mapping
				| '?' atom INDENT ':' atom_or_empty
                {
                    $$ = syck_new_map( 
                        syck_hdlr_add_node( (SyckParser *)parser, $2 ), 
                        syck_hdlr_add_node( (SyckParser *)parser, $5 ) );
                }

in_implicit_map : complex_mapping
				{
                    $$ = $1;
				}
				| in_implicit_map INDENT complex_mapping
                { 
                    syck_map_update( $1, $3 );
                    $$ = $1;
                }

//
// Inline maps
//
inline_map		: '{' in_inline_map '}'
                {
                    $$ = $2;
                }
          		| '{' '}'
                {
                    $$ = syck_alloc_map();
                }
         
in_inline_map	: basic_mapping 
				{
					$$ = $1;
				}
				| in_inline_map ',' basic_mapping
				{
                    syck_map_update( $1, $3 );
                    $$ = $1;
				}

%%


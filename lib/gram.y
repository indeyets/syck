//
// gram.y
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

%{

#include "syck.h"

%}

%union {
    SYMID nodeId;
    struct SyckNode *nodeData;
    char *name;
};

%token <name>       ANCHOR ALIAS TRANSFER FOLD
%token <nodeData>   WORD PLAIN FSTART RAWTEXT
%token              "+" "-" IOPEN INDENT IEND

%type <nodeId>      atom word_rep struct_rep atom_or_empty
%type <nodeId>      scalar_block implicit_seq inline_seq implicit_map inline_map
%type <nodeData>    in_implicit_seq basic_seq

%%

atom	: word_rep
	 	| struct_rep
	 	| ANCHOR atom								
        { 
           /*
            * _Anchors_: The language binding must keep a separate symbol table
            * for anchors.  The actual ID in the symbol table is returned to the
            * higher nodes, though.
            */
           $$ = hdlr_add_anchor( $1, $2 );
        }
		| ALIAS										
        {
           /*
            * _Aliases_: The anchor symbol table is scanned for the anchor name.
            * The anchor's ID in the language's symbol table is returned.
            */
           $$ = hdlr_add_alias( $1 );
        }

atom_or_empty   : atom
                |
                {
                    $$ = hdlr_add_node( new_str_node( "" ) ); 
                }

//
// Words are broken out to distinguish them
// as keys in implicit maps and valid elements
// for the inline structures
//
word_rep	: TRANSFER word_rep						
            { 
                $$ = hdlr_add_transfer( $1, $2 );
            }
			| WORD
            { 
                struct SyckNode *n = $1;
                n->type_id = "str";
                $$ = hdlr_add_node( n );
            }
            | PLAIN
            {
                struct SyckNode *n = $1;
                syck_try_implicit( n );
                $$ = hdlr_add_node( n );
            }
			| "+"									
            { 
                $$ = hdlr_add_node( add_str_implicit( "+" ) );
            }
			| "-"
            { 
                $$ = hdlr_add_node( add_str_implicit( "+" ) );
            }

//
// Any of these structures can be used as
// complex keys
//
struct_rep	: TRANSFER struct_rep
            { 
                $$ = hdlr_add_transfer( $1, $2 );
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
                    struct SyckNode *n = $3;
                    syck_fold_format( $1, $3 );
                    n->type_id = "str";
                    $$ = hdlr_add_node( n );
				}
			    | FOLD FSTART
                { 
                    struct SyckNode *n = $2;
                    n->type_id = "str";
                    $$ = hdlr_add_node( n );
                }

//
// Implicit sequence 
//
implicit_seq	: IOPEN in_implicit_seq	IEND	
                { 
                    struct SyckNode *n = $2;
                    n->type_id = "seq";
                    $$ = hdlr_add_node( n );
                }

basic_seq       : '-' atom_or_empty             
                { 
                    $$ = $2
                }
				| '-' seq_map_short
                { 
                    $$ = $2
                }

in_implicit_seq : basic_seq
                {
                    $$ = new_seq_node( $1 );
                }
				| in_implicit_seq INDENT basic_seq
				{ 
                    add_seq_item( $1, $3 );
                    $$ = $1;
				}

//
// The sequence-mapping shortcut
//
seq_map_short	: simple_mapping
			  	{
                    struct SyckNode *n = $1;
                    n->type_id = "map";
                    $$ = hdlr_add_node( n );
				}
				| simple_mapping implicit_map	
                { 
                    result = hash_update( val[1], val[0] ) 
                }

//
// Inline sequences
//
inline_seq		: '[' in_inline_seq ']'  			{ result = new_node( 'seq', val[1] ) }
				| '[' ']'        					{ result = new_node( 'seq', [] ) }

in_inline_seq   : atom           					{ result = val }
                | in_inline_seq ',' atom 			{ result.push val[2] }

//
// Implicit maps
//
implicit_map	: IOPEN in_implicit_map IEND		{ result = val[1] }

simple_mapping	: word_rep ':' atom
				{ 
                    if val[0] == '<='
                        result = [ :MERGE, val[2] ]
                    else
                        result = { val[0] => val[2] }
                    end
				}
				| '=' ':' atom
				{
					result = [ :DEFAULT, val[2] ]
				}

basic_mapping	: word_rep ':' atom_or_empty
				{ 
                    if val[0] == '<='
                        result = [ :MERGE, val[2] ]
                    else
                        result = { val[0] => val[2] }
                    end
				}
				| '=' ':' atom_or_empty
				{
					result = [ :DEFAULT, val[2] ]
				}

complex_mapping : basic_mapping
				| '?' atom INDENT ':' atom_or_empty
				{
					result = { val[1] => val[4] }
				}

in_implicit_map : complex_mapping
				{
                    result = new_node( 'map', hash_update( {}, val[0] ) )
				}
				| in_implicit_map INDENT complex_mapping
				{
					result = hash_update( val[0], val[2] )
				}

//
// Inline maps
//
inline_map		: '{' in_inline_map '}'   			{ result = new_node( 'map', val[1] ) }
          		| '{' '}'							{ result = new_node( 'map', Hash.new ) }
         
in_inline_map	: basic_mapping 
				{
					result = hash_update( {}, val[0] )
				}
				| in_inline_map ',' basic_mapping
				{
					result = hash_update( val[0], val[2] )
				}

end

%%

                    /* goes in syck_fold_format
					val[2].chomp! if val[0].include?( '|' )
					result = if val[0].include?( '+' )
						val[2] + "\n"
					else
						val[2].chomp!( '' )
						if val[0].include?( '-' )
			 				val[2].chomp
						else
							val[2] + "\n" 
						end
					end
                    */

//
// gram.y
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

%%

atom	: word_rep
	 	| struct_rep
	 	| ANCHOR atom								
        { 
            if @options[:Model] == :Generic
                val[1].anchor = val[0]   
            end
            result = @anchor[ val[0] ] = val[1] 
        }
		| ALIAS										{ result = @anchor[ val[0] ] }

atom_or_empty   : atom
                | { result = '' }

//
// Words are broken out to distinguish them
// as keys in implicit maps and valid elements
// for the inline structures
//
word_rep	: TRANSFER word_rep						{ result = attach_transfer( val[0], val[1] ) }
			| WORD                                  { result = new_node( 'str', val[0] ) }
            | PLAIN
            {
                if @options[:Model] == :Generic
                    result = new_node( YAML.detect_implicit( val[0] ), val[0] )
                else
					result = YAML.try_implicit( val[0] ) 
                end
            }
			| "+"									{ result = new_node( 'boolean', true ) }
			| "-"									{ result = new_node( 'boolean', false ) }

//
// Any of these structures can be used as
// complex keys
//
struct_rep	: TRANSFER struct_rep					{ result = attach_transfer( val[0], val[1] ) }
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
                    result = new_node( 'str', result )
				}
			    | FOLD FSTART						{ result = new_node( 'str', val[1] ) }

//
// Implicit sequence 
//
implicit_seq	: IOPEN in_implicit_seq	IEND	{ result = new_node( 'seq', val[1] ) }

basic_seq       : '-' atom_or_empty             { result = val[1] }
				| '-' seq_map_short				{ result = val[1] }

in_implicit_seq : basic_seq 					{ result = val }
				| in_implicit_seq INDENT basic_seq
				{ 
					result.push val[2]
				}

//
// The sequence-mapping shortcut
//
seq_map_short	: simple_mapping
			  	{
					result = new_node( 'map', hash_update( {}, val[0] ) )
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


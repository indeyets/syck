#
#												vim:sw=4:ts=4
# $Id$
#
# Okay Project
#
require 'yaml'

module Okay

    @@type_registry = {
        'boolean' => nil,
        'str' => nil,
        'int' => nil,
        'null' => nil,
        'float' => nil,
        'seq' => nil,
        'map' => nil
    }

	class Error < StandardError; end

    # Transfer methods for okay types
    TRANSFER_TYPES = {}
    OKAY_TYPE_DOMAIN = 'okay.yaml.org,2002'
    OKAY_TYPE_REGEXP = /^tag:#{ Regexp::quote( OKAY_TYPE_DOMAIN ) }:([^;]+)((?:;\w+)*)$/

    #
    # Base class for Okay types
    # All types should inherit this class to use modules
    #
    class ModuleBase
        attr_accessor :modules
    end

	#
	# Quick Okay type handler, handles modules
	#
	def Okay.add_type( type_name, &transfer_proc )
		type_re = /^(#{Regexp::quote( type_name )})((?:;\w+)*)$/
		Okay::TRANSFER_TYPES[ type_name ] = transfer_proc
        YAML.add_domain_type( OKAY_TYPE_DOMAIN, type_re ) { |type, val|
            type, mods = OKAY_TYPE_REGEXP.match( type ).to_a[1,2]
            unless mods.to_s.empty?
                mod_re = /^(#{ mods.slice( 1..-1 ).gsub( /;/, '|' ) });/
                modules = {}
                val.reject! { |k, v|
                    if k =~ mod_re
                        modules[ $1 ] ||= {}
                        modules[ $1 ][ $' ] = v
                        true
                    else
                        false
                    end
                }
            end
		    Okay::TRANSFER_TYPES[ type ].call( type, val, modules )
        }
	end

    def Okay.object_maker( obj_class, val, modules )
        obj = YAML.object_maker( obj_class, val )
        unless obj.respond_to?( :modules= )
            raise Okay::Error, "Modules #{modules.keys.join( ',' )} can't be handled by class #{obj_class}" if modules
        end
        obj.modules = modules
        obj
    end

    def Okay.load_schema( schema )
        schema = YAML::load( schema )
        schema.each { |k,v|
            @@type_registry[k] = v['schema']
        }
    end

    def Okay.schema( type_name )
        @@type_registry[type_name]
    end

    def Okay.validate_node( node )
        type, mods = OKAY_TYPE_REGEXP.match( node.type_id ).to_a[1,2]
        unless @@type_registry.has_key?( type )
            raise Okay::Error, "Type `#{type}' not found in loaded schema." 
        end
        return true if @@type_registry[ type ].nil?
        node_vs_schema( @@type_registry[ type ], node, [], true )
    end

    def Okay.node_vs_schema( schema, node, depth, head )
        #
        # Head type can be matched against a core type
        # 
        type_id = head ? node.kind : node.type_id
        type_id = node.kind if type_id.empty?
        if type_id =~ /^okay\//
            type_id, mods = OKAY_TYPE_REGEXP.match( type_id ).to_a[1,2]
        end
        if schema.has_key?( type_id )
            attr = schema[ type_id ]
        else
            raise Okay::Error, "Node of type !#{type_id} invalid at /" + depth.join( "/" )
        end

        if @@type_registry[ type_id ]
            node_vs_schema( @@type_registry[ type_id ], node, depth, true )
        end

        #
        # Descend and match types of nodes
        #
        if attr
            attr.each { |prop, prop_schema|
                #
                # Mini-paths
                #
                if prop =~ /^\//
                    key = $'
                    if key == "*"
                        node.children_with_index.each { |c|
                            node_vs_schema( prop_schema, c[0], depth + [c[1]], false )
                        }
                    else
                        if node[key]
                            child = node[key]
                            node_vs_schema( prop_schema, child, depth + [key], false )
                        else
                            unless Array === attr['optional'] and attr['optional'].include?( "/" + key )
                                raise Okay::Error, "No key '#{key}' found at /" + depth.join( "/" )
                            end
                        end
                    end
                end
            }
        else
        end
        return true
    end

    def Okay.make_schema_flexhash( type_root )
        type_root = YAML::transfer( 'tag:ruby.yaml.org,2002:flexhash', type_root )
        type_root.collect! { |e|
            if Hash === e[1]
                e[1].each { |k,v|
                    if k =~ /^\// and Array === v
                        e[1][k] = make_schema_flexhash( v )
                    end
                }
            end
            e
        }
        type_root
    end

    Okay.add_type( 'schema' ) { |type, val, modules|
        val.each { |k,v|
            v['schema'] = make_schema_flexhash( v['schema'] )
        }
    }

end

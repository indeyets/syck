#
# Classes required by the full core typeset
#
require 'yaml/compat'

module YAML

	#
	# Default private type
	#
	class PrivateType
		attr_accessor :type_id, :value
		def initialize( type, val )
			@type_id = type; @value = val
		end
		def to_yaml( opts = {} )
			YAML::quick_emit( self.object_id, opts ) { |out|
				out << " !!#{@type_id}"
				value.to_yaml( :Emitter => out )
			}
		end
	end

    #
    # Default domain type
    #
    class DomainType
		attr_accessor :domain, :type_id, :value
		def initialize( domain, type, val )
			@domain = domain; @type_id = type; @value = val
		end
        def to_yaml_type
            dom = @domain.dup
            if dom =~ /\.yaml\.org,2002$/
                dom = $`
            end
            "#{dom}/#{@type_id}"
        end
		def to_yaml( opts = {} )
			YAML::quick_emit( self.object_id, opts ) { |out|
				out << " !#{to_yaml_type} "
				value.to_yaml( :Emitter => out )
			}
		end
	end

	#
	# YAML Hash class to support comments and defaults
	#
	class SpecialHash < Object::Hash 
		attr_accessor :default
        def inspect
            self.default.to_s
        end
		def to_s
			self.default.to_s
		end
		def update( h )
			if YAML::SpecialHash === h
				@default = h.default if h.default
			end
			super( h )
		end
        def to_yaml( opts = {} )
            opts[:DefaultKey] = self.default
            super( opts )
        end
	end

    #
    # Builtin collection: !omap
    #
    class Omap < Array
        tag_as "tag:yaml.org,2002:omap"
        def yaml_initialize( tag, val )
            if Array === val
                val.each do |v|
                    if Hash === v
                        concat( v.to_a )		# Convert the map to a sequence
                    else
                        raise YAML::Error, "Invalid !omap entry: " + val.inspect
                    end
                end
            else
                raise YAML::Error, "Invalid !omap: " + val.inspect
            end
            self
        end
        def self.[]( *vals )
            o = Omap.new
            0.step( vals.length - 1, 2 ) do |i|
                o[vals[i]] = vals[i+1]
            end
            o
        end
        def []( k )
            self.assoc( k ).to_a[1]
        end
        def []=( k, *rest )
            val, set = rest.reverse
            if ( tmp = self.assoc( k ) ) and not set
                tmp[1] = val
            else
                self << [ k, val ] 
            end
            val
        end
        def has_key?( k )
            self.assoc( k ) ? true : false
        end
        def is_complex_yaml?
            true
        end
        def to_yaml( opts = {} )
            YAML::quick_emit( self.object_id, opts ) do |out|
                out.seq( taguri ) do |seq|
                    self.each do |v|
                        seq.add( Hash[ *v ] )
                    end
                end
            end
        end
    end

    #
    # Builtin collection: !pairs
    #
    class Pairs < Array
        tag_as "tag:yaml.org,2002:pairs"
        def yaml_initialize( tag, val )
            if Array === val
                val.each do |v|
                    if Hash === v
                        concat( v.to_a )		# Convert the map to a sequence
                    else
                        raise YAML::Error, "Invalid !pairs entry: " + val.inspect
                    end
                end
            else
                raise YAML::Error, "Invalid !pairs: " + val.inspect
            end
            self
        end
        def self.[]( *vals )
            p = Pairs.new
            0.step( vals.length - 1, 2 ) { |i|
                p[vals[i]] = vals[i+1]
            }
            p
        end
        def []( k )
            self.assoc( k ).to_a
        end
        def []=( k, val )
            self << [ k, val ] 
            val
        end
        def has_key?( k )
            self.assoc( k ) ? true : false
        end
        def is_complex_yaml?
            true
        end
        def to_yaml( opts = {} )
            YAML::quick_emit( self.object_id, opts ) do |out|
                out.seq( taguri ) do |seq|
                    self.each do |v|
                        seq.add( Hash[ *v ] )
                    end
                end
            end
        end
    end

    #
    # Builtin collection: !set
    #
    class Set < Hash
        tag_as "tag:yaml.org,2002:set"
    end
end

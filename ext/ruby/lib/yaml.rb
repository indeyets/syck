#												vim:sw=4:ts=4
# $Id$
#
#   YAML.rb
#
#   Loads the parser/loader and emitter/writer.
#

module YAML

    def YAML.object_maker( obj_class, val )
        if Hash === val
            name = obj_class.name
            o = ::Marshal.load( sprintf( "\004\006o:%c%s\000", name.length + 5, name ))
            val.each_pair { |k,v|
                o.instance_eval "@#{k} = v"
            }
            o
        else
            raise YAML::Error, "Invalid object explicitly tagged !ruby/Object: " + val.inspect
        end
    end

	#
	# Input methods
	#

	#
	# Load a single document from the current stream
	#
	def YAML.load( io )
		yp = YAML::Parser.new.parse( io )
	end

	#
	# Parse a single document from the current stream
	#
	def YAML.parse( io )
		yp = YAML::Parser.new( :Model => :Generic ).parse( io )
	end

	#
	# Load all documents from the current stream
	#
	def YAML.each_document( io, &doc_proc )
		yp = YAML::Parser.new.parse_documents( io, &doc_proc )
    end

	#
	# Identical to each_document
	#
	def YAML.load_documents( io, &doc_proc )
		YAML.each_document( io, &doc_proc )
    end

	#
	# Parse all documents from the current stream
	#
	def YAML.each_node( io, &doc_proc )
		yp = YAML::Parser.new( :Model => :Generic ).parse_documents( io, &doc_proc )
    end

	#
	# Parse all documents from the current stream
	#
	def YAML.parse_documents( io, &doc_proc )
		YAML.each_node( io, &doc_proc )
    end

	#
	# Load all documents from the current stream
	#
	def YAML.load_stream( io )
		yp = YAML::Parser.new
		d = nil
		yp.parse_documents( io ) { |doc|
			d = YAML::Stream.new( yp.options ) if not d
			d.add( doc ) 
		}
		return d
	end

	#
	# Racc parser will go here
	#
	# class Parser; def initialize; raise YAML::Error, "No parser available"; end; end

	#
	# Add a transfer method to a domain
	#
	def YAML.add_domain_type( domain, type_re, &transfer_proc )
		type_re = /^#{Regexp::quote( type_re )}$/ if type_re.class == String
		if not YAML::TRANSFER_DOMAINS.has_key?( domain )
			YAML::TRANSFER_DOMAINS[ domain ] = {}
		end
		YAML::TRANSFER_DOMAINS[ domain ][ type_re ] = transfer_proc
	end

	#
	# Add a transfer method for a builtin type
	#
	def YAML.add_builtin_type( type_re, &transfer_proc )
		type_re = /^#{Regexp::quote( type_re )}$/ if type_re.class == String
		YAML::TRANSFER_DOMAINS[ 'yaml.org,2002' ][ type_re ] = transfer_proc
	end

	#
	# Add a transfer method for a builtin type
	#
	def YAML.add_ruby_type( type, &transfer_proc )
		type_re = /^#{Regexp::quote( type.to_s )}(?::.+)?$/i
		YAML::TRANSFER_DOMAINS[ 'ruby.yaml.org,2002' ][ type_re ] = transfer_proc
	end

	#
	# Add a private document type
	#
	def YAML.add_private_type( type_re, &transfer_proc )
		type_re = /^#{Regexp::quote( type_re )}$/ if type_re.class == String
		YAML::PRIVATE_TYPES[ type_re ] = transfer_proc
	end

    #
    # Method to extract colon-seperated type and class, returning
    # the type and the constant of the class
    #
    def YAML.read_type_class( type, obj_class )
        type =~ /^([^:]+):(.+)/i
        if $2
            type = $1
            $2.split( "::" ).each { |c|
                obj_class = obj_class.const_get( c )
            }
        end
        return [ type, obj_class ]
    end

	#
	# Utility functions
	#

    #
    # Handle all transfer methods
    # http://www.yaml.org/spec/#syntax-trans
    #
    def YAML.transfer_method( type, val )
        domain = "yaml.org,2002"

        #
        # Figure out the domain
        #
        if type =~ /^(\w+)\/(.+)$/
            domain = "#{$1}.yaml.org,2002"; type = $2
        elsif type =~ /^(#{DNS_NAME_RE},\d{4}(?:-\d{2}){0,2})\/(.+)$/
            domain = $1; type = $2
        end

        #
        # Route the data, transfer it
        #
        begin
            r = nil
            if type == ""
                YAML.try_implicit( val )
            elsif type =~ /^!(.*)/
                type = $1
                if YAML::PRIVATE_TYPES.keys.detect { |r| type =~ r }
                    YAML::PRIVATE_TYPES[r].call( type, val )
                else
                    YAML::PrivateType.new( type, val )
                end
            else
                type = YAML.escape( type )
                if YAML::TRANSFER_DOMAINS.has_key?( domain ) and
                    YAML::TRANSFER_DOMAINS[domain].keys.detect { |r| type =~ r } 
                    YAML::TRANSFER_DOMAINS[domain][r].call( type, val )
                else
                        YAML::DomainType.new( domain, type, val )
                end
            end
        end
    end

	#
	# Try a string against all implicit YAML types
	# http://www.yaml.org/spec/#trans
	#
	def YAML.detect_implicit( str )
        r = nil
        YAML::IMPLICIT_TYPES.each { |type, regexp, tproc|
            val = tproc.call( :Implicit, str )
            return type unless Symbol === val and val == :InvalidType
        }
        return 'str'
	end

    #
    # Uses the implicit if found
    #
    def YAML.try_implicit( str )
        type = YAML.detect_implicit( str )
        YAML.transfer_method( type, str )
    end

	#
	# Make a time with the time zone
	#
	def YAML.mktime( year, mon, day, hour, min, sec, usec, zone = "Z" )
        usec = usec.to_s.to_f * 1000000
		val = Time::utc( year.to_i, mon.to_i, day.to_i, hour.to_i, min.to_i, sec.to_i, usec )
		if zone != "Z"
			hour = zone[0,3].to_i * 3600
			min = zone[3,2].to_i * 60
			ofs = (hour + min)
			val = Time.at( val.to_f - ofs )
		end
		return val
	end

	#
	# Retrieve plain scalar regexp
	#
	def YAML.plain_re( allow = '' )
		space_set = ''
		restrict_set = RESTRICTED_INDICATORS.delete( allow )
		restrict_set.split( // ).each { |ind|
			if ind == "#"
				space_set << "[^#\\s]#+|#+[^#\\s]|"
			elsif SPACE_INDICATORS.include?( ind )
				ind = Regexp::quote( ind )
				space_set << "#{ind}+[^#{ind}\\s]|"
			end
		} 
		/\A([^#{NOT_PLAIN_CHAR}#{Regexp::quote(INDICATOR_CHAR)}](?:#{space_set}[^#{NOT_PLAIN_CHAR}#{Regexp::quote(restrict_set)}])*)/
	end

end

#
# Optimize implicit type searching
#
YAML::IMPLICIT_TYPES.collect! { |type|
    r = YAML::TRANSFER_DOMAINS['yaml.org,2002'].keys.detect { |r| type =~ r }
    [ type, r, YAML::TRANSFER_DOMAINS['yaml.org,2002'][r] ]
}

#
# ryan: You know how Kernel.p is a really convenient way to dump ruby
#       structures?  The only downside is that it's not as legible as
#       YAML.
#
# _why: (listening)
#
# ryan: I know you don't want to urinate all over your users' namespaces.
#       But, on the other hand, convenience of dumping for debugging is,
#       IMO, a big YAML use case.
#
# _why: Go nuts!  Have a pony parade!
#
# ryan: Either way, I certainly will have a pony parade.
#
module Kernel
    def y( x )
        puts x.to_yaml
    end
end



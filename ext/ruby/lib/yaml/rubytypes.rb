# -*- mode: ruby; ruby-indent-level: 4; tab-width: 4 -*- vim: sw=4 ts=4
require 'date'
require 'yaml/compat'

#
# Type conversions
#

class Class
	def to_yaml( opts = {} )
		raise TypeError, "can't dump anonymous class %s" % self.class
	end
end

class Object
    tag_as "tag:ruby.yaml.org,2002:object"
    def to_yaml_properties; instance_variables.sort; end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            out.map( tag_as ) do |map|
				to_yaml_properties.each do |m|
                    map.add( m[1..-1], instance_variable_get( m ) )
                end
            end
        end
	end
end

#
# Maps: Hash#to_yaml
#
class Hash
    tag_as "tag:yaml.org,2002:map"
    tag_as "tag:ruby.yaml.org,2002:hash"
    def yaml_initialize( tag, val )
        if Array === val
            update Hash.[]( *val )		# Convert the map to a sequence
        elsif Hash === val
            update val
        else
            raise YAML::Error, "Invalid map explicitly tagged #{ tag }: " + val.inspect
        end
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            hash_type = 
                if self.class == Hash or self.class == YAML::SpecialHash
                    "tag:yaml.org,2002:map"
                else
                    tag_as
                end
            out.map( hash_type ) do |map|
                each do |k, v|
                    map.add( k, v )
                end
            end
        end
	end
end

#
# Structs: export as a !map
#
class Struct
    tag_as "tag:ruby.yaml.org,2002:struct"
    def self.tag_class_name; self.class.name.gsub( "Struct::", "" ); end
    def self.tag_read_class( name ); "Struct::#{ name }"; end
    def Struct.yaml_new( tag, val )
        if Hash === val
            struct_type = nil

            #
            # Use existing Struct if it exists
            #
            props = {}
            val.delete_if { |k,v| props[k] = v if k =~ /^@/ }
            begin
                struct_name, struct_type = YAML.read_type_class( tag, Struct )
            rescue NameError
            end
            if not struct_type
                struct_def = [ tag.split( ':', 4 ).last ]
                struct_type = Struct.new( *struct_def.concat( val.keys.collect { |k| k.intern } ) ) 
            end

            #
            # Set the Struct properties
            #
            st = YAML::object_maker( struct_type, {} )
            st.members.each do |m|
                st.send( "#{m}=", val[m] )
            end
            props.each do |k,v|
                st.instance_variable_set(k, v)
            end
            st
        else
            raise YAML::Error, "Invalid Ruby Struct: " + val.inspect
        end
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
			#
			# Basic struct is passed as a YAML map
			#
            out.map( tag_as ) do |map|
				self.members.each do |m|
                    map.add( m, self[m] )
                end
				self.to_yaml_properties.each do |m|
                    map.add( m, instance_variable_get( m ) )
                end
            end
        end
	end
end

#
# Sequences: Array#to_yaml
#
class Array
    tag_as "tag:yaml.org,2002:seq"
    tag_as "tag:ruby.yaml.org,2002:array"
    def yaml_initialize( tag, val ); concat( val.to_a ); end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            array_type = 
                if self.class == Array
                    "tag:yaml.org,2002:seq"
                else
                    tag_as
                end
            out.seq( array_type ) do |seq|
                each do |x|
                    seq.add( x )
                end
            end
        end
	end
end

#
# Exception#to_yaml
#
class Exception
    tag_as "tag:ruby.yaml.org,2002:exception"
    def Exception.yaml_new( tag, val )
        o = YAML.object_maker( self, { 'mesg' => val.delete( 'message' ) } )
        val.each_pair do |k,v|
            o.instance_variable_set("@#{k}", v)
        end
        o
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            out.map( tag_as ) do |map|
                map.add( 'message', message )
				to_yaml_properties.each do |m|
                    map.add( m[1..-1], instance_variable_get( m ) )
                end
            end
        end
	end
end


#
# String#to_yaml
#
class String
    tag_as "tag:yaml.org,2002:str"
    tag_as "tag:ruby.yaml.org,2002:string"
    def is_binary_data?
        ( self.count( "^ -~", "^\r\n" ) / self.size > 0.3 || self.count( "\x00" ) > 0 )
    end
    def String.yaml_new( tag, val )
        tag, obj_class = YAML.read_type_class( tag, self )
        if Hash === val
            s = YAML::object_maker( obj_class, {} )
            # Thank you, NaHi
            String.instance_method(:initialize).
                  bind(s).
                  call( val.delete( 'str' ) )
            val.each { |k,v| s.instance_variable_set( k, v ) }
            s
        else
            raise YAML::Error, "Invalid String: " + val.inspect
        end
    end
    def to_yaml_fold; nil; end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            if to_yaml_properties.empty?
                out.scalar( tag_as, self, to_yaml_fold )
            else
                out.map( tag_as ) do |map|
                    map.add( 'str', "#{self}" )
                    to_yaml_properties.each do |m|
                        map.add( m, instance_variable_get( m ) )
                    end
                end
            end
        end
	end
end
#
# Symbol#to_yaml
#
class Symbol
    tag_as "tag:ruby.yaml.org,2002:symbol"
    tag_as "tag:ruby.yaml.org,2002:sym"
    def Symbol.yaml_new( tag, val )
        if String === val
            val = YAML::load( "--- #{val}") if val =~ /^["'].*['"]$/
            val.intern
        else
            raise YAML::Error, "Invalid Symbol: " + val.inspect
        end
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( nil, opts ) do |out|
            out.scalar( tag_as, ":#{ self.id2name }", :plain )
        end
	end
end

#
# Range#to_yaml
# TODO: Rework the Range as a sequence (simpler)
#
class Range
    tag_as "tag:ruby.yaml.org,2002:range"
    def Range.yaml_new( tag, val )
        type, obj_class = YAML.read_type_class( tag, self )
        inr = %r'(\w+|[+-]?\d+(?:\.\d+)?(?:e[+-]\d+)?|"(?:[^\\"]|\\.)*")'
        opts = {}
        if String === val and val =~ /^#{inr}(\.{2,3})#{inr}$/o
            r1, rdots, r2 = $1, $2, $3
            opts = {
                'begin' => YAML.load( "--- #{r1}" ),
                'end' => YAML.load( "--- #{r2}" ),
                'excl' => rdots.length == 3
            }
            val = {}
        elsif Hash === val
            opts['begin'] = val.delete('begin')
            opts['end'] = val.delete('end')
            opts['excl'] = val.delete('excl')
        end
        if Hash === opts
            r = YAML::object_maker( obj_class, {} )
            # Thank you, NaHi
            Range.instance_method(:initialize).
                  bind(r).
                  call( opts['begin'], opts['end'], opts['excl'] )
            val.each { |k,v| r.instance_variable_set( k, v ) }
            r
        else
            raise YAML::Error, "Invalid Range: " + val.inspect
        end
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            if self.begin.is_complex_yaml? or self.begin.respond_to? :to_str or
              self.end.is_complex_yaml? or self.end.respond_to? :to_str or
              not to_yaml_properties.empty?
                out.map( tag_as ) do |map|
                    map.add( 'begin', self.begin )
                    map.add( 'end', self.end )
                    map.add( 'excl', self.exclude_end? )
                    to_yaml_properties.each do |m|
                        map.add( m, instance_variable_get( m ) )
                    end
                end
            else
                out.scalar( tag_as ) do |sc|
                    sc.embed( self.begin )
                    sc.concat( self.exclude_end? ? "..." : ".." )
                    sc.embed( self.end )
                end
            end
        end
	end
end

#
# Make an Regexp
#
class Regexp
    tag_as "tag:ruby.yaml.org,2002:regexp"
    def Regexp.yaml_new( tag, val )
        type, obj_class = YAML.read_type_class( tag, self )
        if String === val and val =~ /^\/(.*)\/([mix]*)$/
            val = { 'regexp' => $1, 'mods' => $2 }
        end
        if Hash === val
            mods = nil
            unless val['mods'].to_s.empty?
                mods = 0x00
                mods |= Regexp::EXTENDED if val['mods'].include?( 'x' )
                mods |= Regexp::IGNORECASE if val['mods'].include?( 'i' )
                mods |= Regexp::MULTILINE if val['mods'].include?( 'm' )
            end
            val.delete( 'mods' )
            r = YAML::object_maker( obj_class, {} )
            Regexp.instance_method(:initialize).
                  bind(r).
                  call( val.delete( 'regexp' ), mods )
            val.each { |k,v| r.instance_variable_set( k, v ) }
            r
        else
            raise YAML::Error, "Invalid Regular expression: " + val.inspect
        end
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( nil, opts ) do |out|
            if to_yaml_properties.empty?
                out.scalar( tag_as, self.inspect, :plain )
            else
                out.map( tag_as ) do |map|
                    src = self.inspect
                    if src =~ /\A\/(.*)\/([a-z]*)\Z/
                        map.add( 'regexp', $1 )
                        map.add( 'mods', $2 )
                    else
		                raise YAML::Error, "Invalid Regular expression: " + src
                    end
                    to_yaml_properties.each do |m|
                        map.add( m, instance_variable_get( m ) )
                    end
                end
            end
        end
	end
end

#
# Emit a Time object as an ISO 8601 timestamp
#
class Time
    tag_as "tag:ruby.yaml.org,2002:time"
    tag_as "tag:yaml.org,2002:timestamp"
    def Time.yaml_new( tag, val )
        type, obj_class = YAML.read_type_class( tag, self )
        if Hash === val
            t = val.delete( 'at' )
            val.each { |k,v| t.instance_variable_set( k, v ) }
            t
        else
            raise YAML::Error, "Invalid Time: " + val.inspect
        end
    end
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            tz = "Z"
            # from the tidy Tobias Peters <t-peters@gmx.de> Thanks!
            unless self.utc?
                utc_same_instant = self.dup.utc
                utc_same_writing = Time.utc(year,month,day,hour,min,sec,usec)
                difference_to_utc = utc_same_writing - utc_same_instant
                if (difference_to_utc < 0) 
                    difference_sign = '-'
                    absolute_difference = -difference_to_utc
                else
                    difference_sign = '+'
                    absolute_difference = difference_to_utc
                end
                difference_minutes = (absolute_difference/60).round
                tz = "%s%02d:%02d" % [ difference_sign, difference_minutes / 60, difference_minutes % 60]
            end
            standard = self.strftime( "%Y-%m-%d %H:%M:%S" )
            standard += ".%06d" % [usec] if usec.nonzero?
            standard += " %s" % [tz]
            if to_yaml_properties.empty?
                out.scalar( tag_as, standard, :plain )
            else
                out.map( tag_as ) do |map|
                    map.add( 'at', standard )
                    to_yaml_properties.each do |m|
                        map.add( m, instance_variable_get( m ) )
                    end
                end
            end
        end
	end
end

#
# Emit a Date object as a simple implicit
#
class Date
    tag_as "tag:yaml.org,2002:timestamp#ymd"
	def to_yaml( opts = {} )
		YAML::quick_emit( object_id, opts ) do |out|
            out.scalar( "tag:yaml.org,2002:timestamp", self.to_s, :plain )
        end
	end
end

#
# Send Integer, Booleans, NilClass to String
#
class Numeric
    tag_as "tag:yaml.org,2002:int"
	def to_yaml( opts = {} )
		YAML::quick_emit( nil, opts ) do |out|
            str = self.to_s
            if str == "Infinity"
                str = ".Inf"
            elsif str == "-Infinity"
                str = "-.Inf"
            elsif str == "NaN"
                str = ".NaN"
            end
            out.scalar( tag_as, str, :plain )
        end
	end
end

class TrueClass
    tag_as "tag:yaml.org,2002:bool#yes"
	def to_yaml( opts = {} )
		out.scalar( tag_as, "true", :plain )
	end
end

class FalseClass
    tag_as "tag:yaml.org,2002:bool#no"
	def to_yaml( opts = {} )
		out.scalar( tag_as, "false", :plain )
	end
end

class NilClass 
    tag_as "tag:yaml.org,2002:null"
	def to_yaml( opts = {} )
		out.scalar( tag_as, "", :plain )
	end
end


#												vim:sw=4:ts=4
# $Id$
#
require 'yaml'

module YAML

#
# Make a time with the time zone
#
def YAML::mktime( year, mon, day, hour, min, sec, usec, zone = "Z" )
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

class Stream
	def ==( doc )
		self.documents == doc.documents
	end
end

class PrivateType
	def ==( pt )
		self.type_id == pt.type_id and self.value == pt.value
	end
end

class DomainType
	def ==( dt )
		self.domain == dt.domain and self.type_id == dt.type_id and self.value == dt.value
	end
end

class SpecialHash
	def ==( h )
		if h.is_a? SpecialHash
			self.default == h.default and not self.keys.detect { |k| self[k] != h[k] }
		else
			false
		end
	end
	def inspect
		"{SpecialHash: @default=#{@default} @hash=#{super}}"
	end
end

end

header = " %YAML:1.0"
YAML::load( File.read( "index.yml" ) ).each do |yst|

	YAML.load_documents( File.read( yst + ".yml" ) ) do |ydoc|
		#
		# Test the document
		#
		reason = nil
		success = 'no'
		round_trip = 'no'
		round_out = nil
        interval = nil
		if ydoc.has_key?( 'ruby' )
			obj_r = nil
			obj_y = nil
			begin
				eval( ydoc['ruby-setup'] ) if ydoc.has_key?( 'ruby-setup' )
				time = Time.now
                ydoc['yaml'].gsub!( / +$/, '' )
				if ydoc.has_key?( 'documents' )
					obj_y = YAML::load_stream( ydoc['yaml'] )
				else
					obj_y = YAML::load( ydoc['yaml'] )
				end
				interval = Time.now - time
				eval( "obj_r = #{ydoc['ruby']}" )
			
                if obj_r == obj_y
                    success = 'yes'

                    # Attempt round trip
                    unless ydoc['no-round-trip'].is_a?( Array ) and ydoc['no-round-trip'].include?( 'ruby' )
                        obj_y2 = nil
                        begin
                            if obj_y.is_a? YAML::Stream
                                round_out = obj_y.emit
                                obj_y2 = YAML::load_stream( round_out )
                            else
                                round_out = obj_y.to_yaml
                                obj_y2 = YAML::load( round_out )
                            end
                        rescue YAML::Error => e
                            reason = e.to_s
                        end

                        obj_y = obj_y2
				        eval( "obj_r = #{ydoc['ruby']}" )
                        if obj_r == obj_y
                            round_trip = 'yes'
                        else
                            reason = 'Expected <' + obj_r.inspect + '>, but was <' +
                            	obj_y2.inspect + '>'
                        end
                    else
                    end
                else
                    reason = 'Expected <' + obj_r.inspect + '>, but was <' +
                        obj_y.inspect + '>'
                end
			rescue Exception => e
				reason = e.to_s
			end
		else
			reason = 'No Ruby parse information available in the test document.'
		end

		#
		# Print out YAML result
		#
		puts <<EOY
---#{header}
file: #{yst}
test: #{ydoc['test']}
success: #{success}
round-trip: #{round_trip}
EOY
		puts "time: %0.6f" % interval if interval
		if round_out and round_trip == 'no'
			round_out.gsub!( /\t/, '  ' )
			round_out.gsub!( /\n/, "\n  " )
			puts <<EOY
round-trip-output: |
  #{round_out}
EOY
		end
		if reason
			reason.gsub!( /\t/, '  ' )
			reason.gsub!( /\n/, "\n  " )
			puts <<EOY
reason: |
  #{reason}
EOY
		end
		header = ""
    end
end

# YPath Test is a bit different
%w(
    YtsYpath.yml
).each { |yst|
	YAML.parse_documents( File.open( yst ) ) { |ydoc|
        next unless ydoc.value
		#
		# Test the document
		#
		reason = nil
		success = 'no'
        begin
            obj = ydoc.select( '/data' )[0]
            ydoc = ydoc.transform
            paths = obj.search( ydoc['ypath'] )
            if ydoc['expected'].sort == paths.to_a.sort
                success = 'yes'
            else
                reason = 'Expected <' + ydoc['expected'].inspect + '>, but was <' +
                    paths.inspect + '>'
            end
        rescue YAML::Error => e
            reason = e.to_s
        end
		puts <<EOY
---
file: #{yst}
path: #{ydoc['ypath']}
test: #{ydoc['test']}
success: #{success}
EOY
		if reason
			reason.gsub!( /\t/, '  ' )
			reason.gsub!( /\n/, "\n  " )
			puts <<EOY
reason: |
  #{reason}
EOY
        end
    }
}

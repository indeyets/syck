require 'yaml'

class String
  def wordwrap( len )
    gsub( /\n/, "\n\n" ).gsub( /.{#{len},}?\s+/, "\\0\n" )
  end
end

puts "** Ordering YAML mapping keys **"
puts

#
# Robert Feldt wrote:
#
# > ...when I use Yaml for ini files I'd like to be able to
# > spec an order for map pairs to be serialized in. Obviously Ruby hashes has
# > no garantuee for this but are there any plans on supporting this? If not
# > I'll whip something up myself; I really wanna make sure the most important
# > stuff comes out on top...
#
# I had three suggestions:
#
# 1. When using objects, you can specify the ordering of your properties
#    explicitly by defining a `to_yaml_properties' method.
#

class Instrument

    attr_accessor :name, :key, :is_woodwind

    def initialize( n, k, ww )
        @name, @key, @is_woodwind = n, k, ww
    end

    def to_yaml_properties
        [ '@name', '@key', '@is_woodwind' ]
    end

end

puts "-- 1. Instrument object --"
test1 = Instrument.new( 'Alto Saxophone', 'Eb', true )
puts test1.inspect.wordwrap( 30 ).gsub( /^/, '   ' )
puts

puts "** With ordered properties **"
puts test1.to_yaml.gsub( /^/, '   ' )
puts

#
# 2. The same can't be done for Hashes because the key set isn't
#    predictable.  But if the :SortKeys method is called, then 
#    the Hash will be sorted with Hash#sort.  You could define a
#    singleton `sort' method to sort certain Hashes.
#

test2 = { 'name' => 'Alto Saxophone', 'key' => 'Eb',
          'is_woodwind' => true }
def test2.sort
    order = [ 'name', 'key', 'is_woodwind' ]
    super { |a, b| order.index( a[0] ) <=> order.index( b[0] ) }
end

puts "-- 2. Instrument hash --"
puts test2.inspect.wordwrap( 30 ).gsub( /^/, '   ' )
puts

puts "** With ordered keys **"
puts test2.to_yaml( :SortKeys => true ).gsub( /^/, '   ' )
puts

#
#     Alternatively, you could define a singleton `to_a' to sort
#     correctly and skip the :SortKeys option.
#
# 3. Finally, the YAML spec now defines an !omap type.  This type is
#    an ordered mapping with unique keys.  YAML.rb will load this
#    into a Hash-like class.
#

test3 = YAML::Omap[ 
    'name', 'Alto Saxophone', 
    'key', 'Eb', 
    'is_woodwind', true
]

puts "-- 3. Instrument Omap --"
puts test3.inspect.wordwrap( 30 ).gsub( /^/, '   ' )
puts

puts "** With ordered keys **"
puts test3.to_yaml.gsub( /^/, '   ' )
puts

#
# Robert's answer was great.  He used a mixin to add a singleton `to_a'
# method, which allows him to easily sort many hashes.
#

module SortableToAKeys

    def key_order=( keys )
        @__key_order = keys
    end

    def to_a
        ks, out = self.keys, []
        (@__key_order + (ks - @__key_order)).each do |key|
            out << [key, self[key]] if ks.include?(key)
        end
        out
    end

end

test4 = { 'name' => 'Alto Saxophone', 'key' => 'Eb',
          'is_woodwind' => true }
puts "-- 4. Instrument SortableToAKeys --"
puts test4.inspect.wordwrap( 30 ).gsub( /^/, '   ' )
puts

test4.extend SortableToAKeys
test4.key_order = [ 'name', 'key', 'is_woodwind' ]

puts "** With ordered keys **"
puts test4.to_yaml.gsub( /^/, '   ' )
puts


#
# Simple client to read metadata from whytheluckystiff.net's
# demo !okay/rpc server.
#
require 'okay/rpc'

host = 'whytheluckystiff.net'
c = Okay::RPC::Client.new( host )

# Wrapping strings for display
class String
  def wordwrap( len )
    gsub( /\n/, "\n\n" ).gsub( /(.{1,#{len}})(\s+|$)/, "\\1\n" )
  end
end

puts "** Sample Client for !okay/rpc using client version #{Okay::RPC::VERSION} **"
puts "** Using !okay/rpc server at #{host} **"
puts "-- system.about() --"
puts
about = c.call( 'system.about' )
if about.is_a? Okay::RPC::Fault
  puts "  Fault: " + about.inspect
  exit
end

puts "   Server: #{ about['name'] }"
puts "   Version: #{ about['version'] }"
puts
puts about['about'].wordwrap( 30 ).gsub!( /^/, '   ' )
puts

puts "-- system.listMethods() --"
puts
methods = c.call( 'system.listMethods' )
puts "** #{methods.length} methods available on server **"
puts "** Requesting method signatures and docs **"
methods.each { |m|
  c.qcall( 'system.methodSignature', m )
  c.qcall( 'system.methodHelp', m )
}
methodSigs = c.qrun.documents

methods.each { |m|
  sig = methodSigs.shift
  help = methodSigs.shift
  puts
  puts "-- !#{sig.shift} #{m}(#{sig.collect { |p| '!' + p }.join( ', ' )}) --"
  puts help.wordwrap( 30 ).gsub( /^/, '   ' )
}


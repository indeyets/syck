#
# The demo server from whytheluckystiff.net.
#
require 'okay/rpc'

s = Okay::RPC::ModRubyServer.new

s.name = "The !okay/rpc Demo Server"
s.add_author( 'why the lucky stiff', 'okay-rpc@whytheluckystiff.net', 'http://whytheluckystiff.net/' )
s.about = <<EOY

    Welcome to the !okay/rpc server at whytheluckystiff.net.
    This server acts as an example to the inhabited world
    concerning the possibilities of using YAML for remote
    procedure calls.

    Yeah, so, it's a real treat having you use the
    system.about method.  I can't imagine what it must
    feel like from your end.  Provocative, I'm sure.
    Issuing the single statement and getting back this
    gorgeous reply.  Yes, life has started for you in
    a very real way, hasn't it?

EOY

# currentTime.getCurrentTime
s.add_handler( "currentTime.getCurrentTime", %w(time), "Returns the current server time." ) { |meth|
    Time.now
}

# examples.getStateName
stateName = [
    "Alabama", "Alaska", "Arizona", "Arkansas", "California",
    "Colorado", "Columbia", "Connecticut", "Delaware", "Florida",
    "Georgia", "Hawaii", "Idaho", "Illinois", "Indiana", "Iowa", "Kansas",
    "Kentucky", "Louisiana", "Maine", "Maryland", "Massachusetts", "Michigan",
    "Minnesota", "Mississippi", "Missouri", "Montana", "Nebraska", "Nevada",
    "New Hampshire", "New Jersey", "New Mexico", "New York", "North Carolina",
    "North Dakota", "Ohio", "Oklahoma", "Oregon", "Pennsylvania", "Rhode Island",
    "South Carolina", "South Dakota", "Tennessee", "Texas", "Utah", "Vermont",
    "Virginia", "Washington", "West Virginia", "Wisconsin", "Wyoming"
]
s.add_handler( "examples.getStateName", %w(str int), "When passed an integer between 1 and 51 returns the name of a US state, where the integer is the index of that state name in an alphabetic order." ) { |meth|
    stateName[ meth.params[0] - 1 ]
}

# examples.sortByAge
s.add_handler( "examples.sortByAge", %w(seq seq), "Send this method an array of [str, int] sequences and the array will be returned with the entries sorted by their numbers." ) { |meth|
    meth.params[0].sort { |x,y|
        x[1] <=> y[1]
    }
}

# examples.addtwo
s.add_handler( "examples.addtwo", %w(int int int), "Add two integers together and return the result." ) { |meth|
    meth.params[0] + meth.params[1]
}

# examples.addtwofloat
s.add_handler( "examples.addtwofloat", %w(float float float), "Add two floats together and return the result." ) { |meth|
    meth.params[0] + meth.params[1]
}

# examples.stringecho
s.add_handler( "examples.stringecho", %w(str str), "Accepts a string parameter, returns the string." ) { |meth|
    meth.params[0]
}

# examples.echo
s.add_handler( "examples.echo", %w(str str), "Accepts a string parameter, returns the entire incoming payload." ) { |meth|
    meth
}

# examples.base64
s.add_handler( "examples.base64", %w(str str), "Accepts a base64 parameter and returns it decoded as a string." ) { |meth|
    [meth.params[0]].pack( "m" )
}

# examples.invertBooleans
s.add_handler( "examples.invertBooleans", %w(seq seq), "Accepts an array of booleans, and returns them inverted." ) { |meth|
    meth.params[0].collect{ |x| !x }
}

s.serve


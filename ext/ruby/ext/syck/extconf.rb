require 'mkmf'

[ 'gram.c', 'gram.h', 'handler.c', 'node.c', 'syck.c', 'syck.h', 'token.c', 'implicit.c' ].
    each { |codefile|
      `cp #{File::dirname $0}/../../../../lib/#{codefile} #{codefile}`
  }
have_header( "st.h" )
create_makefile( "syck" )


require 'mkmf'

[ 'emitter.c', 'gram.c', 'gram.h', 'handler.c', 'node.c', 'syck.c', 'syck.h', 'token.c', 'bytecode.c', 'implicit.c', 'yaml2byte.c', 'yamlbyte.h' ].each do |codefile|
    `cp #{File::dirname $0}/../../../../lib/#{codefile} #{codefile}`
end

have_header( "st.h" )
create_makefile( "syck" )


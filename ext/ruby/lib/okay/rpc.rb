#
# The !okay/rpc module for YAML.rb
# Specification at http://wiki.yaml.org/yamlwiki/OkayRpcProtocol
#
require 'okay'
require 'net/http'

module Okay
    module RPC

        VERSION = '0.06'

        class Method
            attr_accessor :methodName, :params
            def initialize( mn, p )
                @methodName = mn
                @params = p
            end
            def to_yaml( opts = {} )
                YAML::quick_emit( self.id, opts ) { |out|
                    out.map( "!okay/rpc/method" ) { |map|
                        map.add( self.methodName, self.params )
                    }
                }
            end
        end

        YAML.add_domain_type( "okay.yaml.org,2002", "rpc/method" ) { |type, val|
            if val.class == Hash and val.length == 1
                val = val.to_a.first
                RPC::Method.new( val[0], val[1] )
            end
        }

        class Fault
            attr_accessor :code, :message
            def initialize( c, m )
                @code = c
                @message = m
            end
            def to_yaml( opts = {} )
                YAML::quick_emit( self.id, opts ) { |out|
                    out.map( "!okay/rpc/fault" ) { |map|
                        map.add( self.code, self.message )
                    }
                }
            end
        end

        YAML.add_domain_type( "okay.yaml.org,2002", "rpc/fault" ) { |type, val|
            if val.class == Hash and val.length == 1
                val = val.to_a.first
                Fault.new( val[0], val[1] )
            end
        }

        class Client
            USER_AGENT = "Okay::RPC::Client (Ruby #{RUBY_VERSION})"
            def initialize(host=nil, path=nil, port=nil, proxy_host=nil, proxy_port=nil, 
                           user=nil, password=nil, use_ssl=nil, timeout=nil)

                @host       = host || "localhost"
                @path       = path || "/okayRpc/"
                @proxy_host = proxy_host
                @proxy_port = proxy_port
                @proxy_host ||= 'localhost' if @proxy_port != nil
                @proxy_port ||= 8080 if @proxy_host != nil
                @use_ssl    = use_ssl || false
                @timeout    = timeout || 30
                @queue      = YAML::Stream.new( :UseHeader => true )

                if use_ssl
                  require "net/https"
                  @port = port || 443
                else
                  @port = port || 80
                end

                @user, @password = user, password

                set_auth

                # convert ports to integers
                @port = @port.to_i if @port != nil
                @proxy_port = @proxy_port.to_i if @proxy_port != nil

                # HTTP object for synchronous calls
                Net::HTTP.version_1_2
                @http = Net::HTTP.new(@host, @port, @proxy_host, @proxy_port) 
                @http.use_ssl = @use_ssl if @use_ssl
                @http.read_timeout = @timeout
                @http.open_timeout = @timeout

                @parser = nil
                @create = nil
            end

            # Attribute Accessors -------------------------------------------------------------------

            attr_reader :timeout, :user, :password

            def timeout=(new_timeout)
                @timeout = new_timeout
                @http.read_timeout = @timeout
                @http.open_timeout = @timeout
            end

            def user=(new_user)
                @user = new_user
                set_auth
            end

            def password=(new_password)
                @password = new_password
                set_auth
            end

            # Call methods --------------------------------------------------------------

            def call(method, *args)
                meth = Method.new( method, args )
                YAML::load( do_rpc(meth.to_yaml, false) )
            end 

            def qcall(method, *args)
                @queue.add( Method.new( method, args ) )
            end

            def qrun
                ret = YAML.load_stream( do_rpc( @queue.emit, false ) )
                @queue = YAML::Stream.new( :UseHeader => true )
                ret
            end

            private # ----------------------------------------------------------

            def set_auth
                if @user.nil?
                    @auth = nil
                else
                    a =  "#@user"
                    a << ":#@password" if @password != nil
                    @auth = ("Basic " + [a].pack("m")).chomp
                end
            end

            def do_rpc(request, async=false)
                header = {  
                   "User-Agent"     =>  USER_AGENT,
                   "Content-Type"   => "text/yaml",
                   "Content-Length" => request.size.to_s, 
                   "Connection"     => (async ? "close" : "keep-alive")
                }

                if @auth != nil
                    # add authorization header
                    header["Authorization"] = @auth
                end
 
                if async
                    # use a new HTTP object for each call 
                    Net::HTTP.version_1_2
                    http = Net::HTTP.new(@host, @port, @proxy_host, @proxy_port) 
                    http.use_ssl = @use_ssl if @use_ssl
                    http.read_timeout = @timeout
                    http.open_timeout = @timeout
                else
                    # reuse the HTTP object for each call => connection alive is possible
                    http = @http
                end
  
                # post request
                resp = http.post2(@path, request, header) 
                data = resp.body
                http.finish if async

                if resp.code == "401"
                    # Authorization Required
                    raise "Authorization failed.\nHTTP-Error: #{resp.code} #{resp.message}" 
                elsif resp.code[0,1] != "2"
                    raise "HTTP-Error: #{resp.code} #{resp.message}" 
                end

                if resp["Content-Type"] != "text/yaml"
                    raise "Wrong content-type [#{resp['Content-Type']}]: \n#{data}"
                end

                expected = resp["Content-Length"] || "<unknown>"
                if data.nil? or data.size == 0 
                    raise "Wrong size. Was #{data.size}, should be #{expected}" 
                elsif expected.to_i != data.size and resp["Transfer-Encoding"].nil?
                    raise "Wrong size. Was #{data.size}, should be #{expected}"
                end

                return data
            end

        end

        class BasicServer
            def initialize
                @handlers = {}
                @aboutHash = {
                  'name' => 'An !okay/rpc server.',
                  'uri' => nil,
                  'version' => VERSION,
                  'authors' => nil,
                  'about' => "Welcome to the !okay/rpc server."
                }
                add_introspection
            end
            def name( str )
                @aboutHash['name']
            end
            def name=( str )
                @aboutHash['name'] = str
            end
            def about
                @aboutHash['about']
            end
            def about=( str )
                @aboutHash['about'] = str.strip.gsub( /^ +/, '' ).gsub( /\n(\n*)/, ' \1' )
            end
            def uri( str )
                @aboutHash['uri']
            end
            def uri=( str )
                @aboutHash['uri'] = str
            end
            def add_author( name, email, url )
                @aboutHash['authors'] ||= []
                @aboutHash['authors'] << {
                  'name' => name,
                  'email' => email,
                  'url' => url
                }
            end
            def add_handler( methodName, sig, doc, &block )
                @handlers[ methodName ] = {
                    :signature => sig,
                    :help => doc,
                    :block => block
                }
            end
            def get_handler( methodName, prop )
                unless @handlers.has_key?( methodName )
                    Fault.new( 101, "No method ''#{methodName}'' available." )
                else
                    @handlers[ methodName ][ prop ]
                end
            end
            def dispatch( meth )
                b = get_handler( meth.methodName, :block )
                return b if b.is_a? Fault
                b.call( meth )
            end
            def process( meth_yml )
                s = YAML::Stream.new( :UseHeader => true )
                YAML::load_documents( meth_yml ) { |doc|
                    s.add( dispatch( doc ) )
                }
                s.emit
            end

            private

            def add_introspection

                add_handler( 
                    "system.about", %w(map), 
                    "Provides a short description of this !okay/rpc server's intent."
                ) {
                    @aboutHash
                }

                add_handler( 
                    "system.getCapabilities", %w(map), 
                    "Describes this server's capabilities, including version of " +
                    "!okay/rpc available and YAML implementation." 
                ) { 
                    {
                      'okay/rpc' =>
                        { 'version' => Okay::RPC::VERSION,
                          'id' => 'YAML.rb Okay::RPC',
                          'url' => 'http://wiki.yaml.org/yamlwiki/OkayRpcProtocol'
                        },
                      'yaml' =>
                        { 'version' => YAML::VERSION,
                          'id' => 'YAML.rb',
                          'url' => 'http://yaml4r.sf.net/'
                        },
                      'sys' =>
                        { 'version' => Kernel::RUBY_VERSION,
                          'id' => Kernel::RUBY_PLATFORM
                        }
                    }
                }

                add_handler( 
                    "system.listMethods", %w(seq), 
                    "Lists the available methods for this !okay/rpc server."
                ) {
                    @handlers.keys.sort
                }

                add_handler( 
                    "system.methodSignature", %w(seq str), 
                    "Returns a method signature." 
                ) { |meth|
                    get_handler( meth.params[0], :signature )
                }

                add_handler( 
                    "system.methodHelp", %w(str str), 
                    "Returns help on using this method." 
                ) { |meth|
                    get_handler( meth.params[0], :help )
                }

                add_handler( 
                    "system.methodBlank", %w(str str), 
                    "Returns a blank method." 
                ) { |meth|
                    sig = get_handler( meth.params[0], :signature )
                    unless sig.is_a? Fault
                        "--- !okay/rpc/method\n#{meth.params[0]}:\n" +
                            sig[1..-1].collect { |type| "  - !#{type}" }.join( "\n" )
                    else
                        sig
                    end
                }

            end
        end

        class ModRubyServer < BasicServer

            def initialize(*a)
                @ap = Apache::request
                super(*a)
                url ||= @ap.uri
            end
            
            def serve
                catch(:exit_serve) {
                    header = {} 
                    hdr_in_proc = proc {|key, value| header[key.capitalize] = value}
                    if @ap.respond_to? :headers_in
                        @ap.headers_in.each( &hdr_in_proc )
                    else
                        @ap.each_header( &hdr_in_proc )
                    end

                    length = header['Content-length'].to_i

                    http_error(405, "Method Not Allowed") unless @ap.request_method  == "POST" 
                    http_error(400, "Bad Request")        unless header['Content-type'] == "text/yaml"
                    http_error(411, "Length Required")    unless length > 0 

                    # TODO: do we need a call to binmode?
                    @ap.binmode
                    data = @ap.read(length)

                    http_error(400, "Bad Request")        if data.nil? or data.size != length

                    resp =
                        begin
                            process(data)
                        rescue Exception => e
                            Fault.new(101, e.message).to_yaml
                        end
                    http_write(resp, 200, "Content-type" => "text/yaml")
                }
            end


            private

            def http_error(status, message)
                err = "#{status} #{message}"
                msg = <<-"MSGEND" 
                  <html>
                    <head>
                      <title>#{err}</title>
                    </head>
                    <body>
                      <h1>#{err}</h1>
                      <p>Unexpected error occured while processing !okay/rpc request!</p>
                    </body>
                  </html>
                MSGEND

                http_write(msg, status, "Status" => err, "Content-type" => "text/html")
                throw :exit_serve # exit from the #serve method
            end

            def http_write(body, status, header)
                h = {}
                header.each {|key, value| h[key.to_s.capitalize] = value}
                h['Status']         ||= "200 OK"
                h['Content-length'] ||= body.size.to_s 

                h.each do |key, value| 
                    if @ap.respond_to? :headers_out
                        @ap.headers_out[key] = value
                    else
                        @ap[key] = value
                    end
                end
                @ap.content_type = h["Content-type"] 
                @ap.status = status.to_i 
                @ap.send_http_header 

                @ap.print body
            end
        end

    end
end

Okay.load_schema( <<EOY )

--- %YAML:1.0 !okay/schema
okay/rpc/method:
  description: >
    XML-RPC for YAML.
  examples: >
    Testing
  schema:
    - map:
        /*: [ seq ]
        length: 1

EOY

#												vim:sw=4:ts=4
# $Id$
#
require 'runit/testcase'
require 'runit/cui/testrunner'
require 'yaml'

# [ruby-core:01946]
module YAML_Tests
    StructTest = Struct::new( :c )
end

class YAML_Unit_Tests < RUNIT::TestCase
	#
	# Convert between YAML and the object to verify correct parsing and
	# emitting
	#
	def assert_to_yaml( obj, yaml )
		assert_equal( obj, YAML::load( yaml ) )
        assert_equal( obj, YAML::parse( yaml ).transform )
        assert_equal( obj, YAML::load( obj.to_yaml ) )
        assert_equal( obj, YAML::parse( obj.to_yaml ).transform )
        assert_equal( obj, YAML::load(
			obj.to_yaml( :UseVersion => true, :UseHeader => true, :SortKeys => true ) 
		) )

        # yb = YAML::Parser.new
        # yb.resolver = YAML.resolver
        # yb.input = :bytecode
        # assert_equal( obj, yb.load( YAML::Syck::compile( yaml ) ) )
	end

    #
    # Test bytecode parser
    #
    def assert_bytecode( obj, yaml )
        yp = YAML::Parser.new
        yp.input = :bytecode
        assert_equal( obj, yp.load( yaml ) )
    end

	#
	# Test parser only
	#
	def assert_parse_only( obj, yaml )
		assert_equal( obj, YAML::load( yaml ) )
        assert_equal( obj, YAML::parse( yaml ).transform )
        yp = YAML::Parser.new
        yp.input = :bytecode
        assert_equal( obj, yp.load( YAML::Syck::compile( yaml ) ) )
	end

    def assert_path_segments( path, segments )
        YAML::YPath.each_path( path ) { |choice|
            assert_equal( choice.segments, segments.shift )
        }
        assert_equal( segments.length, 0, "Some segments leftover: #{ segments.inspect }" )
    end

	#
	# Make a time with the time zone
	#
	def mktime( year, mon, day, hour, min, sec, usec, zone = "Z" )
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
	# Tests modified from 00basic.t in YAML.pm
	#
	def test_basic_map
		# Simple map
        map = { 'one' => 'foo', 'three' => 'baz', 'two' => 'bar' }
		assert_to_yaml(
			map, <<EOY
one: foo
two: bar
three: baz
EOY
		)
        assert_bytecode( map, "D\nM\nSone\nSfoo\nStwo\nSbar\nSthree\nSbaz\nE\n" )
	end

	def test_basic_strings
		# Common string types
		basic = { 1 => 'simple string', 2 => 42, 3 => '1 Single Quoted String',
			  4 => 'YAML\'s Double "Quoted" String', 5 => "A block\n  with several\n    lines.\n",
			  6 => "A \"chomped\" block", 7 => "A folded\n string\n" }
		assert_to_yaml(
			basic, <<EOY
1: simple string
2: 42
3: '1 Single Quoted String'
4: "YAML's Double \\\"Quoted\\\" String"
5: |
  A block
    with several
      lines.
6: |-
  A "chomped" block
7: >
  A
  folded
   string
EOY
		)
        assert_bytecode( basic,
            "D\nM\nS1\nSsimple string\nS2\nS42\nS3\nS1 Single Quoted String\nS4\nSYAML's Double \"Quoted\" String\n" +
            "S5\nSA block\nN\nC  with several\nN\nC    lines.\nN\nS6\nSA \"chomped\" block\nS7\nSA folded\nN\nC string\nN\nE\n" )
	end

	#
	# Test the specification examples
	# - Many examples have been changes because of whitespace problems that
	#   caused the two to be inequivalent, or keys to be sorted wrong
	#

	def test_spec_simple_implicit_sequence
	  	# Simple implicit sequence
        seq = [ 'Mark McGwire', 'Sammy Sosa', 'Ken Griffey' ]
		assert_to_yaml(
			seq, <<EOY
- Mark McGwire
- Sammy Sosa
- Ken Griffey
EOY
		)
        assert_bytecode( seq, "D\nQ\nSMark McGwire\nSSammy Sosa\nSKen Griffey\nE\n" )
	end

	def test_spec_simple_implicit_map
		# Simple implicit map
        map = { 'hr' => 65, 'avg' => 0.278, 'rbi' => 147 }
		assert_to_yaml(
			map, <<EOY
avg: 0.278
hr: 65
rbi: 147
EOY
		)
        assert_bytecode( map, "D\nM\nSavg\nS0.278\nShr\nS65\nSrbi\nS147\nE\n" )
	end

	def test_spec_simple_map_with_nested_sequences
		# Simple mapping with nested sequences
        nest = { 'american' => 
			  [ 'Boston Red Sox', 'Detroit Tigers', 'New York Yankees' ],
			  'national' =>
			  [ 'New York Mets', 'Chicago Cubs', 'Atlanta Braves' ] }
		assert_to_yaml(
			nest, <<EOY
american:
  - Boston Red Sox
  - Detroit Tigers
  - New York Yankees
national:
  - New York Mets
  - Chicago Cubs
  - Atlanta Braves
EOY
		)
        assert_bytecode( nest, "D\nM\nSamerican\nQ\nSBoston Red Sox\nSDetroit Tigers\nSNew York Yankees\nE\n" +
            "Snational\nQ\nSNew York Mets\nSChicago Cubs\nSAtlanta Braves\nE\n" )
	end

	def test_spec_simple_sequence_with_nested_map
		# Simple sequence with nested map
        nest = [
		    {'name' => 'Mark McGwire', 'hr' => 65, 'avg' => 0.278},
			{'name' => 'Sammy Sosa', 'hr' => 63, 'avg' => 0.288}
		]
		assert_to_yaml(
		  nest, <<EOY
-
  avg: 0.278
  hr: 65
  name: Mark McGwire
-
  avg: 0.288
  hr: 63
  name: Sammy Sosa
EOY
		)
        assert_bytecode( nest, "D\nQ\nM\nSavg\nS0.278\nShr\nS65\nSname\nSMark McGwire\nE\n" +
            "M\nSavg\nS0.288\nShr\nS63\nSname\nSSammy Sosa\nE\nE\n" )
	end

	def test_spec_sequence_of_sequences
		# Simple sequence with inline sequences
        seq = [ 
		  	[ 'name', 'hr', 'avg' ],
			[ 'Mark McGwire', 65, 0.278 ],
			[ 'Sammy Sosa', 63, 0.288 ]
		]
		assert_to_yaml(
		  seq, <<EOY
- [ name         , hr , avg   ]
- [ Mark McGwire , 65 , 0.278 ]
- [ Sammy Sosa   , 63 , 0.288 ]
EOY
		)
        assert_bytecode( seq, "D\nQ\nQ\nSname\nShr\nSavg\nE\nQ\nSMark McGwire\nS65\nS0.278\nE\n" +
            "Q\nSSammy Sosa\nS63\nS0.288\nE\nE\n" )
	end

	def test_spec_mapping_of_mappings
		# Simple map with inline maps
        map = { 'Mark McGwire' =>
		    { 'hr' => 65, 'avg' => 0.278 },
			'Sammy Sosa' =>
		    { 'hr' => 63, 'avg' => 0.288 }
		}
		assert_to_yaml(
		  map, <<EOY
Mark McGwire: {hr: 65, avg: 0.278}
Sammy Sosa:   {hr: 63,
               avg: 0.288}
EOY
		)
        assert_bytecode( map, "D\nM\nSMark McGwire\nM\nShr\nS65\nSavg\nS0.278\nE\n" +
            "SSammy Sosa\nM\nShr\nS63\nSavg\nS0.288\nE\nE\n" )
	end

    def test_ambiguous_comments
        assert_to_yaml( "Call the method #dave", <<EOY )
--- "Call the method #dave"
EOY
    end

	def test_spec_nested_comments
		# Map and sequences with comments
        nest = { 'hr' => [ 'Mark McGwire', 'Sammy Sosa' ],
		    'rbi' => [ 'Sammy Sosa', 'Ken Griffey' ] }
		assert_to_yaml(
		  nest, <<EOY
hr: # 1998 hr ranking
  - Mark McGwire
  - Sammy Sosa
rbi:
  # 1998 rbi ranking
  - Sammy Sosa
  - Ken Griffey
EOY
		)
        assert_bytecode( nest, "D\nM\nShr\nc 1998 hr ranking\nQ\nSMark McGwire\nSSammy Sosa\nE\n" + 
            "Srbi\nc 1998 rbi ranking\nQ\nSSammy Sosa\nSKen Griffey\nE\nE\n" )
	end

	def test_spec_anchors_and_aliases
		# Anchors and aliases
        anc1 = { 'hr' =>
			  [ 'Mark McGwire', 'Sammy Sosa' ],
			  'rbi' =>
			  [ 'Sammy Sosa', 'Ken Griffey' ] }
		assert_to_yaml( anc1, <<EOY )
hr:
   - Mark McGwire
   # Name "Sammy Sosa" scalar SS
   - &SS Sammy Sosa
rbi:
   # So it can be referenced later.
   - *SS
   - Ken Griffey
EOY

        assert_bytecode( anc1, "D\nM\nShr\nQ\nSMark McGwire\nc Name \"Sammy Sosa\" scalar SS\n" +
            "ASS\nSSammy Sosa\nE\nSrbi\nc So it can be referenced later\nQ\nRSS\nSKen Griffey\nE\nE\n" )

        anc2 = [{"arrival"=>"EDI", "departure"=>"LAX", "fareref"=>"DOGMA", "currency"=>"GBP"}, {"arrival"=>"MEL", "departure"=>"SYD", "fareref"=>"MADF", "currency"=>"AUD"}, {"arrival"=>"MCO", "departure"=>"JFK", "fareref"=>"DFSF", "currency"=>"USD"}]
        assert_to_yaml(
            anc2, <<EOY
  -   
    &F fareref: DOGMA
    &C currency: GBP
    &D departure: LAX
    &A arrival: EDI 
  - { *F: MADF, *C: AUD, *D: SYD, *A: MEL }
  - { *F: DFSF, *C: USD, *D: JFK, *A: MCO }
EOY
        )

        assert_bytecode( anc2, "D\nQ\nM\nAF\nSfareref\nSDOGMA\nAC\nScurrency\nSGBP\n" +
            "AD\nSdeparture\nSLAX\nAA\nSarrival\nSEDI\nE\n" +
            "M\nRF\nSMADF\nRC\nSAUD\nRD\nSSYD\nRA\nSMEL\nE\n" + 
            "M\nRF\nSDFSF\nRC\nSUSD\nRD\nSJFK\nRA\nSMCO\nE\n" + 
            "E\n" )

        anc3 = {"ALIASES"=>["fareref", "currency", "departure", "arrival"], "FARES"=>[{"arrival"=>"EDI", "departure"=>"LAX", "fareref"=>"DOGMA", "currency"=>"GBP"}, {"arrival"=>"MEL", "departure"=>"SYD", "fareref"=>"MADF", "currency"=>"AUD"}, {"arrival"=>"MCO", "departure"=>"JFK", "fareref"=>"DFSF", "currency"=>"USD"}]}
        assert_to_yaml(
            anc3, <<EOY
---
ALIASES: [&f fareref, &c currency, &d departure, &a arrival]
FARES:
- *f: DOGMA
  *c: GBP
  *d: LAX
  *a: EDI

- *f: MADF
  *c: AUD
  *d: SYD
  *a: MEL

- *f: DFSF
  *c: USD
  *d: JFK
  *a: MCO

EOY
        )
        assert_bytecode( anc3, "D\nM\nSALIASES\nQ\nAf\nSfareref\nAc\nScurrency\nAd\nSdeparture\nAa\nSarrival\nE\n" +
            "SFARES\nQ\nM\nRf\nSDOGMA\nRc\nSGBP\nRd\nSLAX\nRa\nSEDI\nE\n" +
            "M\nRf\nSMADF\nRc\nSAUD\nRd\nSSYD\nRa\nSMEL\nE\n" +
            "M\nRf\nSDFSF\nRc\nSUSD\nRd\nSJFK\nRa\nSMCO\nE\n" +
            "E\nE\n" )
	end

	def test_spec_mapping_between_sequences
		# Complex key #1
		dj = Date.new( 2001, 7, 23 )
        complex = { [ 'Detroit Tigers', 'Chicago Cubs' ] => [ Date.new( 2001, 7, 23 ) ],
			  [ 'New York Yankees', 'Atlanta Braves' ] => [ Date.new( 2001, 7, 2 ), Date.new( 2001, 8, 12 ), Date.new( 2001, 8, 14 ) ] }
		assert_to_yaml(
			complex, <<EOY
? # PLAY SCHEDULE
  - Detroit Tigers
  - Chicago Cubs
:  
  - 2001-07-23

? [ New York Yankees,
    Atlanta Braves ]
: [ 2001-07-02, 2001-08-12, 
    2001-08-14 ]
EOY
		)

		# Complex key #2
		assert_to_yaml(
		  { [ 'New York Yankees', 'Atlanta Braves' ] =>
		    [ Date.new( 2001, 7, 2 ), Date.new( 2001, 8, 12 ),
			  Date.new( 2001, 8, 14 ) ],
			[ 'Detroit Tigers', 'Chicago Cubs' ] =>
			[ Date.new( 2001, 7, 23 ) ]
		  }, <<EOY
?
    - New York Yankees
    - Atlanta Braves
:
  - 2001-07-02
  - 2001-08-12
  - 2001-08-14
?
    - Detroit Tigers
    - Chicago Cubs
:
  - 2001-07-23
EOY
		)
	end

	def test_spec_sequence_key_shortcut
		# Shortcut sequence map
        seq = { 'invoice' => 34843, 'date' => Date.new( 2001, 1, 23 ),
		    'bill-to' => 'Chris Dumars', 'product' =>
			[ { 'item' => 'Super Hoop', 'quantity' => 1 },
			  { 'item' => 'Basketball', 'quantity' => 4 },
			  { 'item' => 'Big Shoes', 'quantity' => 1 } ] }
		assert_to_yaml(
		  seq, <<EOY
invoice: 34843
date   : 2001-01-23
bill-to: Chris Dumars
product:
  - item    : Super Hoop
    quantity: 1
  - item    : Basketball
    quantity: 4
  - item    : Big Shoes
    quantity: 1
EOY
		)

        # assert_bytecode( seq, "D\nM\nSinvoice\nS34843\nSdate\nS2001-01-03\nSbill-to\nSChris Dumars\nSproduct\n" + 
        #     "Q\nM\nSitem\nSSuper Hoop\nSquantity\nS1\nE\nM\nSitem\nSBasketball\nSquantity\nS4\nE\n" +
        #     "M\nSitem\nSBig Shoes\nSquantity\nS1\nE\nE\nE\n" )
	end

    def test_spec_sequence_in_sequence_shortcut
        # Seq-in-seq
        seq = [ [ [ 'one', 'two', 'three' ] ] ]
        assert_to_yaml( seq, <<EOY )
- - - one
    - two
    - three
EOY

        assert_bytecode( seq, "D\nQ\nQ\nQ\nSone\nStwo\nSthree\nE\nE\nE\n" )
    end

    def test_spec_sequence_shortcuts
        # Sequence shortcuts combined
        seq = [
          [ 
            [ [ 'one' ] ],
            [ 'two', 'three' ],
            { 'four' => nil },
            [ { 'five' => [ 'six' ] } ],
            [ 'seven' ]
          ],
          [ 'eight', 'nine' ]
        ]
        assert_to_yaml( seq, <<EOY )
- - - - one
  - - two
    - three
  - four:
  - - five:
      - six
  - - seven
- - eight
  - nine
EOY

        assert_bytecode( seq, "D\nQ\nQ\nQ\nQ\nSone\nE\nE\nQ\nStwo\nSthree\nE\n" +
            "M\nSfour\nS~\nE\nQ\nM\nSfive\nQ\nSsix\nE\nE\nE\n" +
            "Q\nSseven\nE\nE\nQ\nSeight\nSnine\nE\nE\n" )
    end

	def test_spec_single_literal
		# Literal scalar block
        lit = [ "\\/|\\/|\n/ |  |_\n" ]
		assert_to_yaml( lit, <<EOY )
- |
    \\/|\\/|
    / |  |_
EOY
        assert_bytecode( lit, "D\nQ\nS\\/|\\/|\nN\nC/ |  |_\nN\nE\n" )
	end

	def test_spec_single_folded
		# Folded scalar block
        fold = [ "Mark McGwire's year was crippled by a knee injury.\n" ]
		assert_to_yaml( fold, <<EOY )
- >
    Mark McGwire's
    year was crippled
    by a knee injury.
EOY

        # Force a few elaborate folded blocks
		assert_to_yaml( [ <<STR1, <<STR2, <<STR3 ], <<EOY )
Here's what you're going to need:


* Ruby 1.8.0 or higher.

** Linux/FreeBSD: "Get the latest, please.":http://ruby-lang.org/en/20020102.html

** Windows: "Installer for Windows.":http://rubyinstaller.sourceforge.net/

** Mac: "OSX disk image.":http://homepage.mac.com/discord/Ruby/

* Once Ruby is installed, open a command prompt and type:


<pre>
  ruby -ropen-uri -e 'eval(open("http://go.hobix.com/").read)'
</pre>

STR1
Ok, so the idea here is that one whole weblahhg is contained
in a single directory.  What is stored in the directory?


<pre>
  hobix.yaml <- configuration

  entries/   <- edit and organize
                your news items,
                articles and so on.

  skel/      <- contains your
                templates

  htdocs/    <- html is created here,
                store all your images here,
                this is your viewable
                websyht

  lib/       <- extra hobix libraries
                (plugins) go here
</pre>


One weblahhg can be shared with many authors.  In the @hobix.yaml@
file, you can store information about each author, as well as 
information about others who contribute to your websyht.


You also have a file of your own, a configuration file called
@.hobixrc@, which contains a list of the weblahhgs you belong
to.


h2. Pull One From the Sky


If you would like to create your own weblahhg from scratch:


<pre>
  hobix create blahhg
</pre>


You will be prompted for a full path where the new weblahhg can
be created.  Don't worry if the directory doesn't yet exist,
Hobix will take care of clearing a path for it.


Once you give it the path, Hobix will create all the necessary
directories, as well as the @hobix.yaml@.  You should also have
a set of sample templates to get you started.


In fact, if you want to generate the default site:


<pre>
  hobix regen blahhg
</pre>


h2. Joining Hands With Others


To join an existing weblahhg:


<pre>
  hobix add other-blahhg /path/to/other-blahhg/hobix.yaml
</pre>


You don't need to be on a weblahhg's author list to join the weblahhg.
You just need permissions to edit the file.


h2. Leaving in a Cloud of Keystrokes


To remove a weblahhg from your configuration:


<pre>
  hobix del other-blahhg
</pre>


Please don't be afraid to edit your configuration file yourself,
should the commandline not suit your style.


See, here's my @.hobixrc@:


<pre>
  --- 
  weblogs: 
    hobix: /usr/local/www/hobix.com/www/hobix.yaml
    why: /usr/local/www/whytheluckystiff.net/www/hobix.yaml
  username: why
  use editor: true
</pre>


That's a YAML file.  Very simple to edit.  You can manually edit
your information, safely add or edit your weblogs, and save back
to your @.hobixrc@.


Enough then.  Time to trick out your new Hoblahhg.
You will be guided through an automatic installation (or upgrade) of
Hobix.
STR2
NOW BUNKING together: the ruby and yaml scripts up at http://go.hobix.com/. Hey ,, the life and unity we feel is tree-mending-US!!

Try:

<pre>
  ruby -ropen-uri -e'eval open("http://go.hobix.com/").read'
</pre>

No longer are curtains dropping ON the actors' arms !!  No longer are swift currents ON the actors' legs !!  The actors have a bottomless cereal container, witness.
STR3
--- 
- |+
  Here's what you're going to need:


  * Ruby 1.8.0 or higher.

  ** Linux/FreeBSD: "Get the latest, please.":http://ruby-lang.org/en/20020102.html

  ** Windows: "Installer for Windows.":http://rubyinstaller.sourceforge.net/

  ** Mac: "OSX disk image.":http://homepage.mac.com/discord/Ruby/

  * Once Ruby is installed, open a command prompt and type:


  <pre>
    ruby -ropen-uri -e 'eval(open("http://go.hobix.com/").read)'
  </pre>

- |
  Ok, so the idea here is that one whole weblahhg is contained
  in a single directory.  What is stored in the directory?


  <pre>
    hobix.yaml <- configuration

    entries/   <- edit and organize
                  your news items,
                  articles and so on.

    skel/      <- contains your
                  templates

    htdocs/    <- html is created here,
                  store all your images here,
                  this is your viewable
                  websyht

    lib/       <- extra hobix libraries
                  (plugins) go here
  </pre>


  One weblahhg can be shared with many authors.  In the @hobix.yaml@
  file, you can store information about each author, as well as 
  information about others who contribute to your websyht.


  You also have a file of your own, a configuration file called
  @.hobixrc@, which contains a list of the weblahhgs you belong
  to.


  h2. Pull One From the Sky


  If you would like to create your own weblahhg from scratch:


  <pre>
    hobix create blahhg
  </pre>


  You will be prompted for a full path where the new weblahhg can
  be created.  Don't worry if the directory doesn't yet exist,
  Hobix will take care of clearing a path for it.


  Once you give it the path, Hobix will create all the necessary
  directories, as well as the @hobix.yaml@.  You should also have
  a set of sample templates to get you started.


  In fact, if you want to generate the default site:


  <pre>
    hobix regen blahhg
  </pre>


  h2. Joining Hands With Others


  To join an existing weblahhg:


  <pre>
    hobix add other-blahhg /path/to/other-blahhg/hobix.yaml
  </pre>


  You don't need to be on a weblahhg's author list to join the weblahhg.
  You just need permissions to edit the file.


  h2. Leaving in a Cloud of Keystrokes


  To remove a weblahhg from your configuration:


  <pre>
    hobix del other-blahhg
  </pre>


  Please don't be afraid to edit your configuration file yourself,
  should the commandline not suit your style.


  See, here's my @.hobixrc@:


  <pre>
    --- 
    weblogs: 
      hobix: /usr/local/www/hobix.com/www/hobix.yaml
      why: /usr/local/www/whytheluckystiff.net/www/hobix.yaml
    username: why
    use editor: true
  </pre>


  That's a YAML file.  Very simple to edit.  You can manually edit
  your information, safely add or edit your weblogs, and save back
  to your @.hobixrc@.


  Enough then.  Time to trick out your new Hoblahhg.
  You will be guided through an automatic installation (or upgrade) of
  Hobix.


- |
  NOW BUNKING together: the ruby and yaml scripts up at http://go.hobix.com/. Hey ,, the life and unity we feel is tree-mending-US!!

  Try:

  <pre>
    ruby -ropen-uri -e'eval open("http://go.hobix.com/").read'
  </pre>

  No longer are curtains dropping ON the actors' arms !!  No longer are swift currents ON the actors' legs !!  The actors have a bottomless cereal container, witness.

EOY

        assert_bytecode( fold, "D\nQ\nSMark McGwire's year was crippled by a knee injury.\nN\nE\n" )
	end

	def test_spec_preserve_indent
		# Preserve indented spaces
        fold = "Sammy Sosa completed another fine season with great stats.\n\n  63 Home Runs\n  0.288 Batting Average\n\nWhat a year!\n"
		assert_to_yaml( fold, <<EOY )
--- >
 Sammy Sosa completed another
 fine season with great stats.

   63 Home Runs
   0.288 Batting Average

 What a year!
EOY

        assert_bytecode( fold, "D\nSSammy Sosa completed another fine season with great stats.\nN\nN\nC  63 Home Runs\nN\nC  0.288 Batting Average\nN\nN\nCWhat a year!\nN\n" )
	end

	def test_spec_indentation_determines_scope
        map = { 'name' => 'Mark McGwire', 'accomplishment' => "Mark set a major league home run record in 1998.\n",
			  'stats' => "65 Home Runs\n0.278 Batting Average\n" }
		assert_to_yaml( map, <<EOY )
name: Mark McGwire
accomplishment: >
   Mark set a major league
   home run record in 1998.
stats: |
   65 Home Runs
   0.278 Batting Average
EOY
        assert_bytecode( map, "D\nM\nSname\nSMark McGwire\nSaccomplishment\nSMark set a major league home run record in 1998.\nN\nSstats\nS65 Home Runs\nN\nC0.278 Batting Average\nN\nE\n" )
	end

    #	def test_spec_quoted_scalars
    #		assert_to_yaml(
    #			{"tie-fighter"=>"|\\-*-/|", "control"=>"\0101998\t1999\t2000\n", "unicode"=>"Sosa did fine." + ["263A".hex].pack('U*'), "quoted"=>" # not a 'comment'.", "single"=>"\"Howdy!\" he cried.", "hexesc"=>"\r\n is \r\n"}, <<EOY
    #unicode: "Sosa did fine.\\u263A"
    #control: "\\b1998\\t1999\\t2000\\n"
    #hexesc:  "\\x0D\\x0A is \\r\\n"
    #
    #single: '"Howdy!" he cried.'
    #quoted: ' # not a ''comment''.'
    #tie-fighter: '|\\-*-/|'
    #EOY
    #		)
    #	end

	def test_spec_multiline_scalars
		# Multiline flow scalars
        map = { 'plain' => 'This unquoted scalar spans many lines.',
	 		  'quoted' => "So does this quoted scalar.\n" } 
	 	assert_to_yaml( map, <<EOY )
plain: This unquoted
       scalar spans
       many lines.
quoted: "\\
  So does this quoted
  scalar.\\n"
EOY
        assert_bytecode( map, "D\nM\nSplain\nSThis unquoted scalar spans many lines.\nSquoted\nSSo does this quoted scalar.\nN\nE\n" )
	end	

	def test_spec_type_int
        map = { 'canonical' => 12345, 'decimal' => 12345, 'octal' => '014'.oct, 'hexadecimal' => '0xC'.hex }
		assert_to_yaml( map, <<EOY )
canonical: 12345
decimal: +12,345
octal: 014
hexadecimal: 0xC
EOY
        assert_bytecode( map, "D\nM\nScanonical\nS12345\nSdecimal\nS+12,345\nSoctal\nS014\nShexadecimal\nS0xC\nE\n" )

        map = { 'canonical' => 685230, 'decimal' => 685230, 'octal' => '02472256'.oct, 'hexadecimal' => '0x0A74AE'.hex, 'sexagesimal' => 685230 }
		assert_to_yaml( map, <<EOY )
canonical: 685230
decimal: +685,230
octal: 02472256
hexadecimal: 0x0A,74,AE
sexagesimal: 190:20:30
EOY
        assert_bytecode( map, "D\nM\nScanonical\nS685230\nSdecimal\nS+685,230\nSoctal\nS02472256\nShexadecimal\nS0x0A,74,AE\nSsexagesimal\nS190:20:30\nE\n" )

	end

	def test_spec_type_float
        map = { 'canonical' => 1230.15, 'exponential' => 1230.15, 'fixed' => 1230.15,
			  'sexagecimal' => 1230.15, 'negative infinity' => -1.0/0.0 }
		assert_to_yaml( map, <<EOY )
canonical: 1.23015e+3
exponential: 12.3015e+02
fixed: 1,230.15
sexagecimal: 20:30.15
negative infinity: -.inf
EOY
		nan = YAML::load( <<EOY
not a number: .NaN
EOY
		)
		assert( nan['not a number'].nan? )

        assert_bytecode( map, "D\nM\nScanonical\nS1.23015e+3\nSexponential\nS12.3015e+02\nSfixed\nS1,230.15\nSsexagecimal\nS20:30.15\nSnegative infinity\nS-.inf\nE\n" )
	end

	def test_spec_type_misc
        map = { nil => nil, true => true, false => false, 'string' => '12345' }
		assert_to_yaml( map, <<EOY )
null: ~
true: yes
false: no
string: '12345'
EOY
        assert_bytecode( map, "D\nM\nSnull\nS~\nStrue\nSyes\nSfalse\nSno\nSstring\nTtag:yaml.org,2002:str\nS12345\nE\n" )
	end

	def test_spec_complex_invoice
		# Complex invoice type
		id001 = { 'given' => 'Chris', 'family' => 'Dumars', 'address' =>
			{ 'lines' => "458 Walkman Dr.\nSuite #292\n", 'city' => 'Royal Oak',
			  'state' => 'MI', 'postal' => 48046 } }
        invoice = { 'invoice' => 34843, 'date' => Date.new( 2001, 1, 23 ),
			  'bill-to' => id001, 'ship-to' => id001, 'product' =>
			  [ { 'sku' => 'BL394D', 'quantity' => 4,
			      'description' => 'Basketball', 'price' => 450.00 },
				{ 'sku' => 'BL4438H', 'quantity' => 1,
				  'description' => 'Super Hoop', 'price' => 2392.00 } ],
			  'tax' => 251.42, 'total' => 4443.52,
			  'comments' => "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.\n" }
		assert_to_yaml( invoice, <<EOY )
invoice: 34843
date   : 2001-01-23
bill-to: &id001
    given  : Chris
    family : !str Dumars
    address:
        lines: |
            458 Walkman Dr.
            Suite #292
        city    : Royal Oak
        state   : MI
        postal  : 48046
ship-to: *id001
product:
    - !map
      sku         : BL394D
      quantity    : 4
      description : Basketball
      price       : 450.00
    - sku         : BL4438H
      quantity    : 1
      description : Super Hoop
      price       : 2392.00
tax  : 251.42
total: 4443.52
comments: >
    Late afternoon is best.
    Backup contact is Nancy
    Billsmer @ 338-4338.
EOY
        assert_bytecode( invoice, "D\nM\nSinvoice\nS34843\nSdate\nS2001-01-23\nSbill-to\nAid001\n" +
            "M\nSgiven\nSChris\nSfamily\nT!str\nSDumars\nSaddress\n" +
            "M\nSlines\nS458 Walkman Dr.\nN\nCSuite #292\nN\nScity\nSRoyal Oak\nSstate\nSMI\nSpostal\nS48046\nE\nE\n" +
            "Sship-to\nRid001\nSproduct\nQ\n" +
            "T!map\nM\nSsku\nSBL394D\nSquantity\nS4\nSdescription\nSBasketball\nSprice\nS450.00\nE\n" +
            "M\nSsku\nSBL4438H\nSquantity\nS1\nSdescription\nSSuper Hoop\nSprice\nS2392.00\nE\nE\n" +
            "Stax\nS251.42\nStotal\nS4443.52\nScomments\n" + 
                "SLate afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.\nN\n" + 
            "E\n" )
	end

	def test_spec_log_file
		doc_ct = 0
        doc1 = { 'Time' => mktime( 2001, 11, 23, 15, 01, 42, 00, "-05:00" ),
			 	'User' => 'ed', 'Warning' => "This is an error message for the log file\n" }
        doc2 = { 'Time' => mktime( 2001, 11, 23, 15, 02, 31, 00, "-05:00" ),
				'User' => 'ed', 'Warning' => "A slightly different error message.\n" }
        doc3 = { 'Date' => mktime( 2001, 11, 23, 15, 03, 17, 00, "-05:00" ),
                'User' => 'ed', 'Fatal' => "Unknown variable \"bar\"\n",
                'Stack' => [
                    { 'file' => 'TopClass.py', 'line' => 23, 'code' => "x = MoreObject(\"345\\n\")\n" },
                    { 'file' => 'MoreClass.py', 'line' => 58, 'code' => "foo = bar" } ] }
		YAML::load_documents( <<EOY
---
Time: 2001-11-23 15:01:42 -05:00
User: ed
Warning: >
  This is an error message
  for the log file
---
Time: 2001-11-23 15:02:31 -05:00
User: ed
Warning: >
  A slightly different error
  message.
---
Date: 2001-11-23 15:03:17 -05:00
User: ed
Fatal: >
  Unknown variable "bar"
Stack:
  - file: TopClass.py
    line: 23
    code: |
      x = MoreObject("345\\n")
  - file: MoreClass.py
    line: 58
    code: |-
      foo = bar
EOY
		) { |doc|
			case doc_ct
				when 0
					assert_equals( doc, doc1 )
				when 1
					assert_equals( doc, doc2 )
				when 2
					assert_equals( doc, doc3 )
			end
			doc_ct += 1
		}
		assert_equals( doc_ct, 3 )

        doc_ct = 0
        yp = YAML::Parser.new
        yp.input = :bytecode
        yp.load_documents( 
            "D\nM\nSTime\nS2001-11-23 15:01:42 -05:00\nSUser\nSed\nSWarning\nSThis is an error message for the log file\nN\nE\n" +
            "D\nM\nSTime\nS2001-11-23 15:02:31 -05:00\nSUser\nSed\nSWarning\nSA slightly different error message.\nN\nE\n" +
            "D\nM\nSDate\nS2001-11-23 15:03:17 -05:00\nSUser\nSed\nSFatal\nSUnknown variable \"bar\"\nN\nSStack\n" +
                "Q\nM\nSfile\nSTopClass.py\nSline\nS23\nScode\nSx = MoreObject(\"345\\n\")\nN\nE\n" +
                    "M\nSfile\nSMoreClass.py\nSline\nS58\nScode\nSfoo = bar\nE\nE\nE\n"
		) { |doc|
			case doc_ct
				when 0
					assert_equals( doc, doc1 )
				when 1
					assert_equals( doc, doc2 )
				when 2
					assert_equals( doc, doc3 )
			end
			doc_ct += 1
		}
		assert_equals( doc_ct, 3 )

	end

	def test_spec_root_fold
		y = YAML::load( <<EOY
--- >
This YAML stream contains a single text value.
The next stream is a log file - a sequence of
log entries. Adding an entry to the log is a
simple matter of appending it at the end.
EOY
		)
		assert_equals( y, "This YAML stream contains a single text value. The next stream is a log file - a sequence of log entries. Adding an entry to the log is a simple matter of appending it at the end.\n" )
	end

	def test_spec_root_mapping
		y = YAML::load( <<EOY
# This stream is an example of a top-level mapping.
invoice : 34843
date    : 2001-01-23
total   : 4443.52
EOY
		)
		assert_equals( y, { 'invoice' => 34843, 'date' => Date.new( 2001, 1, 23 ), 'total' => 4443.52 } )
	end

    #	def test_spec_oneline_docs
    #		doc_ct = 0
    #		YAML::load_documents( <<EOY
    ## The following is a sequence of three documents.
    ## The first contains an empty mapping, the second
    ## an empty sequence, and the last an empty string.
    #--- {}
    #--- [ ]
    #--- ''
    #EOY
    #		) { |doc|
    #			case doc_ct
    #				when 0
    #					assert_equals( doc, {} )
    #				when 1
    #					assert_equals( doc, [] )
    #				when 2
    #					assert_equals( doc, '' )
    #			end
    #			doc_ct += 1
    #		}
    #		assert_equals( doc_ct, 3 )
    #	end

	def test_spec_domain_prefix
        customer_proc = proc { |type, val|
			if Hash === val
                scheme, domain, type = type.split( ':', 3 )
				val['type'] = "domain #{type}"
				val
			else
				raise ArgumentError, "Not a Hash in domain.tld,2002/invoice: " + val.inspect
			end
		}
		YAML.add_domain_type( "domain.tld,2002", 'invoice', &customer_proc ) 
		YAML.add_domain_type( "domain.tld,2002", 'customer', &customer_proc ) 
        map = { "invoice"=> { "customers"=> [ { "given"=>"Chris", "type"=>"domain customer", "family"=>"Dumars" } ], "type"=>"domain invoice" } }
		assert_to_yaml( map, <<EOY )
# 'http://domain.tld,2002/invoice' is some type family.
invoice: !domain.tld,2002/^invoice
  # 'seq' is shorthand for 'http://yaml.org/seq'.
  # This does not effect '^customer' below
  # because it is does not specify a prefix.
  customers: !seq
    # '^customer' is shorthand for the full
    # notation 'http://domain.tld,2002/customer'.
    - !^customer
      given : Chris
      family : Dumars
EOY
        assert_bytecode( map, "D\nc 'http://domain.tld,2002/invoice' is some type family.\n" + 
            "M\nSinvoice\nT!domain.tld,2002/^invoice\nc 'seq' is shorthand for 'http://yaml.org/seq'.\n" + 
            "c This does not effect '^customer' below\nc because it is does not specify a prefix.\n" +
            "M\nScustomers\nT!seq\nc '^customer' is shorthand for the full\nc notation 'http://domain.tld,2002/customer'.\n" +
            "Q\nT!^customer\nM\nSgiven\nSChris\nSfamily\nSDumars\nE\nE\nE\n" )
	end

	def test_spec_throwaway
        map = {"this"=>"contains three lines of text.\nThe third one starts with a\n# character. This isn't a comment.\n"}
		assert_to_yaml( map, <<EOY )
### These are four throwaway comment  ###

### lines (the second line is empty). ###
this: |   # Comments may trail lines.
    contains three lines of text.
    The third one starts with a
    # character. This isn't a comment.

# These are three throwaway comment
# lines (the first line is empty).
EOY
        assert_bytecode( map, "D\nc These are four throwaway comment\nc\nc lines (the second line is empty).\n" +
            "M\nSthis\nc Comments may trail lines.\nScontains three lines of text.\nN\nCThe third one starts with a\nN\n" +
            "C# character. This isn't a comment.\nN\n" + 
            "c These are three throwaway comment\nc lines (the first line is empty).\nE\n" )
	end

	def test_spec_force_implicit
		# Force implicit
        map = { 'integer' => 12, 'also int' => 12, 'string' => '12' }
		assert_to_yaml( map, <<EOY )
integer: 12
also int: ! "12"
string: !str 12
EOY
	end

    #	def test_spec_private_types
    #		doc_ct = 0
    #		YAML::parse_documents( <<EOY
    ## Private types are per-document.
    #---
    #pool: !!ball
    #   number: 8
    #   color: black
    #---
    #bearing: !!ball
    #   material: steel
    #EOY
    #		) { |doc|
    #			case doc_ct
    #				when 0
    #					assert_equals( doc['pool'].type_id, 'x-private:ball' )
    #					assert_equals( doc['pool'].transform.value, { 'number' => 8, 'color' => 'black' } )
    #				when 1
    #					assert_equals( doc['bearing'].type_id, 'x-private:ball' ) 
    #					assert_equals( doc['bearing'].transform.value, { 'material' => 'steel' } )
    #			end
    #			doc_ct += 1
    #		}
    #		assert_equals( doc_ct, 2 )
    #
    #		doc_ct = 0
    #		YAML::Syck::Parser.new( :Input => :Bytecode, :Model => :Generic )::load_documents( 
    #            "D\nc Private types are per-document.\nM\nSpool\nT!!ball\n" +
    #                "M\nSnumber\nS8\nScolor\nSblack\nE\nE\n" +
    #            "D\nM\nSbearing\nT!!ball\nM\nSmaterial\nSsteel\nE\nE\n"
    #		) { |doc|
    #			case doc_ct
    #				when 0
    #					assert_equals( doc['pool'].type_id, 'x-private:ball' )
    #					assert_equals( doc['pool'].transform.value, { 'number' => 8, 'color' => 'black' } )
    #				when 1
    #					assert_equals( doc['bearing'].type_id, 'x-private:ball' ) 
    #					assert_equals( doc['bearing'].transform.value, { 'material' => 'steel' } )
    #			end
    #			doc_ct += 1
    #		}
    #		assert_equals( doc_ct, 2 )
    #	end

	def test_spec_url_escaping
		YAML.add_domain_type( "domain.tld,2002", "type0" ) { |type, val|
			"ONE: #{val}"
		}
		YAML.add_domain_type( "domain.tld,2002", "type%30" ) { |type, val|
			"TWO: #{val}"
		}
        map = { 'same' => [ 'ONE: value', 'ONE: value' ], 'different' => [ 'TWO: value' ] }
		assert_to_yaml( map, <<EOY )
same:
  - !domain.tld,2002/type\\x30 value
  - !domain.tld,2002/type0 value
different: # As far as the YAML parser is concerned
  - !domain.tld,2002/type%30 value
EOY
        assert_bytecode( map, "D\nM\nSsame\nQ\nT!domain.tld,2002/type\x30\nSvalue\nT!domain.tld,2002/type0\nSvalue\nE\n" +
            "Sdifferent\nc As far as the YAML parser is concerned\nQ\nT!domain.tld,2002/type%30\nSvalue\nE\nE\n" )
	end

	def test_spec_override_anchor
		# Override anchor
		a001 = "The alias node below is a repeated use of this value.\n"
        anc = { 'anchor' => 'This scalar has an anchor.', 'override' => a001, 'alias' => a001 }
		assert_to_yaml( anc, <<EOY )
anchor : &A001 This scalar has an anchor.
override : &A001 >
 The alias node below is a
 repeated use of this value.
alias : *A001
EOY
        assert_bytecode( anc, "D\nM\nSanchor\nAA001\nSThis scalar has an anchor.\nSoverride\nAA001\n" +
            "SThe alias node below is a repeated use of this value.\nN\nSalias\nRA001\nE\n" )
	end

	def test_spec_explicit_families
		YAML.add_domain_type( "somewhere.com,2002", 'type' ) { |type, val|
			"SOMEWHERE: #{val}"
		}
        map = { 'not-date' => '2002-04-28', 'picture' => "GIF89a\f\000\f\000\204\000\000\377\377\367\365\365\356\351\351\345fff\000\000\000\347\347\347^^^\363\363\355\216\216\216\340\340\340\237\237\237\223\223\223\247\247\247\236\236\236i^\020' \202\n\001\000;", 'hmm' => "SOMEWHERE: family above is short for\nhttp://somewhere.com/type\n" }
		assert_to_yaml( map, <<EOY )
not-date: !str 2002-04-28
picture: !binary |
 R0lGODlhDAAMAIQAAP//9/X
 17unp5WZmZgAAAOfn515eXv
 Pz7Y6OjuDg4J+fn5OTk6enp
 56enmleECcgggoBADs=

hmm: !somewhere.com,2002/type | 
 family above is short for
 http://somewhere.com/type
EOY
        assert_bytecode( map, "D\nM\nSnot-date\nT!str\nS2002-04-28\nSpicture\nT!binary\n" +
            "SR0lGODlhDAAMAIQAAP//9/X\nN\nC17unp5WZmZgAAAOfn515eXv\nN\nCPz7Y6OjuDg4J+fn5OTk6enp\nN\n" +
            "C56enmleECcgggoBADs=\nN\nShmm\nT!somewhere.com,2002/type\nSfamily above is short for\nN\n" +
            "Chttp://somewhere.com/type\nN\nE\n" )
	end

	def test_spec_application_family
		# Testing the clarkevans.com graphs
		YAML.add_domain_type( "clarkevans.com,2002", 'graph/shape' ) { |type, val|
			if Array === val
				val << "Shape Container"
				val
			else
				raise ArgumentError, "Invalid graph of type #{val.class}: " + val.inspect
			end
		}
		one_shape_proc = Proc.new { |type, val|
			if Hash === val
                type = type.split( /:/ )
				val['TYPE'] = "Shape: #{type[2]}"
				val
			else
				raise ArgumentError, "Invalid graph of type #{val.class}: " + val.inspect
			end
		}
		YAML.add_domain_type( "clarkevans.com,2002", 'graph/circle', &one_shape_proc )
		YAML.add_domain_type( "clarkevans.com,2002", 'graph/line', &one_shape_proc )
		YAML.add_domain_type( "clarkevans.com,2002", 'graph/text', &one_shape_proc )
        seq = [[{"radius"=>7, "center"=>{"x"=>73, "y"=>129}, "TYPE"=>"Shape: graph/circle"}, {"finish"=>{"x"=>89, "y"=>102}, "TYPE"=>"Shape: graph/line", "start"=>{"x"=>73, "y"=>129}}, {"TYPE"=>"Shape: graph/text", "value"=>"Pretty vector drawing.", "start"=>{"x"=>73, "y"=>129}, "color"=>16772795}, "Shape Container"]]
		assert_to_yaml( seq, <<EOY )
- !clarkevans.com,2002/graph/^shape
  - !^circle
    center: &ORIGIN {'x': 73, 'y': 129}
    radius: 7
  - !^line # !clarkevans.com,2002/graph/line
    start: *ORIGIN
    finish: { 'x': 89, 'y': 102 }
  - !^text
    start: *ORIGIN
    color: 0xFFEEBB
    value: Pretty vector drawing.
EOY
        assert_bytecode( seq, "D\nQ\nT!clarkevans.com,2002/graph/^shape\nQ\n" +
            "T!^circle\nM\nScenter\nAORIGIN\nM\nSx\nS73\nTstr\nSy\nS129\nE\nSradius\nS7\nE\n" +
            "T!^line\nc !clarkevans.com,2002/graph/line\nM\nSstart\nRORIGIN\nSfinish\nM\nSx\nS89\nTstr\nSy\nS102\nE\nE\n" +
            "T!^text\nM\nSstart\nRORIGIN\nScolor\nS0xFFEEBB\nSvalue\nSPretty vector drawing.\nE\nE\nE\n" )
	end

	def test_spec_float_explicit
        seq = [ 10.0, 10.0, 10.0, 10.0 ]
		assert_to_yaml( seq, <<EOY )
# All entries in the sequence
# have the same type and value.
- 10.0
- !float 10
- !yaml.org,2002/^float '10'
- !yaml.org,2002/float "\\
  1\\
  0"
EOY
        assert_bytecode( seq, "D\nc All entries in the sequence\nc have the same type and value\n" +
            "Q\nS10.0\nT!float\nS10\nT!yaml.org,2002/^float\nS10\nT!yaml.org,2002/float\nS10\nE\n" )
	end

	def test_spec_builtin_seq
		# Assortment of sequences
		assert_to_yaml(
			{ 'empty' => [], 'in-line' => [ 'one', 'two', 'three', 'four', 'five' ],
			  'nested' => [ 'First item in top sequence', [ 'Subordinate sequence entry' ],
			  	"A multi-line sequence entry\n", 'Sixth item in top sequence' ] }, <<EOY
empty: []
in-line: [ one, two, three # May span lines,
         , four,           # indentation is
           five ]          # mostly ignored.
nested:
 - First item in top sequence
 -
  - Subordinate sequence entry
 - >
   A multi-line
   sequence entry
 - Sixth item in top sequence
EOY
		)
	end

	def test_spec_builtin_map
		# Assortment of mappings
		assert_to_yaml( 
			{ 'empty' => {}, 'in-line' => { 'one' => 1, 'two' => 2 },
			  'spanning' => { 'one' => 1, 'two' => 2 },
			  'nested' => { 'first' => 'First entry', 'second' =>
			  	{ 'key' => 'Subordinate mapping' }, 'third' =>
				  [ 'Subordinate sequence', {}, 'Previous mapping is empty.',
					{ 'A key' => 'value pair in a sequence.', 'A second' => 'key:value pair.' },
					'The previous entry is equal to the following one.',
					{ 'A key' => 'value pair in a sequence.', 'A second' => 'key:value pair.' } ],
				  12.0 => 'This key is a float.', "?\n" => 'This key had to be protected.',
				  "\a" => 'This key had to be escaped.',
				  "This is a multi-line folded key\n" => "Whose value is also multi-line.\n",
				  [ 'This key', 'is a sequence' ] => [ 'With a sequence value.' ] } }, <<EOY

empty: {}
in-line: { one: 1, two: 2 }
spanning: { one: 1,
   two: 2 }
nested:
 first : First entry
 second:
  key: Subordinate mapping
 third:
  - Subordinate sequence
  - { }
  - Previous mapping is empty.
  - A key: value pair in a sequence.
    A second: key:value pair.
  - The previous entry is equal to the following one.
  -
    A key: value pair in a sequence.
    A second: key:value pair.
 !float 12 : This key is a float.
 ? >
  ?
 : This key had to be protected.
 "\\a" : This key had to be escaped.
 ? >
   This is a
   multi-line
   folded key
 : >
   Whose value is
   also multi-line.
 ?
  - This key
  - is a sequence
 :
  - With a sequence value.
# The following parses correctly,
# but Ruby 1.6.* fails the comparison!
# ?
#  This: key
#  is a: mapping
# :
#  with a: mapping value.
EOY
		)
	end

	def test_spec_builtin_literal_blocks
		# Assortment of literal scalar blocks
		assert_to_yaml(
			{"both are equal to"=>"  This has no newline.", "is equal to"=>"The \\ ' \" characters may be\nfreely used. Leading white\n   space is significant.\n\nLine breaks are significant.\nThus this value contains one\nempty line and ends with a\nsingle line break, but does\nnot start with one.\n", "also written as"=>"  This has no newline.", "indented and chomped"=>"  This has no newline.", "empty"=>"", "literal"=>"The \\ ' \" characters may be\nfreely used. Leading white\n   space is significant.\n\nLine breaks are significant.\nThus this value contains one\nempty line and ends with a\nsingle line break, but does\nnot start with one.\n"}, <<EOY
empty: |

literal: |
 The \\ ' " characters may be
 freely used. Leading white
    space is significant.

 Line breaks are significant.
 Thus this value contains one
 empty line and ends with a
 single line break, but does
 not start with one.

is equal to: "The \\ \' \\" characters may \\
 be\\nfreely used. Leading white\\n   space \\
 is significant.\\n\\nLine breaks are \\
 significant.\\nThus this value contains \\
 one\\nempty line and ends with a\\nsingle \\
 line break, but does\\nnot start with one.\\n"

# Comments may follow a nested
# scalar value. They must be
# less indented.

# Modifiers may be combined in any order.
indented and chomped: |2-
    This has no newline.

also written as: |-2
    This has no newline.

both are equal to: "  This has no newline."
EOY
		)

		str1 = "This has one newline.\n"
		str2 = "This has no newline."
		str3 = "This has two newlines.\n\n"
		assert_to_yaml( 
			{ 'clipped' => str1, 'same as "clipped" above' => str1, 
			  'stripped' => str2, 'same as "stripped" above' => str2, 
			  'kept' => str3, 'same as "kept" above' => str3 }, <<EOY
clipped: |
    This has one newline.

same as "clipped" above: "This has one newline.\\n"

stripped: |-
    This has no newline.

same as "stripped" above: "This has no newline."

kept: |+
    This has two newlines.

same as "kept" above: "This has two newlines.\\n\\n"

EOY
		)
	end
	
	def test_spec_span_single_quote
		assert_to_yaml( {"third"=>"a single quote ' must be escaped.", "second"=>"! : \\ etc. can be used freely.", "is same as"=>"this contains six spaces\nand one line break", "empty"=>"", "span"=>"this contains six spaces\nand one line break"}, <<EOY
empty: ''
second: '! : \\ etc. can be used freely.'
third: 'a single quote '' must be escaped.'
span: 'this contains
      six spaces
      
      and one
      line break'
is same as: "this contains six spaces\\nand one line break"
EOY
		)
	end

	def test_spec_span_double_quote
		assert_to_yaml( {"is equal to"=>"this contains four  spaces", "third"=>"a \" or a \\ must be escaped.", "second"=>"! : etc. can be used freely.", "empty"=>"", "fourth"=>"this value ends with an LF.\n", "span"=>"this contains four  spaces"}, <<EOY
empty: ""
second: "! : etc. can be used freely."
third: "a \\\" or a \\\\ must be escaped."
fourth: "this value ends with an LF.\\n"
span: "this contains
  four  \\
      spaces"
is equal to: "this contains four  spaces"
EOY
		)
	end

	def test_spec_builtin_time
		# Time
		assert_to_yaml(
			{ "space separated" => mktime( 2001, 12, 14, 21, 59, 43, ".10", "-05:00" ), 
			  "canonical" => mktime( 2001, 12, 15, 2, 59, 43, ".10" ), 
			  "date (noon UTC)" => Date.new( 2002, 12, 14), 
			  "valid iso8601" => mktime( 2001, 12, 14, 21, 59, 43, ".10", "-05:00" ) }, <<EOY
canonical: 2001-12-15T02:59:43.1Z
valid iso8601: 2001-12-14t21:59:43.10-05:00
space separated: 2001-12-14 21:59:43.10 -05:00
date (noon UTC): 2002-12-14
EOY
		)
	end

	def test_spec_builtin_binary
		arrow_gif = "GIF89a\f\000\f\000\204\000\000\377\377\367\365\365\356\351\351\345fff\000\000\000\347\347\347^^^\363\363\355\216\216\216\340\340\340\237\237\237\223\223\223\247\247\247\236\236\236iiiccc\243\243\243\204\204\204\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371\377\376\371!\376\016Made with GIMP\000,\000\000\000\000\f\000\f\000\000\005,  \216\2010\236\343@\024\350i\020\304\321\212\010\034\317\200M$z\357\3770\205p\270\2601f\r\e\316\001\303\001\036\020' \202\n\001\000;"
		assert_to_yaml(
			{ 'canonical' => arrow_gif, 'base64' => arrow_gif, 
			  'description' => "The binary value above is a tiny arrow encoded as a gif image.\n" }, <<EOY
canonical: !binary "\\
 R0lGODlhDAAMAIQAAP//9/X17unp5WZmZgAAAOf\\
 n515eXvPz7Y6OjuDg4J+fn5OTk6enp56enmlpaW\\
 NjY6Ojo4SEhP/++f/++f/++f/++f/++f/++f/++\\
 f/++f/++f/++f/++f/++f/++f/++SH+Dk1hZGUg\\
 d2l0aCBHSU1QACwAAAAADAAMAAAFLCAgjoEwnuN\\
 AFOhpEMTRiggcz4BNJHrv/zCFcLiwMWYNG84Bww\\
 EeECcgggoBADs="
base64: !binary |
 R0lGODlhDAAMAIQAAP//9/X17unp5WZmZgAAAOf
 n515eXvPz7Y6OjuDg4J+fn5OTk6enp56enmlpaW
 NjY6Ojo4SEhP/++f/++f/++f/++f/++f/++f/++
 f/++f/++f/++f/++f/++f/++f/++SH+Dk1hZGUg
 d2l0aCBHSU1QACwAAAAADAAMAAAFLCAgjoEwnuN
 AFOhpEMTRiggcz4BNJHrv/zCFcLiwMWYNG84Bww
 EeECcgggoBADs=
description: >
 The binary value above is a tiny arrow
 encoded as a gif image.
EOY
		)
	end
	def test_ruby_regexp
		# Test Ruby regular expressions
		assert_to_yaml( 
			{ 'simple' => /a.b/, 'complex' => /\A"((?:[^"]|\")+)"/,
			  'case-insensitive' => /George McFly/i }, <<EOY
case-insensitive: !ruby/regexp "/George McFly/i"
complex: !ruby/regexp "/\\\\A\\"((?:[^\\"]|\\\\\\")+)\\"/"
simple: !ruby/regexp "/a.b/"
EOY
		)
	end

	def test_ruby_struct
		# Ruby structures
		book_struct = Struct::new( "BookStruct", :author, :title, :year, :isbn )
		assert_to_yaml(
			[ book_struct.new( "Yukihiro Matsumoto", "Ruby in a Nutshell", 2002, "0-596-00214-9" ),
			  book_struct.new( [ 'Dave Thomas', 'Andy Hunt' ], "The Pickaxe", 2002, 
				book_struct.new( "This should be the ISBN", "but I have another struct here", 2002, "None" ) 
			  ) ], <<EOY
- !ruby/struct:BookStruct
  author: Yukihiro Matsumoto
  title: Ruby in a Nutshell
  year: 2002
  isbn: 0-596-00214-9
- !ruby/struct:BookStruct
  author:
    - Dave Thomas
    - Andy Hunt
  title: The Pickaxe
  year: 2002
  isbn: !ruby/struct:BookStruct
    author: This should be the ISBN
    title: but I have another struct here
    year: 2002
    isbn: None
EOY
		)

        assert_to_yaml( YAML_Tests::StructTest.new( 123 ), <<EOY )
--- !ruby/struct:YAML_Tests::StructTest
c: 123
EOY

	end

	def test_emitting_indicators
		assert_to_yaml( "Hi, from Object 1. You passed: please, pretty please", <<EOY
--- "Hi, from Object 1. You passed: please, pretty please"
EOY
		)
	end

	#
	# Test the YAML::Stream class -- INACTIVE at the moment
	#
	def test_document
		y = YAML::Stream.new( :Indent => 2, :UseVersion => 0 )
		y.add( 
			{ 'hi' => 'hello', 'map' =>
				{ 'good' => 'two' },
			  'time' => Time.now,
			  'try' => /^po(.*)$/,
			  'bye' => 'goodbye'
			}
		)
		y.add( { 'po' => 'nil', 'oper' => 90 } )
		y.add( { 'hi' => 'wow!', 'bye' => 'wow!' } )
		y.add( { [ 'Red Socks', 'Boston' ] => [ 'One', 'Two', 'Three' ] } )
		y.add( [ true, false, false ] )
	end

    #
    # Test YPath choices parsing
    #
    def test_ypath_parsing
        assert_path_segments( "/*/((one|three)/name|place)|//place",
          [ ["*", "one", "name"],
            ["*", "three", "name"],
            ["*", "place"],
            ["/", "place"] ]
        )
    end

    #
    # Test of Ranges
    #
    def test_ranges

        # Simple numeric
        assert_to_yaml( 1..3, <<EOY )
--- !ruby/range 1..3
EOY

        # Simple alphabetic
        assert_to_yaml( 'a'..'z', <<EOY )
--- !ruby/range a..z
EOY

        # Float
        assert_to_yaml( 10.5...30.3, <<EOY )
--- !ruby/range 10.5...30.3
EOY

    end

    #
    # Tests from Tanaka Akira on [ruby-core]
    #
    def test_akira

        # Commas in plain scalars [ruby-core:1066]
        assert_to_yaml(
            {"A"=>"A,","B"=>"B"}, <<EOY
A: "A,"
B: B
EOY
        )

        # Double-quoted keys [ruby-core:1069]
        assert_to_yaml(
            {"1"=>2, "2"=>3}, <<EOY
'1': 2
"2": 3
EOY
        )

        # Anchored mapping [ruby-core:1071]
        assert_to_yaml(
            [{"a"=>"b"}] * 2, <<EOY
- &id001
  a: b
- *id001
EOY
        )

        # Stress test [ruby-core:1071]
        # a = []; 1000.times { a << {"a"=>"b", "c"=>"d"} }
        # YAML::load( a.to_yaml )

    end

    #
    # Test Time.now cycle
    #
    def test_time_now_cycle
        #
        # From Minero Aoki [ruby-core:2305]
        #
        require 'yaml'
        t = Time.now
        5.times do
            assert_equals( t, YAML.load( YAML.dump( t ) ) )
        end
    end

    #    #
    #    # Circular references
    #    #
    #    def test_circular_references
    #        a = []; a[0] = a; a[1] = a
    #        inspect_str = "[[...], [...]]"
    #        assert_equals( inspect_str, YAML::load( a.to_yaml ).inspect )
    #    end
    
    #
    # Test Exception
    #
    def test_exception
        e1 = Exception.new( "hello" )
        e1.set_backtrace( ["file1.rb", "file2.rb"] )
        e2 = YAML.load( YAML.dump( e1 ))
        
        if RUBY_VERSION.match( /^1\.[012345678]/ )
            # Ruby 1.8 uses Object.== for Exception.==, so is not valid
            assert_equals( e1.class, e2.class )
            assert_equals( e1.backtrace, e2.backtrace )
            assert_equals( e1.message, e2.message )
        else
            # Ruby 1.9 and above
            assert_equals( e1, e2 )
        end
    end
end

RUNIT::CUI::TestRunner.run( YAML_Unit_Tests.suite )

#
# YTS.c.rb
#
# $Author$
# $Date$
#
# Copyright (C) 2004 why the lucky stiff
#
# This Ruby script generates the YTS suite for the
# Syck base lib.  Basically, it searches ext/ruby/yts/ for
# tests with a 'syck' entry.
#
# To regenerate things yourself:
#
#   ruby YTS.c.rb > YTS.c
#
# Oh and your Ruby must have YAML installed.
#
require 'erb'
require 'yaml'

# Find the Syck directory.
yts_dir = "ext/ruby/yts/"
syck_dir = ""
while File.expand_path( syck_dir ) != "/"
    break if File.directory?( syck_dir + yts_dir )
    syck_dir << "../"
end
yts_dir = syck_dir + yts_dir
abort "No YTS directory found" unless File.directory?( yts_dir )

# Load the YTS 
syck_tests = []
YAML::load( File.open( yts_dir + "index.yml" ) ).each do |yst|
    ct = 0
	YAML.each_document( File.open( yts_dir + yst + ".yml" ) ) do |ydoc|
        ydoc['group'] = yst
        ydoc['func'] = "#{ ydoc['group'] }_#{ ct }"
        syck_tests << ydoc if ydoc['syck']
        ct += 1
    end
end

puts ERB.new( File.read( syck_dir + "tests/YTS.c.erb" ), 0, "%<>" ).result

import syck
import unittest

class TestCase(unittest.TestCase):
    def parseOnly(self, obj, yaml):
        self.assertEqual(obj, syck.load(yaml))
    def roundTrip(self, obj, yaml):
        self.assertEqual(obj, syck.load(syck.dump(yaml)))
        self.assertEqual(obj, syck.load(yaml))

class BasicTests(TestCase):
    def testBasicMap(self):
        self.roundTrip(
            { 'one': 'foo', 'three': 'baz', 'two': 'bar' }, """
one: foo
two: bar
three: baz
""" 
        )

    def testBasicStrings(self):
        self.parseOnly(
            { 1: 'simple string', 2: 42, 3: '1 Single Quoted String',
              4: 'YAML\'s Double "Quoted" String', 5: "A block\n  with several\n    lines.\n",
              6: 'A "chomped" block', 7: "A folded\n string\n" }, """
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
"""
        )

    def testSimpleSequence(self):
        self.parseOnly(
            [ 'Mark McGwire', 'Sammy Sosa', 'Ken Griffey' ], """
- Mark McGwire
- Sammy Sosa
- Ken Griffey
"""
        )

    def testSimpleMap(self):
        self.parseOnly(
            { 'hr': 65, 'avg': 0.278, 'rbi': 147 }, """
avg: 0.278
hr: 65
rbi: 147
"""
        )

    def testSimpleMapWithNestedSequences(self):
        self.parseOnly(
            { 'american':
                [ 'Boston Red Sox', 'Detroit Tigers', 'New York Yankees' ],
              'national':
                [ 'New York Mets', 'Chicago Cubs', 'Atlanta Braves' ] }, """
american:
  - Boston Red Sox
  - Detroit Tigers
  - New York Yankees
national:
  - New York Mets
  - Chicago Cubs
  - Atlanta Braves
"""
        )

    def testSimpleSequenceWithNestedMaps(self):
        self.parseOnly(
            [ { 'name': 'Mark McGwire', 'hr': 65, 'avg': 0.278 },
              { 'name': 'Sammy Sosa', 'hr': 63, 'avg': 0.288} ], """
-
  avg: 0.278
  hr: 65
  name: Mark McGwire
-
  avg: 0.288
  hr: 63
  name: Sammy Sosa
"""
        )

    def testSequenceOfSequences(self):
        self.parseOnly(
		  [ 
		  	[ 'name', 'hr', 'avg' ],
			[ 'Mark McGwire', 65, 0.278 ],
			[ 'Sammy Sosa', 63, 0.288 ]
		  ], """
- [ name         , hr , avg   ]
- [ Mark McGwire , 65 , 0.278 ]
- [ Sammy Sosa   , 63 , 0.288 ]
"""
        )

    def testMappingOfMappings(self):
        self.parseOnly(
		  { 'Mark McGwire':
		    { 'hr': 65, 'avg': 0.278 },
			'Sammy Sosa':
		    { 'hr': 63, 'avg': 0.288 }
		  }, """
Mark McGwire: {hr: 65, avg: 0.278}
Sammy Sosa:   {hr: 63,
               avg: 0.288}
"""
        )

    def testNestedComments(self):
        self.parseOnly(
		  { 'hr': [ 'Mark McGwire', 'Sammy Sosa' ],
		    'rbi': [ 'Sammy Sosa', 'Ken Griffey' ] }, """
hr: # 1998 hr ranking
  - Mark McGwire
  - Sammy Sosa
rbi:
  # 1998 rbi ranking
  - Sammy Sosa
  - Ken Griffey
"""
        )

    def testAnchorsAndAliases(self):
        self.parseOnly(
            { 'hr':
			  [ 'Mark McGwire', 'Sammy Sosa' ],
              'rbi':
			  [ 'Sammy Sosa', 'Ken Griffey' ] }, """
hr:
   - Mark McGwire
   # Name "Sammy Sosa" scalar SS
   - &SS Sammy Sosa
rbi:
   # So it can be referenced later.
   - *SS
   - Ken Griffey
"""
        )

    def testSpecSequenceKeyShortcut(self):
        self.parseOnly(
		  { 'invoice': 34843, 'date': '2001-01-23',
		    'bill-to': 'Chris Dumars', 'product':
			[ { 'item': 'Super Hoop', 'quantity': 1 },
			  { 'item': 'Basketball', 'quantity': 4 },
			  { 'item': 'Big Shoes', 'quantity': 1 } ] }, """
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
"""
        )

    def testSpecSequenceKeyEmpties(self):
        self.parseOnly(
            [
              [ 
                [ [ 'one' ] ],
                [ 'two', 'three' ],
                { 'four': None },
                [ { 'five': [ 'six' ] } ],
                [ 'seven' ]
              ],
              [ 'eight', 'nine' ]
            ], """
- - - - one
  - - two
    - three
  - four:
  - - five:
      - six
  - - seven
- - eight
  - nine
"""
        )

    def testSeqInSeqShortcut(self):
        self.parseOnly(
            [ [ [ 'one', 'two', 'three' ] ] ], """
- - - one
    - two
    - three
"""
        )

    def testSpecShortcuts2(self):
        self.parseOnly(
            ['This sequence is not indented.', {'map-in-seq': 'further indented by four.', 'this key': 'is also further indented by four.'}, ['seq-in-seq; further indented by three.', 'second entry in nested sequence.'], 'Last entry in top sequence.'], """
- This sequence is not indented.
-   map-in-seq: further indented by four.
    this key: is also further indented by four.
-  - seq-in-seq; further indented by three.
   -    second entry in nested sequence.
- Last entry in top sequence.
"""
        )

    def testSingleLiteral(self):
        self.parseOnly(
		    [ "\\/|\\/|\n/ |  |_\n" ], """
- |
    \\/|\\/|
    / |  |_
"""
        )

    def testSingleFolded(self):
        self.parseOnly(
			[ "Mark McGwire's year was crippled by a knee injury.\n" ], """
- >
    Mark McGwire's
    year was crippled
    by a knee injury.
"""
        )

    def testIndentationScope(self):
        self.parseOnly(
			{ 'name': 'Mark McGwire', 'accomplishment': "Mark set a major league home run record in 1998.\n",
			  'stats': "65 Home Runs\n0.278 Batting Average\n" }, """
name: Mark McGwire
accomplishment: >
   Mark set a major league
   home run record in 1998.
stats: |
   65 Home Runs
   0.278 Batting Average
"""
        )

    def testTypeInt(self):
        self.parseOnly(
			{ 'canonical': 12345, 'decimal': 12345, 'octal': 12, 'hexadecimal': 12 }, """
canonical: 12345
decimal: +12,345
octal: 014
hexadecimal: 0xC
"""
        )

    def testTypeFloat(self):
        self.parseOnly(
			{ 'canonical': 1230.15, 'exponential': 1230.15, 'fixed': 1230.15 }, """
canonical: 1.23015e+3
exponential: 12.3015e+02
fixed: 1,230.15
"""
        )

    def testTypeMisc(self):
        self.parseOnly(
			{ None: None, 1: 1, 0: 0, 'string': '12345' }, """
null: ~
true: yes
false: no
string: '12345'
"""
        )

    def testDocComments(self):
        self.parseOnly(
			[1, 2, 3], """
# This is a test of root-level comments
# surrounding a document
---
# We want comments to be
- 1
# Interspersed in the
- 2
# Sequence merely for testing.
- 3
"""
        )

    def testDocComments2(self):
        self.parseOnly(
            {"this": "contains three lines of text.\nThe third one starts with a\n# character. This isn't a comment.\n", "and": "okay"}, """
### These are four throwaway comment  ###

### lines (the second line is empty). ###
and: okay   # no way!
this: |     # Comments may trail lines.
    contains three lines of text.
    The third one starts with a
    # character. This isn't a comment.

# These are three throwaway comment
# lines (the first line is empty).
"""
        )

    def testExampleInvoice(self):
        id001 = { 'given': 'Chris', 'family': 'Dumars', 'address':
            { 'lines': "458 Walkman Dr.\nSuite #292\n", 'city': 'Royal Oak',
              'state': 'MI', 'postal': 48046 } }
        self.parseOnly(
            { 'invoice': 34843, 'date': '2001-01-23',
              'bill-to': id001, 'ship-to': id001, 'product':
              [ { 'sku': 'BL394D', 'quantity': 4,
                  'description': 'Basketball', 'price': 450.00 },
                { 'sku': 'BL4438H', 'quantity': 1,
                  'description': 'Super Hoop', 'price': 2392.00 } ],
              'tax': 251.42, 'total': 4443.52,
              'comments': "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.\n" }, """
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
    - sku         : BL394D
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
"""
        )

    def testRootBlock(self):
        self.parseOnly(
            'This YAML stream contains a single text value. The next stream is a log file - a sequence of log entries. Adding an entry to the log is a simple matter of appending it at the end.\n', """
--- >
This YAML stream contains a single text value.
The next stream is a log file - a sequence of
log entries. Adding an entry to the log is a
simple matter of appending it at the end.
"""
        )

    def testForceImplicit(self):
        self.parseOnly(
			{ 'integer': 12, 'also int': 12, 'string': '12' }, """
integer: 12
also int: ! "12"
string: !str 12
""" 
        )

    def testOverrideAnchor(self):
        a001 = "The alias node below is a repeated use of this value.\n"
        self.parseOnly(
			{ 'anchor': 'This scalar has an anchor.', 'override': a001, 'alias': a001 }, """
anchor : &A001 This scalar has an anchor.
override : &A001 >
 The alias node below is a
 repeated use of this value.
alias : *A001
"""
        )

    def testSpecBuiltinSeq(self):
        self.parseOnly(
			 {'nested': ['First item in top sequence', ['Subordinate sequence entry'], 'A multi-line sequence entry\n', 'Sixth item in top sequence'], 'empty': [], 'in-line': ['one', 'two', 'three', 'four', 'five']}, """
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
"""
    )

    def testSpecBuiltinMap(self):
        self.parseOnly(
			{ 'empty': {}, 'in-line': { 'one': 1, 'two': 2 },
			  'spanning': { 'one': 1, 'two': 2 },
			  'nested': { 'first': 'First entry', 'second':
			  	{ 'key': 'Subordinate mapping' }, 'third':
				  [ 'Subordinate sequence', {}, 'Previous mapping is empty.',
					{ 'A key': 'value pair in a sequence.', 'A second': 'key:value pair.' },
					'The previous entry is equal to the following one.',
					{ 'A key': 'value pair in a sequence.', 'A second': 'key:value pair.' } ],
				  12.0: 'This key is a float.', "?\n": 'This key had to be protected.',
				  "\a": 'This key had to be escaped.',
				  "This is a multi-line folded key\n": "Whose value is also multi-line.\n" } }, """
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
# The following parses correctly,
# but Python cannot hash!
# ?
#  - This key
#  - is a sequence
# :
#  - With a sequence value.
# ?
#  This: key
#  is a: mapping
# :
#  with a: mapping value.
"""
        )

    def testSpecSingleQuote(self):
        self.parseOnly( {'empty': '', 'span': 'this contains six spaces\nand one line break', 'third': "a single quote ' must be escaped.", 'second': '! : \\ etc. can be used freely.', 'is same as': 'this contains six spaces\nand one line break'}, """
empty: ''
second:
  '! : \\ etc. can be used freely.'
third: 'a single quote '' must be escaped.'
span: 'this contains
      six spaces

      and one
      line break'
is same as: "this contains six spaces\\nand one line break" 
"""
        )

    def testSpecDoubleQuote(self):
        self.parseOnly( {'empty': '', 'third': 'a " or a \\ must be escaped.', 'is equal to': 'this contains four  spaces', 'fourth': 'this value ends with an LF.\n', 'span': 'this contains four  spaces', 'second': '! : etc. can be used freely.'}, """
empty: ""
second: "! : etc. can be used freely."
third: "a \\" or a \\\\ must be escaped."
fourth:
  "this value ends with an LF.\\n"
span: "this contains
  four  \\
      spaces"
is equal to: "this contains four  spaces" 
"""
        )

    def testSpecLiteralBlocks(self):
        self.parseOnly( {"both are equal to":"  This has no newline.", "is equal to":"The \\ ' \" characters may be\nfreely used. Leading white\n   space is significant.\n\nLine breaks are significant.\nThus this value contains one\nempty line and ends with a\nsingle line break, but does\nnot start with one.\n", "also written as":"  This has no newline.", "indented and chomped":"  This has no newline.", "empty":"", "literal":"The \\ ' \" characters may be\nfreely used. Leading white\n   space is significant.\n\nLine breaks are significant.\nThus this value contains one\nempty line and ends with a\nsingle line break, but does\nnot start with one.\n"}, """
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

is equal to: "The \\\\ \' \\" characters may \\
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
"""
        )
        str1 = "This has one newline.\n"
        str2 = "This has no newline."
        str3 = "This has two newlines.\n\n"
        self.parseOnly( 
            { 'clipped': str1, 'same as "clipped" above': str1, 
              'stripped': str2, 'same as "stripped" above': str2, 
              'kept': str3, 'same as "kept" above': str3 }, """
clipped: |
    This has one newline.

same as "clipped" above: "This has one newline.\\n"

stripped: |-
    This has no newline.

same as "stripped" above: "This has no newline."

kept: |+
    This has two newlines.

same as "kept" above: "This has two newlines.\\n\\n"

"""
        )

class SyckTestSuite(unittest.TestSuite):
    def __init__(self):
        unittest.TestSuite.__init__(self,map(BasicTests,     
            ("testBasicMap",
             "testBasicStrings",
             "testSimpleSequence",
             "testSimpleMap",
             "testSimpleMapWithNestedSequences",
             "testSimpleSequenceWithNestedMaps",
             "testSequenceOfSequences",
             "testMappingOfMappings",
             "testAnchorsAndAliases",
             "testSeqInSeqShortcut",
             "testSpecSequenceKeyShortcut",
             "testSpecSequenceKeyEmpties",
             "testSingleLiteral",
             "testSingleFolded",
             "testIndentationScope",
             "testDocComments", 
             "testDocComments2",
             "testRootBlock",
             "testForceImplicit",
             "testOverrideAnchor",
             "testSpecBuiltinSeq",
             "testSpecBuiltinMap",
             "testSpecSingleQuote",
             "testSpecDoubleQuote",
             "testSpecLiteralBlocks"
             )))

if __name__ == '__main__':
    unittest.main()


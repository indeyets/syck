import syck
import unittest

class TestCase(unittest.TestCase):
    def parseOnly(self, obj, yaml):
        self.assertEqual(obj, syck.load(yaml))

class BasicTests(TestCase):
    def testBasicMap(self):
        self.parseOnly(
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
                [ { 'five': None }, 'six' ],
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
			{ None: None, 'true': 'yes', 'false':'no', 'string': '12345' }, """
null: ~
true: yes
false: no
string: '12345'
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
             "testIndentationScope"
             )))

if __name__ == '__main__':
    unittest.main()


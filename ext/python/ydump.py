# This is temporarly placed into syck from PyYaml till syck
# gets its own emitter functionality.  - Clark Evans
# 
# A. HISTORY OF THE SOFTWARE
# ==========================
# 
# The Python library for YAML was started by Steve Howell in 
# February 2002.  Steve is the primary author of the project,
# but others have contributed.  See the README for more on 
# the project.  The term "PyYaml" refers to the entire 
# distribution of this library, including examples, documentation,
# and test files, as well as the core implementation.
# 
# This library is intended for general use, and the license 
# below protects the "open source" nature of the library.  The 
# license does, however, allow for use of the library in 
# commercial applications as well, subject to the terms 
# and conditions listed.  The license below is a minor 
# rewrite of the Python 2.2 license, with no substantive 
# differences.
# 
# 
# B. TERMS AND CONDITIONS FOR ACCESSING OR OTHERWISE USING PyYaml
# ===============================================================
# 
# LICENSE AGREEMENT FOR PyYaml
# ----------------------------
# 
# 1. This LICENSE AGREEMENT is between Stephen S. Howell ("Author"), 
# and the Individual or Organization ("Licensee") accessing and
# otherwise using PyYaml software in source or binary form and its
# associated documentation.
# 
# 2. Subject to the terms and conditions of this License Agreement, Author
# hereby grants Licensee a nonexclusive, royalty-free, world-wide
# license to reproduce, analyze, test, perform and/or display publicly,
# prepare derivative works, distribute, and otherwise use PyYaml
# alone or in any derivative version, provided, however, that Author's
# License Agreement and Author's notice of copyright, i.e., "Copyright (c)
# 2001 Steve Howell and Friends; All Rights Reserved" are never removed
# from PyYaml, and are included in any derivative version prepared by Licensee.
# 
# 3. In the event Licensee prepares a derivative work that is based on
# or incorporates PyYaml or any part thereof, and wants to make
# the derivative work available to others as provided herein, then
# Licensee hereby agrees to include in any such work a brief summary of
# the changes made to PyYaml.
# 
# 4. Author is making PyYaml available to Licensee on an "AS IS"
# basis.  Author MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
# IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, Author MAKES NO AND
# DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
# FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PyYaml WILL NOT
# INFRINGE ANY THIRD PARTY RIGHTS.
# 
# 5. Author SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON
# 2.2 FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS
# A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON 2.2,
# OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
# 
# 6. This License Agreement will automatically terminate upon a material
# breach of its terms and conditions.
# 
# 7. Nothing in this License Agreement shall be deemed to create any
# relationship of agency, partnership, or joint venture between Author and
# Licensee.  This License Agreement does not grant permission to use Author
# trademarks or trade name in a trademark sense to endorse or promote
# products or services of Licensee, or any third party.
# 
# 8. By copying, installing or otherwise using PyYaml, Licensee
# agrees to be bound by the terms and conditions of this License
# Agreement.
# 

import types
import string
from types import StringType, UnicodeType, IntType, FloatType
from types import DictType, ListType, TupleType, InstanceType
import re

"""
  The methods from this module that are exported to the top 
  level yaml package should remain stable.  If you call
  directly into other methods of this module, be aware that 
  they may change or go away in future implementations.
  Contact the authors if there are methods in this file 
  that you wish to remain stable.
"""

def hasMethod(object, method_name):
    try:    
        klass = object.__class__
    except:
        return 0
    if not hasattr(klass, method_name):
        return 0
    method = getattr(klass, method_name)
    if not callable(method):
        return 0
    return 1

def isDictionary(data):
    return isinstance(data, dict)

try:
    isDictionary({})
except:
    def isDictionary(data): 
        return type(data) == type({}) # XXX python 2.1

def dump(*data):
    return Dumper().dump(*data)

ydump = dump

def dumpToFile(file, *data):
    return Dumper().dumpToFile(file, *data)

class Dumper:
    def __init__(self):
        self.currIndent   = "\n"
        self.indent = "    "
        self.keysrt   = None
        self.alphaSort = 1 # legacy -- on by default

    def setIndent(self, indent):
        self.indent = indent
        return self

    def setSort(self, sort_hint):
        self.keysrt = sortMethod(sort_hint)
        return self

    def dump(self, *data):
        self.result = []  
        self.output = self.outputToString
        self.dumpDocuments(data)
        return string.join(self.result,"")

    def outputToString(self, data):
        self.result.append(data)

    def dumpToFile(self, file, *data):
        self.file = file
        self.output = self.outputToFile
        self.dumpDocuments(data)

    def outputToFile(self, data):
        self.file.write(data)

    def dumpDocuments(self, data):
        for obj in data:
            self.anchors  = YamlAnchors(obj)
            self.output("---")
            self.dumpData(obj)
            self.output("\n")       

    def indentDump(self, data):
        oldIndent = self.currIndent
        self.currIndent += self.indent
        self.dumpData(data)
        self.currIndent = oldIndent

    def dumpData(self, data):
        anchor = self.anchors.shouldAnchor(data)
        if anchor: 
            self.output(" &%d" % anchor )
        else:
            anchor = self.anchors.isAlias(data)
            if anchor:
                self.output(" *%d" % anchor )
                return
        if (data is None):
            pass
        elif hasMethod(data, 'to_yaml'):
            self.dumpTransformedObject(data)            
        elif hasMethod(data, 'to_yaml_implicit'):
            self.output(" " + data.to_yaml_implicit())
        elif type(data) is InstanceType:
            self.dumpRawObject(data)
        elif isDictionary(data):
            self.dumpDict(data)
        elif type(data) in [ListType, TupleType]:
            self.dumpList(data)
        else:
            self.dumpScalar(data)

    def dumpTransformedObject(self, data):
        obj_yaml = data.to_yaml()
        if type(obj_yaml) is not TupleType:
            self.raiseToYamlSyntaxError()
        (data, typestring) = obj_yaml
        if typestring:
            self.output(" " + typestring)
        self.dumpData(data)

    def dumpRawObject(self, data):
        self.output(' !!%s.%s' % (data.__module__, data.__class__.__name__))
        self.dumpData(data.__dict__)

    def dumpDict(self, data):
        keys = data.keys()
        if len(keys) == 0:
            self.output(" {}")
            return
        if self.keysrt:
            keys = sort_keys(keys,self.keysrt)
        else:
            if self.alphaSort:
                keys.sort()
        for key in keys:
            self.output(self.currIndent)
            self.dumpKey(key)
            self.output(":")
            self.indentDump(data[key])

    def dumpKey(self, key):
        if type(key) is TupleType:
            self.output("?")
            self.indentDump(key) 
            self.output("\n")
        else:
            self.output(quote(key))

    def dumpList(self, data):
        if len(data) == 0:
            self.output(" []")
            return
        for item in data:
            self.output(self.currIndent)
            self.output("-")
            self.indentDump(item)

    def dumpScalar(self, data):
        if isUnicode(data):
            self.output(' "%s"' % repr(data)[2:-1])
        elif isMulti(data):
            self.dumpMultiLineScalar(data.splitlines())
        else:
            self.output(" ")
            self.output(quote(data))
    
    def dumpMultiLineScalar(self, lines):
        self.output(" |")
        if lines[-1] == "":
            self.output("+")
        for line in lines:
            self.output(self.currIndent)
            self.output(line)

    def raiseToYamlSyntaxError(self):
            raise """
to_yaml should return tuple w/object to dump 
and optional YAML type.  Example:
({'foo': 'bar'}, '!!foobar')
"""

#### ANCHOR-RELATED METHODS

def accumulate(obj,occur):
    typ = type(obj)
    if obj is None or \
       typ is IntType or \
       typ is FloatType or \
       ((typ is StringType or typ is UnicodeType) \
       and len(obj) < 32): return
    obid = id(obj)
    if 0 == occur.get(obid,0):
        occur[obid] = 1
        if typ is ListType:
            for x in obj: 
                accumulate(x,occur)
        if typ is DictType:
            for (x,y) in obj.items():
                accumulate(x,occur)
                accumulate(y,occur)
    else:
        occur[obid] = occur[obid] + 1

class YamlAnchors:
     def __init__(self,data):
         occur = {}
         accumulate(data,occur)
         anchorVisits = {}
         for (obid, occur) in occur.items():
             if occur > 1:
                 anchorVisits[obid] = 0 
         self._anchorVisits = anchorVisits
         self._currentAliasIndex     = 0
     def shouldAnchor(self,obj):
         ret = self._anchorVisits.get(id(obj),None)
         if 0 == ret:
             self._currentAliasIndex = self._currentAliasIndex + 1
             ret = self._currentAliasIndex
             self._anchorVisits[id(obj)] = ret
             return ret
         return 0
     def isAlias(self,obj):
         return self._anchorVisits.get(id(obj),0)

### SORTING METHODS

def sort_keys(keys,fn):
    tmp = []
    for key in keys:
        val = fn(key)
        if val is None: val = ''
        tmp.append((val,key))
    tmp.sort()
    return [ y for (x,y) in tmp ]

def sortMethod(sort_hint):
    typ = type(sort_hint)
    if DictType == typ:
        return sort_hint.get
    elif ListType == typ or TupleType == typ:
        indexes = {}; idx = 0
        for item in sort_hint:
            indexes[item] = idx
            idx += 1
        return indexes.get
    else:
        return sort_hint

### STRING QUOTING AND SCALAR HANDLING
def isStr(data):
    # XXX 2.1 madness
    if type(data) == type(''):
        return 1
    if type(data) == type(u''):
        return 1
    return 0
    
def doubleUpQuotes(data):
    return data.replace("'", "''")

def quote(data):
    if not isStr(data):
        return str(data)
    single = "'"
    double = '"'
    quote = ''
    if len(data) == 0:
        return "''"
    if hasSpecialChar(data) or data[0] == single:
        data = `data`[1:-1]
        data = string.replace(data, r"\x08", r"\b")
        quote = double 
    elif needsSingleQuote(data):
        quote = single
        data = doubleUpQuotes(data)
    return "%s%s%s" % (quote, data, quote)

def needsSingleQuote(data):
    if re.match(r"^\d", data):
        return 1
    if data[0] == '&':
        return 1
    if data[0] == '"':
        return 1
    return (re.search(r'[-:]', data) or re.search(r'(\d\.){2}', data))

def hasSpecialChar(data):
    # need test to drive out '#' from this
    return re.search(r'[\t\b\r\f#]', data)

def isMulti(data):
    if not isStr(data):
        return 0
    if hasSpecialChar(data):
        return 0
    return re.search("\n", data)

def isUnicode(data):
    return type(data) == unicode
    
def sloppyIsUnicode(data):
        # XXX - hack to make tests pass for 2.1
        return repr(data)[:2] == "u'" and repr(data) != data

import sys
if sys.hexversion < 0x20200000:
    isUnicode = sloppyIsUnicode
    



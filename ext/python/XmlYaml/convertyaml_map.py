#!/usr/bin/env python
"""
Sample code to convert YAML to XML and XML to YAML using a canonical
form.

The canonical YAML representation of an XML element is a
dictionary (mapping) containing the following key/value pairs:
    (1) "name" (required) -- a string.
    (2) "attributes" (optional) -- a dictionary (mapping) of name/value
        pairs.
    (3) "text" (optional) -- a string.
    (4) "children" (optional) -- a sequence of dictionaries (mappings).

For usage information, type:
    python convertyaml_map.py

Usage: python convertyaml_map.py <option> <in_file>

Options:
    -y2x    Convert YAML file to XML document.
    -x2y    Convert XML document to YAML.


Requirements:
    PyXML
    PyYaml

"""

import sys, string, re, types
import yaml

from xml.dom import minidom
from xml.dom import Node


#
# Convert a YAML file to XML and write it to stdout.
#
def convertYaml2Xml(inFileName):
    inobj = yaml.loadFile(inFileName)
    out = []
    level = 0
    convertYaml2XmlAux(inobj, level, out)
    outStr = "".join(out)
    sys.stdout.write(outStr)


def convertYaml2XmlAux(inobj, level, out):
    for obj in inobj:
        if type(obj) == types.DictType and \
            obj.has_key('name'):
            name = obj['name']
            attributes = None
            if obj.has_key('attributes'):
                attributes = obj['attributes']
            text = None
            if obj.has_key('text'):
                text = obj['text']
            children = None
            if obj.has_key('children'):
                children = obj['children']
            addLevel(level, out)
            out.append('<%s' % name)
            if attributes:
                for key in attributes:
                    out.append(' %s="%s"' % (key, attributes[key]))
            if not (children or text):
                out.append('/>\n')
            else:
                if children:
                    out.append('>\n')
                else:
                    out.append('>')
                if text:
                    out.append(text)
                if children:
                    convertYaml2XmlAux(children, level + 1, out)
                    addLevel(level, out)
                out.append('</%s>\n' % name)


#
# Convert an XML document (file) to YAML and write it to stdout.
#
def convertXml2Yaml(inFileName):
    doc = minidom.parse(inFileName)
    root = doc.childNodes[0]
    # Convert the DOM tree into "YAML-able" data structures.
    out = convertXml2YamlAux(root)
    # Ask YAML to dump the data structures to a string.
    outStr = yaml.dump(out)
    # Write the string to stdout.
    sys.stdout.write(outStr)


def convertXml2YamlAux(obj):
    objDict = {}
    # Add the element name.
    objDict['name'] = obj.nodeName
    # Convert the attributes.
    attrs = obj.attributes
    if attrs.length > 0:
        attrDict = {}
        for idx in range(attrs.length):
            attr = attrs.item(idx)
            attrDict[attr.name] = attr.value
        objDict['attributes'] = attrDict
    # Convert the text.
    text = []
    for child in obj.childNodes:
        if child.nodeType == Node.TEXT_NODE and \
            not isAllWhiteSpace(child.nodeValue):
            text.append(child.nodeValue)
    if text:
        textStr = "".join(text)
        objDict['text'] = textStr
    # Convert the children.
    children = []
    for child in obj.childNodes:
        if child.nodeType == Node.ELEMENT_NODE:
            obj = convertXml2YamlAux(child)
            children.append(obj)
    if children:
        objDict['children'] = children
    return objDict


#
# Utility functions.
#
def addLevel(level, out):
    for idx in range(level):
        out.append('    ')


NonWhiteSpacePattern = re.compile('\S')

def isAllWhiteSpace(text):
    if NonWhiteSpacePattern.search(text):
        return 0
    return 1


USAGE_TEXT = """
Convert a file from YAML to XML or XML to YAML and write it to stdout.

Usage: python convertyaml_map.py <option> <in_file>

Options:
    -y2x    Convert YAML file to XML document.
    -x2y    Convert XML document to YAML.
"""

def usage():
    print USAGE_TEXT
    sys.exit(-1)


def main():
    args = sys.argv[1:]
    if len(args) != 2:
        usage()
    option = args[0]
    inFileName = args[1]
    if option == '-y2x':
        convertYaml2Xml(inFileName)
    elif option == '-x2y':
        convertXml2Yaml(inFileName)
    else:
        usage()


if __name__ == '__main__':
    main()
    #import pdb
    #pdb.run('main()')


""" yaml2xml

    This currently takes a recursive map/list/string structure
    and serializes it as http://yaml.org/xml/ discusses.  It is
    definately 'unstable'.  There is also a html option, and with
    a really nice CSS style sheet, it can be made to look quite
    like real YAML (using images for lists and mappings).  Note,
    this does not handle XML escaping....
"""
import types
tab = '  '

def yaml2html(data, write = None):
    return yaml2xml(data, write,'div','dl','dt','dd','ol','li')
      
def encode(data):
    if data is None:
       return ''
    return str(data).replace('"',"&quot;").replace(\
             "&","&amp;").replace("<","&lt;")
                  
def yaml2xml(data, write = None, root = 'root',
             map = '', map_key = '*', map_val = '',
             seq = '', seq_val = '_', rootattr = {} ):
    """ Very primitive dump from native map, list, scalar data
        structures into XML per http://yaml.org/xml/
    """
    ret = lambda: None
    if not write:
        lst = []
        write = lst.append
        ret = lambda: "".join(lst)

    def opentag(key,val,spacer='\n',inmap=False,attrib = {},close=False):
        if '*' != map_key and inmap:
            write("<%s>%s</%s>" % (map_key, key, map_key))
            write(spacer)
            key = map_val
        write("<%s" % key)
        keys = attrib.keys()
        keys.sort()
        indent = spacer + (' ' * (len(key)+1))
        siz = len(indent)
        if type({}) == type(val):
            lst = [(k,v) for (k,v) in val.items() if k[0] == '@']
            lst.sort()
            for (k,v) in lst:
                item = ' %s="%s"' % (k[1:],encode(v))
                if siz + len(item) > 59:
                    siz = len(indent)
                    write(indent)
                write(item)
                siz += len(item)
        for k in keys:
            item = ' %s="%s"' % (k,encode(attrib[k]))
            if siz + len(item) > 59:
                siz = len(indent)
                write(indent)
            write(item)
            siz += len(item)
        if close:
            write("/")
        write(">")

    def apply(data, indent = tab):
        spacer = "\n" + indent
        if data is None:
            return
        typ = type(data)
        if typ in (type([]), type(tuple())):
            if seq:
                write(spacer)
                write("<%s>" % seq)
            for v in data:
                write(spacer)
                opentag(seq_val,v,spacer)
                if apply(v,indent + tab):
                   write(spacer)
                write('</%s>' % seq_val )
            if seq:
                write(spacer)
                write("</%s>" % seq)
            return 1
        if type({}) == typ:
            lst = data.items()
            lst.sort()
            if map:
                write(spacer)
                write("<%s>" % map)
            for (k,v) in lst:
                if '@' == k[0] or '=' == k[0]:
                    continue
                write(spacer)
                opentag(k,v,spacer,inmap=True)
                if apply(v,indent + tab):
                   write(spacer)
                if '*' == map_key:
                    write("</%s>" % k)
                else:
                    write("</%s>" % map_val)
            if map:
                write(spacer)
                write("</%s>" % map)
            return 1
        write(encode(data))
        return 0
    if data:
        opentag(root,data,attrib=rootattr)
        if apply(data):
           write("\n")
        write('</%s>\n' % root)
    else:
        opentag(root,data,attrib=rootattr,close=True)
        write("\n")
    return ret()

if __name__ == '__main__':
    assert yaml2xml(None) == """\
<root/>
"""
    assert yaml2xml(None,root='bing',rootattr={'one': '1', 'two': '2'}) == """\
<bing one="1" two="2"/>
"""
    assert yaml2xml('hello') == """\
<root>hello</root>
"""
    assert yaml2xml({'k':'v'}) == """\
<root>
  <k>v</k>
</root>
"""
    assert yaml2xml({'@att': 'val'}, rootattr={'ick': 'value'}) == """\
<root att="val" ick="value">
</root>
"""
    assert yaml2xml({'key': 'value','sublist': [{'@at': 'aval', 'a': 'b' }]}
) == """\
<root>
  <key>value</key>
  <sublist>
    <_ at="aval">
      <a>b</a>
    </_>
  </sublist>
</root>
"""

    assert yaml2xml({'key': 'value','submap': {'@at': 'aval', 'a': 'b' }}
) == """\
<root>
  <key>value</key>
  <submap at="aval">
    <a>b</a>
  </submap>
</root>
"""
    assert yaml2xml(['one','two']) == """\
<root>
  <_>one</_>
  <_>two</_>
</root>
"""
    assert yaml2xml({'key': 'value', 'list': ['one',{'a':'b'}]}) == """\
<root>
  <key>value</key>
  <list>
    <_>one</_>
    <_>
      <a>b</a>
    </_>
  </list>
</root>
"""
    assert yaml2xml(None,root='bing',
            rootattr={'name': "Full Name",
                      "tele": "Telephone",
                      "addr": "This is someone's address." }) == """\
<bing addr="This is someone's address." name="Full Name"
      tele="Telephone"/>
"""
    assert yaml2html(None) == """\
<div/>
"""
    assert yaml2html('hello') == """\
<div>hello</div>
"""
    assert yaml2html({'k':'v'}) == """\
<div>
  <dl>
  <dt>k</dt>
  <dd>v</dd>
  </dl>
</div>
"""
    assert yaml2html(['one','two']) == """\
<div>
  <ol>
  <li>one</li>
  <li>two</li>
  </ol>
</div>
"""
    assert yaml2html({'key': 'value', 'list': ['one',{'a':'b'}]}) == """\
<div>
  <dl>
  <dt>key</dt>
  <dd>value</dd>
  <dt>list</dt>
  <dd>
    <ol>
    <li>one</li>
    <li>
      <dl>
      <dt>a</dt>
      <dd>b</dd>
      </dl>
    </li>
    </ol>
  </dd>
  </dl>
</div>
"""
    assert yaml2html({'key': 'value','submap': {'@at': 'aval', 'a': 'b' }}
) == """\
<div>
  <dl>
  <dt>key</dt>
  <dd>value</dd>
  <dt>submap</dt>
  <dd at="aval">
    <dl>
    <dt>a</dt>
    <dd>b</dd>
    </dl>
  </dd>
  </dl>
</div>
"""

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
                  
def yaml2xml(data, write = None, root = 'root',
             map = '', map_key = '*', map_val = '',
             seq = '', seq_val = '_'):
    """ Very primitive dump from native map, list, scalar data
        structures into XML per http://yaml.org/xml/
    """
    ret = lambda: None
    if not write:
        lst = []
        write = lst.append
        ret = lambda: "".join(lst)
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
                write("<%s>" % seq_val)
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
                write(spacer)
                if '*' == map_key:
                    write("<%s>" % k)
                else:
                    write("<%s>%s</%s>" % (map_key, k, map_key))
                    write(spacer)
                    write("<%s>" % map_val)
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
        write(str(data))
        return 0  
    if not data:
        write('<%s/>\n' % root)
        return ret()
    write('<%s>' % root)
    if apply(data):
       write("\n")
    write('</%s>\n' % root)
    return ret()

if __name__ == '__main__':
    assert yaml2xml(None) == """\
<root/>
"""
    assert yaml2xml('hello') == """\
<root>hello</root>
"""
    assert yaml2xml({'k':'v'}) == """\
<root>
  <k>v</k>
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

from syck import load
from ypath import ypath
from ydump import dump

for test in open("ypath_test.yml").read().split("---\n"):
    try:
       data = load(test)
    except TypeError, e:
       print dump({"error": e, "data": test })
    if not data.has_key('ignore'):
        expr = data['ypath']
        pth = ypath(expr,cntx=1)
        lst = [] 
        for x in pth.apply(data['data']):
            lst.append(str(x))
        exp = data['expected']
        if data.has_key('unordered'):
           lst.sort()
           exp.sort()
        if str(lst) != str(exp):
           print dump({"assert": test, "received": lst })

from distutils.core import setup, Extension

syck = Extension('syck',
                 include_dirs = ['/usr/local/include','../../lib'],
                 libraries = ['syck'],
                 library_dirs = ['/usr/local/lib','../../lib'],
                 sources = ['pyext.c'])

setup (name = 'Syck',
       version = '1.0',
       description = 'This is a demo package',
       py_modules = ['ypath', 'ydump'],
       ext_modules = [syck])

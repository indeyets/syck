from distutils.core import setup, Extension

syck = Extension('syck',
                 include_dirs = ['/usr/local/include'],
                 libraries = ['syck'],
                 library_dirs = ['/usr/local/lib'],
                 sources = ['pyext.c'])

setup (name = 'Syck',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [syck])

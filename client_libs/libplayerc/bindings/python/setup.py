
from distutils.core import setup, Extension


module = Extension('_playerc',
                   sources = ['playerc.i'],
                   include_dirs = ['../..', '../../../../server'],
                   library_dirs = ['../..'],
                   libraries = ['playerc'])


setup(name = 'playerc',
      version = 'X.X',
      description = 'Bindings for playerc',
      py_modules = ['playerc'],
      ext_modules = [module])
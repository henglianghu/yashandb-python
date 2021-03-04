import os
import scripts.build_handler as handler

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext as build_ext_orig


class CMakeExtension(Extension):

    def __init__(self, name):
        # don't invoke the original build_ext for this special extension
        super().__init__(name, sources=[])


class build_ext(build_ext_orig):

    def run(self):
        for ext in self.extensions:
            self.build_cmake(ext)

    def build_cmake(self, ext):
        cwd = os.getcwd()
        handler.build(['anchor_python'], [])
        os.chdir(cwd)

setup (
    name='anchor_python',
    version='0.1.1',
    author='sics',
    description='the python driver for anchorbase',
    ext_modules=[CMakeExtension('anchor_python')],
    cmdclass={
        'build_ext': build_ext,
    },
    zip_safe=False,
)
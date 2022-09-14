import os
import sys
import sysconfig
from setuptools import setup, Extension

#define the version
BUILD_VERSION = "1.0.0"
_DEBUG = False
_DEBUG_LEVEL = 0

# setup extra link and compile args
extra_link_args = []
extra_compile_args = sysconfig.get_config_var('CFLAGS').split()
if _DEBUG:
    extra_compile_args += ["-g3", "-O0", "-DDEBUG=%s" % _DEBUG_LEVEL, "-UNDEBUG"]
else:
    extra_compile_args += ["-DNDEBUG", "-O3"]
    
if sys.platform == "aix4":
    extra_compile_args.append("-qcpluscmt")
elif sys.platform == "aix5":
    extra_compile_args.append("-DAIX5")
elif sys.platform == "cygwin":
    extra_link_args.append("-Wl,--enable-runtime-pseudo-reloc")
elif sys.platform == "darwin":
    extra_link_args.append("-shared-libgcc")

source_dir = "src"
sources = [os.path.join(source_dir, n) \
           for n in sorted(os.listdir(source_dir)) if n.endswith(".c")]
depends = []


include_dirs = ["yacapi/include", "yacapi/src"]
yacapi_source_dir = os.path.join("yacapi", "src")
yacapi_sources = [os.path.join(yacapi_source_dir, n) \
            for n in sorted(os.listdir(yacapi_source_dir)) if n.endswith(".c")]
depends.extend(["yacapi/include/yacapi.h"])

apmodule = Extension(
        name='yaspy',
        sources = sources + yacapi_sources,
        include_dirs=['yacapi/include'],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        depends=depends
        )

setup (
    version=BUILD_VERSION,
    ext_modules=[apmodule],
)
import os
import platform
import sys
import sysconfig
from setuptools import setup, Extension

def get_project_path():
    return os.path.dirname(os.path.abspath(__file__))


def get_version():
    if platform.system() == 'Windows':
        null_path = 'nul'
    else:
        null_path = '/dev/null'

    os.chdir(get_project_path())
    describe = "".join(os.popen("git describe --tags --abbrev=0 2>{}".format(null_path)).readlines()).strip()
    return describe if describe else "unknow"


#define the version
BUILD_VERSION = get_version()
_DEBUG = False
_DEBUG_LEVEL = 0

# setup extra link and compile args
extra_link_args = []
extra_compile_args = []
if sys.platform == "linux":
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
elif sys.platform == "win32":
    if _DEBUG:
        extra_link_args.append("/DEBUG")
        extra_compile_args.append("/Zi")

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
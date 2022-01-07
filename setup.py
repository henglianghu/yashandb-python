from setuptools import setup, Extension

#define the version
BUILD_VERSION = "0.1.6"

apmodule = Extension('yaspy',
                    sources = ['src/anp_cli.c',
                               'src/anp_connection.c',
                               'src/anp_cursor.c',
                               'src/anp_exception.c',
                               'src/anp_module.c',
                               'src/anp_var.c'
                               ],
                    include_dirs=['ancli/include'],
                    libraries=['yascli'],
                    library_dirs=['ancli/lib']
                    )

setup (
    version=BUILD_VERSION,
    ext_modules=[apmodule],
)
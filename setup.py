from setuptools import setup, Extension

apmodule = Extension('anchor_python',
                    sources = ['src/anp_cli.c',
                               'src/anp_connection.c',
                               'src/anp_cursor.c',
                               'src/anp_exception.c',
                               'src/anp_module.c',
                               'src/anp_var.c'
                               ],
                    include_dirs=['ancli/include'],
                    libraries=['ancli'],
                    library_dirs=['ancli/lib']
                    )

setup (
    name='anchor_python',
    version='0.1.1',
    author='sics',
    description='the python driver for anchorbase',
    ext_modules=[apmodule],
    classifiers=[
        "Programming Language :: Python :: 3",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.6",
)
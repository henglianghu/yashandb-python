# yaspy

yaspy is a Python extension module, through which the YashanDB database can be accessed. It follows the conventions of [Python Database API 2.0 Specification].

## Prepare
Download yasdb-c-v0.x.x-linux-x86_64.tar.gz in [anchor_package](http://192.168.3.114:28880/anchor_pkg/latest/release)

`mkdir ancli && mv yasdb-c-v0.x.x-linux-x86_64.tar.gz ancli && cd ancli &&  tar -xzvf yasdb-c-v0.x.x-linux-x86_64.tar.gz`

## Build
`python setup.py build bdist_wheel`

## Installation
`python setup.py install`

## Documentation
See [/doc].

## Tests

See [/test].
 python -m unittest test*.py

## License

yaspy is a commercial software



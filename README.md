# yaspy

yaspy is a Python extension module, through which the YashanDB database can be accessed. It follows the conventions of [Python Database API 2.0 Specification].

## Prepare
execute `cd anchor_python && git submodule update --init --recursive` to download the c driver.

## Build
`python setup.py build`

## Package
```
pip install wheel
python setup.py build bdist_wheel
```

## Installation
`python setup.py install`

## Documentation
See [/doc].

## Tests

See [/test].
 python -m unittest test*.py

## License

yaspy is a commercial software



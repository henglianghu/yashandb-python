
from setuptools import setup


setup(
    name="yashandb-sqlalchemy",
    version="1.0.0",
    description="YashanDB Dialect for SQLAlchemy",
    author="Yang Deliu",
    author_email="yangdeliu@sics.ac.cn",
    license="MIT",
    packages=["yashandb_sqlalchemy"],
    include_package_data=True,
    entry_points={
        "sqlalchemy.dialects": [
            "yashandb = yashandb_sqlalchemy.yaspy:YasDialect_yaspy",
            "yashandb.yaspy = yashandb_sqlalchemy.yaspy:YasDialect_yaspy",
        ]
    },
)
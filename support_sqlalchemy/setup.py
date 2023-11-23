
from setuptools import setup


setup(
    name="yashandb-sqlalchemy",
    version="1.0",
    description="YashanDB Dialect for SQLAlchemy",
    author="Cod",
    author_email="Cod",
    license="MIT",
    packages=["yashandb_sqlalchemy"],
    include_package_data=True,
    #install_requires=["SQLAlchemy<2.0", "yaspy>=1.0"],
    entry_points={
        "sqlalchemy.dialects": [
            "yashandb = yashandb_sqlalchemy.yaspy:YasDialect_yaspy",
            "yashandb.yaspy = yashandb_sqlalchemy.yaspy:YasDialect_yaspy",
        ]
    },
)
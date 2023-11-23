# -*- coding: utf-8 -*-
from sqlalchemy.dialects import registry
import pytest

registry.register(
    "sqlalchemy.dialects", "yashandb.yaspy", "YasDialect_yaspy"
)

pytest.register_assert_rewrite("sqlalchemy.testing.assertions")
from sqlalchemy.testing.plugin.pytestplugin import *

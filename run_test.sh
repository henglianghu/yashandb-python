#!/bin/bash

export YASPY_TEST_ADMIN_USER=sys
export YASPY_TEST_ADMIN_PASSWORD=Cod-2022
export YASPY_TEST_CONNECT_STRING=127.0.0.1:1688
export YASPY_TEST_MAIN_USER=regress
export YASPY_TEST_MAIN_PASSWORD=regress
export YASPY_TEST_PROXY_USER=regress
export YASPY_TEST_PROXY_PASSWORD=regress
python3 -m pytest -vv test && python3 oracle_test/setup_test.py && python3 -m pytest -vv oracle_test/test_*

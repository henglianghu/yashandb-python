from sqlalchemy import create_engine
from sqlalchemy import exc
from sqlalchemy.engine import url as sa_url
from sqlalchemy.testing.provision import configure_follower
from sqlalchemy.testing.provision import create_db
from sqlalchemy.testing.provision import drop_db
from sqlalchemy.testing.provision import follower_url_from_main
from sqlalchemy.testing.provision import log
from sqlalchemy.testing.provision import post_configure_engine
from sqlalchemy.testing.provision import run_reap_dbs
from sqlalchemy.testing.provision import set_default_schema_on_connection
from sqlalchemy.testing.provision import stop_test_class_outside_fixtures
from sqlalchemy.testing.provision import temp_table_keyword_args

@temp_table_keyword_args.for_db("yashandb")
def _yashandb_temp_table_keyword_args(cfg, eng):
    return {
        "prefixes": ["GLOBAL TEMPORARY"],
        "yashandb_on_commit": "PRESERVE ROWS",
    }
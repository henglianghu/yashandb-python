import unittest
import os
import yaspy

class TestBaseCase(unittest.TestCase):
    need_connection = True
    host = os.environ.get("YASPY_TEST_HOST", "127.0.0.1")
    port = int(os.environ.get("YASPY_TEST_PORT", "1688"))
    user = os.environ.get("YASPY_TEST_MAIN_USER", "regress")
    passwd = os.environ.get("YASPY_TEST_MAIN_PASSWORD", "regress")

    def setUp(self):
        if self.need_connection:
            self.connection = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
            self.cursor = self.connection.cursor()

    def tearDown(self):
        if self.need_connection:
            self.connection.close()
            del self.cursor
            del self.connection

    def getDsn(self):
        return os.environ.get("YASPY_TEST_CONNECT_STRING", self.host + ":" + str(self.port))

    def createTable(self, name, colums):
        sql = "create table " + name + "(" + colums + ")"
        return sql

    def dropTable(self, name):
        sql = "drop table if exists " + name
        return sql

def run_test_cases():
    unittest.main(testRunner=unittest.TextTestRunner(verbosity=2))

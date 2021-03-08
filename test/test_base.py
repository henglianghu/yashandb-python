import unittest
import anchor_python as anchor

class TestBaseCase(unittest.TestCase):
    need_connection = True
    host = "127.0.0.1"
    port = 1688
    user = "sys"
    passwd = "sys"

    def setUp(self):
        if self.need_connection:
            self.connection = anchor.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
            self.cursor = self.connection.cursor()

    def tearDown(self):
        if self.need_connection:
            self.connection.close()
            del self.cursor
            del self.connection

    def getDsn(self):
        return self.host + ":"+ str(self.port)

    def createTable(self, name, colums):
        sql = "create table " + name + "(" + colums + ")"
        return sql

    def dropTable(self, name):
        sql = "drop table if exists " + name
        return sql

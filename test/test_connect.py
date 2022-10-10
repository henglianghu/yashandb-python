import unittest
import yaspy
import test_base

class TestCase(test_base.TestBaseCase):
    need_connection = False
    usr = "regress"
    pwd = "regress"

    def test_conn(self):
        conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
        self.assertFalse(conn.autocommit)
        del conn

    def test_conn_cursor(self):
        conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
        self.assertFalse(conn.autocommit)
        cursor = conn.cursor()
        cursor.execute("select * from v$instance")
        row = cursor.fetchone()
        conn.close()
        del cursor
        del conn

    def test_conn_fail(self):
        self.assertRaises(TypeError, yaspy.connect, dsn="127.0.0.1:1688")

    def test_conn_srv_fail(self):
        self.assertRaises(yaspy.DatabaseError, yaspy.connect, "127.0.0.1:1600", self.user, self.passwd)

    def test_exception_on_close(self):
        conn = yaspy.connect(user=self.usr, password=self.pwd, dsn=self.getDsn())
        self.assertFalse(conn.autocommit)

    def test_autocommit_true(self):
        conn = yaspy.connect(dsn="127.0.0.1:1688", user=self.usr, password=self.pwd)
        self.assertFalse(conn.autocommit)
        conn.autocommit = True
        self.assertTrue(conn.autocommit)
        cursor = conn.cursor()
        cursor.execute("drop table if exists t_p_autocommit")
        cursor.execute("create table t_p_autocommit(id int)")
        self.assertEqual(1, cursor.rowcount)
        cursor.execute("insert into t_p_autocommit values(1)")
        conn2 = yaspy.connect(dsn="127.0.0.1:1688", user=self.usr, password=self.pwd)
        cursor2 = conn2.cursor()
        cursor2.execute("select count(*) from t_p_autocommit")
        row = cursor2.fetchone()
        self.assertEqual(1, row[0])
        cursor.execute("drop table if exists t_p_autocommit")

    def test_autocommit_false(self):
        conn = yaspy.connect(dsn="127.0.0.1:1688", user=self.usr, password=self.pwd)
        self.assertFalse(conn.autocommit)
        cursor = conn.cursor()
        cursor.execute(self.dropTable("t_p_autocommit"))
        cursor.execute(self.createTable("t_p_autocommit", "id int"))
        self.assertEqual(1, cursor.rowcount)
        cursor.execute("insert into t_p_autocommit values(1)")
        conn2 = yaspy.connect(dsn="127.0.0.1:1688", user=self.usr, password=self.pwd)
        cursor2 = conn2.cursor()
        cursor2.execute("select count(*) from t_p_autocommit")
        row = cursor2.fetchone()
        self.assertEqual(0, row[0])
        cursor.execute("drop table if exists t_p_autocommit")

    def test_repr_connection(self):
        conn = yaspy.connect(dsn="127.0.0.1:1688", user=self.usr, password=self.pwd)
        self.assertEquals(repr(conn), 'yaspy.Connection to regress@127.0.0.1:1688')
        conn.close()
        self.assertEquals(repr(conn), 'yaspy.Connection to regress@127.0.0.1:1688')

if __name__ == '__main__':
    test_base.run_test_cases()

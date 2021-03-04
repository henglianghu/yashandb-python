from decimal import Decimal

import test_base
import anchor_python as anchor

class TestCase(test_base.TestBaseCase):
    def setUp(self):
        super().setUp()
        self.cursor.execute("drop table if exists test_cursor")
        self.cursor.execute("create table test_cursor(id int , name varchar(256))")
    

    def test_select(self):
        self.cursor.execute("select 1 from dual")
        row = self.cursor.fetchone()
        self.assertEqual(row[0], 1)
        self.assertEqual(self.cursor.rowcount, 1)

    def test_cursor_reuse(self):
        sql = "select * from v$instance"
        self.cursor.execute(sql)
        row = self.cursor.fetchone()
        self.assertEqual(1, self.cursor.rowcount)
        self.cursor.execute(sql)
        row = self.cursor.fetchone()
        self.assertEqual(1, self.cursor.rowcount)

    def test_cursor_bind(self):
        self.cursor.execute("select * from v$session where sid=?", (16,))
        row = self.cursor.fetchone()
        while row is not None:
            row = self.cursor.fetchone()
            self.assertEqual(self.cursor.rowcount, 1)

    def test_ddl(self):
        self.cursor.execute("drop table if exists t_python")
        self.assertEqual(self.cursor.rowcount, 1)
        self.cursor.execute("create table t_python(id int, name varchar(256))")
        self.assertEqual(self.cursor.rowcount, 1)

    def test_cursor_count(self):
        self.cursor.execute("select count(*) from v$instance");
        row = self.cursor.fetchone();
        self.assertGreaterEqual(row[0], 1)

    def test_cursor_all_num(self):
        self.cursor.execute("drop table if exists t_python_num")
        self.assertEqual(self.cursor.rowcount, 1)
        self.cursor.execute("create table t_python_num(int1 tinyint, int2 smallint, int3 integer, int4 bigint, int5 number(10,5), int6 float, int7 double )")
        self.assertEqual(self.cursor.rowcount, 1)
        self.cursor.execute('insert into t_python_num values(1,1,1,1,1,1,1)')
        self.connection.commit()
        self.cursor.execute('select * from t_python_num')
        row = self.cursor.fetchone()
        self.assertEqual(1, self.cursor.rowcount)
        data = (1,1,1,1,1,1,1)
        self.assertEqual(data, row)

    def test_cursor_named_param(self):
        self.cursor.execute("select * from v$session where sid=:id", (16,))
        row = self.cursor.fetchone()
        self.assertEqual(1, self.cursor.rowcount)
    
    def test_cursor_rowcount(self):
        self.cursor.execute("truncate table test_cursor")
        data = [(1,1),(2,2),(3,3)]
        self.cursor.execute("insert into test_cursor values(:1, :2)", (1,1))
        self.cursor.execute("insert into test_cursor values(:1, :2)", (2,2))
        self.cursor.execute("insert into test_cursor values(:1, :2)", (3,2))
        self.cursor.execute("select * from test_cursor")
        row = self.cursor.fetchone()
        self.assertEqual(1, self.cursor.rowcount)
        row = self.cursor.fetchone()
        self.assertEqual(2, self.cursor.rowcount)
        row = self.cursor.fetchone()
        self.assertEqual(3, self.cursor.rowcount)

    def test_cursor_desc(self):
        self.cursor.execute(self.dropTable("t_python_desc"))
        self.cursor.execute(self.createTable(
            "t_python_desc", "int1 tinyint, int2 smallint, int3 integer, int4 bigint, int5 number(10,5), int6 float, int7 double"))
        self.cursor.execute("select * from t_python_desc")
        desc = self.cursor.description
        self.assertEqual(desc, [('INT1', 1, 5, 1, None, None, 1),
                                ('INT2', 2, 8, 2, None, None, 1),
                                ('INT3', 3, 12, 4, None, None, 1),
                                ('INT4', 4, 21, 8, None, None, 1),
                                ('INT5', 7, 20, None, 10, 5, 1),
                                ('INT6', 5, 21, 4, None, None, 1),
                                ('INT7', 6, 21, 8, None, None, 1)])

    def test_cursor_iter(self):
        self.cursor.execute("truncate table test_cursor")
        self.cursor.execute("insert into test_cursor values(:1, :2)", (1,1))
        self.cursor.execute("insert into test_cursor values(:1, :2)", (2,2))
        self.cursor.execute("insert into test_cursor values(:1, :2)", (3,2))
        self.cursor.execute("select * from test_cursor")
        rows = [v for v,k in self.cursor]
        self.assertEqual(rows, [1, 2, 3])

    def test_cursor_iter_error(self):
        cur = self.connection.cursor()
        self.assertRaisesRegex(anchor.InterfaceError, 'not a query', next, cur)
        cur.execute("select * from test_cursor")
        cur.close()
        self.assertRaisesRegex(anchor.InterfaceError, 'not open', next, cur)


if __name__ == "__main__":
    test_base.run_test_cases()

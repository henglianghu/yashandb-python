import datetime
from decimal import Decimal

import test_base
import yaspy
import random
import string

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
        self.assertGreaterEqual(1, self.cursor.rowcount)

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

    def test_cursor_fetchall(self):
        self.cursor.execute("truncate table test_cursor")
        data = [(1,1),(2,2),(3,3)]
        self.cursor.execute("insert into test_cursor values(:1, :2)", (1,1))
        self.cursor.execute("insert into test_cursor values(:1, :2)", (2,2))
        self.cursor.execute("insert into test_cursor values(:1, :2)", (3,2))
        self.cursor.execute("select * from test_cursor")
        rows = self.cursor.fetchall()
        self.assertEqual(3, self.cursor.rowcount)
        self.assertEqual(3, len(rows))

    def test_cursor_fetchmany(self):
        self.cursor.execute("truncate table test_cursor")
        for i in range(10):
            self.cursor.execute("insert into test_cursor values(:1, :2)", (i,i))
        self.cursor.execute("select * from test_cursor")
        rows = self.cursor.fetchmany(5)
        self.assertEqual(5, self.cursor.rowcount)
        self.assertEqual(5, len(rows))

    def test_cursor_desc(self):
        self.cursor.execute(self.dropTable("t_python_desc"))
        self.cursor.execute(self.createTable(
            "t_python_desc", "int1 tinyint, int2 smallint, int3 integer, int4 bigint, int5 number(10,5), int6 float, int7 double"))
        self.cursor.execute("select * from t_python_desc")
        desc = self.cursor.description
        self.assertEqual(desc, [('INT1', 2, 5, 1, None, None, 1),
                                ('INT2', 3, 8, 2, None, None, 1),
                                ('INT3', 4, 12, 4, None, None, 1),
                                ('INT4', 5, 21, 8, None, None, 1),
                                ('INT5', 12, 20, 7, 10, 5, 1),
                                ('INT6', 10, 21, 4, None, None, 1),
                                ('INT7', 11, 21, 8, None, None, 1)])

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
        self.assertRaisesRegex(yaspy.InterfaceError, 'not a query', next, cur)
        cur.execute("select * from test_cursor")
        cur.close()
        self.assertRaisesRegex(yaspy.InterfaceError, 'not open', next, cur)

    def test_cursor_db_error(self):
        self.assertRaisesRegex(yaspy.DatabaseError, "table or view does not exist", self.cursor.execute, "select * from ttttttttttttt")

    def test_bind_insert_char(self):
        self.cursor.execute("drop table if exists test_char")
        self.cursor.execute("create table test_char(col1 char(10), col2 integer)")
        data = (random.choice(string.ascii_letters), random.randint(-32768, 32767))
        self.cursor.execute("insert into test_char values(?,?)", data)
        self.connection.commit()
        self.cursor.execute("select count(*) from test_char")
        row = self.cursor.fetchone()
        assert(row[0] == 1)

    def test_cursor_fetch_char(self):
        self.cursor.execute("drop table if exists test_fetch_char")
        self.cursor.execute("create table test_fetch_char(col1 varchar(20), col2 varchar(20))")
        data = ('data1','data2')
        self.cursor.execute("insert into test_fetch_char values(?,?)", data)
        self.connection.commit()
        self.cursor.execute("select * from test_fetch_char")
        row = self.cursor.fetchone()
        assert (row,data)

    def test_bug_892(self):
        self.cursor.execute("select * from v$instance")
        row = self.cursor.fetchone()
        self.assertEqual(row[0], "OPEN")

    def test_date_datatype(self):
        self.cursor.execute("drop table if exists test_date_1")
        self.cursor.execute("create table test_date_1(id int,c1 date)")
        self.cursor.execute("insert into test_date_1 values(1,'2000-10-10')")
        self.cursor.execute("select * from test_date_1")
        row = self.cursor.fetchone()
        self.assertEqual(row[1], datetime.datetime(2000, 10, 10, 0, 0))
        self.cursor.execute("drop table if exists test_date_1")
        self.cursor.execute("create table test_date_1(id int,c1 time)")
        self.cursor.execute("insert into test_date_1 values(2,'11:11:11')")
        self.cursor.execute("select * from test_date_1")
        row = self.cursor.fetchone()
        self.assertEqual(row[1], datetime.time(11, 11, 11))
        self.cursor.execute("drop table if exists test_date_1")
        self.cursor.execute("create table test_date_1(id int,c1 timestamp)")
        self.cursor.execute("insert into test_date_1 values(2,'2022-2-2 11:11:11')")
        self.cursor.execute("select * from test_date_1")
        row = self.cursor.fetchone()
        self.assertEqual(row[1], datetime.datetime(2022, 2, 2, 11, 11, 11))
        self.cursor.execute("drop table if exists test_varchar_1")

        
    def test_fix_6443(self):
        self.cursor.execute("drop table if exists test_fix_1")
        self.cursor.execute("create table test_fix_1(c1 tinyint)")
        values = ['-128', '127', '0', '0.00000000001', '127.001']
        for value in values:
            self.cursor.execute("insert into test_fix_1 values(?)", (value,))
            self.cursor.execute("select * from test_fix_1")
            self.connection.commit()
            row = self.cursor.fetchone()
            self.cursor.execute("delete from test_fix_1")
        self.cursor.execute("drop table if exists test_fix_1")
        
    def test_fix_6456(self):
        self.cursor.execute("drop table if exists test_fix_1")
        self.cursor.execute("create table test_fix_1(c1 char(10))")
        values = ['Null', 'abcd!@#$%']
        for value in values:
            self.cursor.execute("insert into test_fix_1 values(?)", (value,))
            self.cursor.execute("select * from test_fix_1")
            row = self.cursor.fetchone()
        self.cursor.execute("drop table if exists test_fix_1")
        
    def test_rowid(self):
        self.cursor.execute("drop table if exists test_rowid")
        self.cursor.execute("create table test_rowid(c1 int,c2 varchar(20))")
        data = (1, 'aaa')
        self.cursor.execute("insert into test_rowid values(?,?)", data)
        self.cursor.execute("select * from test_rowid")
        row = self.cursor.fetchone()
        self.cursor.execute("select rowid from test_rowid")
        row = self.cursor.fetchone()
        self.cursor.execute("select rowid,* from test_rowid")
        row = self.cursor.fetchone()
        self.cursor.execute("drop table if exists test_rowid")

if __name__ == "__main__":
    test_base.run_test_cases()

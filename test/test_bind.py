from decimal import Decimal

import test_base
import yaspy

class TestCase(test_base.TestBaseCase):
    def setUp(self):
        super().setUp()
        self.cursor.execute("drop table if exists test_bind")
        self.cursor.execute("create table test_bind(id int , name varchar(256))")

    def test_bind_param(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_param_heap_1")
        cursor.execute("create table test_bind_param_heap_1(a int, b double, c int)")
        self.assertEqual(0, self.cursor.rowcount)
        cursor.execute("insert into test_bind_param_heap_1 values(:1, :2, 30)",(1,10))
        cursor.execute("insert into test_bind_param_heap_1 values(:1, :2, 40)",(2,20))
        self.connection.commit()
        cursor.execute("delete from test_bind_param_heap_1 where a=:1",(1,))
        self.connection.commit()
        cursor.execute("select * from test_bind_param_heap_1")
        row = cursor.fetchone()
        data = (2,20.0,40)
        self.assertEqual(data, row)
        cursor.execute("update test_bind_param_heap_1 set c=:1", (50,))
        self.connection.commit()
        cursor.execute("select * from test_bind_param_heap_1")
        row = cursor.fetchone()
        data = (2,20.0,50)
        self.assertEqual(data, row)

    def test_bind_over(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_over")
        cursor.execute("create table test_bind_over(c1 int , c2 varchar(10))")
        try:
            cursor.execute("insert into test_bind_over values(:1, :2)", (1, 2, 3))
        except Exception as e:
            error = str(e)
            expectMsg = "YAS-00212 index 3 is out of [1, 2]"
            self.assertEqual(error, expectMsg)
        cursor.execute("drop table if exists test_bind_over")

    def test_bind_param_by_name(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_param_heap_1")
        cursor.execute("create table test_bind_param_heap_1(a int, b double, c int)")
        self.assertEqual(0, self.cursor.rowcount)
        cursor.execute("insert into test_bind_param_heap_1 values(:arg0, :arg1, 30)", {'arg0': 1, 'arg1': 10})
        cursor.execute("insert into test_bind_param_heap_1 values(:arg0, :arg1, 40)", {'arg0': 2, 'arg1': 20})
        cursor.executemany("insert into test_bind_param_heap_1 values(:arg0, :arg1, 30)", [{'arg0': 3, 'arg1': 30}, {'arg0': 4, 'arg1': 40}])
        self.connection.commit()
        cursor.execute("delete from test_bind_param_heap_1 where a=:arg0", {'arg0': 1,})
        self.connection.commit()
        cursor.execute("select * from test_bind_param_heap_1 where a=:arg0", {'arg0': 2,})
        row = cursor.fetchone()
        data = (2, 20.0, 40)
        cursor.execute("select * from test_bind_param_heap_1 where a=:arg0", {'arg0': 3,})
        row = cursor.fetchone()
        data = (3, 30.0, 30)
        self.assertEqual(data, row)
        cursor.execute("update test_bind_param_heap_1 set c=:arg0", {'arg0': 50,})
        self.connection.commit()
        cursor.execute("select * from test_bind_param_heap_1 order by a")
        row = cursor.fetchone()
        data = (2, 20.0, 50)
        self.assertEqual(data, row)

    def test_batch_bind_param(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_batch_param")
        cursor.execute("create table test_batch_param(a int, b blob, c clob, d nclob)")
        self.assertEqual(0, self.cursor.rowcount)
        cursor.executemany(
            "insert into test_batch_param values(:arg0, :arg1, :arg2, :arg3)",
            [
                {"arg0": 1, "arg1": b"111111", "arg2": "this is a clob1", "arg3": "这是一个NCLOB1"},
                {"arg0": 2, "arg1": b"222222", "arg2": "this is a clob2", "arg3": "这是一个NCLOB2"},
                {"arg0": 3, "arg1": b"333333", "arg2": "this is a clob3", "arg3": "这是一个NCLOB3"},
            ],
        )
        self.connection.commit()
        cursor.execute("delete from test_batch_param where a=:arg0", {'arg0': 1,})
        self.connection.commit()
        cursor.execute("select * from test_batch_param where a=:arg0", {'arg0': 2,})
        row = cursor.fetchone()
        data = (2, b"222222", "this is a clob2", "这是一个NCLOB2")
        self.assertEqual(data, row)
        cursor.execute("select * from test_batch_param where a=:arg0", {'arg0': 3,})
        row = cursor.fetchone()
        data = (3, b"333333", "this is a clob3", "这是一个NCLOB3")
        self.assertEqual(data, row)
        cursor.execute("update test_batch_param set d=:arg0 where a=:arg1", {'arg0': '更新了NCLOB2', 'arg1': 2})
        self.connection.commit()
        cursor.execute("select * from test_batch_param order by a")
        row = cursor.fetchall()
        data = [(2, b"222222", "this is a clob2", "更新了NCLOB2"), (3, b"333333", "this is a clob3", "这是一个NCLOB3")]
        self.assertEqual(data, row)

    def test_sdv_tb_python_ydbrd_sit_29_1(self):
        conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
        cursor = conn.cursor()
        cursor.execute("drop table if exists tb_python_ydbrd_sit_29_1")
        # I1.建表
        cursor.execute("create table tb_python_ydbrd_sit_29_1(col1 varchar(50),col2 int)")
        cursor.execute("insert into  tb_python_ydbrd_sit_29_1 values(null,0)")
        # I2.Using  Bind  Variables
        sql = """insert into tb_python_ydbrd_sit_29_1 values(:col1,:col2)"""
        cursor.execute(sql, ['fdsfdf', 1])
        cursor.execute("commit")
        cursor.execute("select trim(col1),col2  from tb_python_ydbrd_sit_29_1   order by col2 ")
        result = cursor.fetchall()
        print('result_1', result)
        self.assertEqual(result, [(None, 0), ('fdsfdf', 1)])
        # I3.Binding By Name or Position
        cursor.execute("""insert into tb_python_ydbrd_sit_29_1 values(:col1,:col2)""", ("中国", 2))
        cursor.execute("commit")
        cursor.execute("select trim(col1),col2  from tb_python_ydbrd_sit_29_1   order by col2 ")
        result = cursor.fetchall()
        print('result_2', result)
        self.assertEqual(result, [(None, 0), ('fdsfdf', 1), ('中国', 2)])
        data = dict(col1='fdfdf', col2=3)
        cursor.execute("""insert into tb_python_ydbrd_sit_29_1 values(:col1,:col2)""", data)
        data = ('fdfdf', 999)
        cursor.execute("""insert into tb_python_ydbrd_sit_29_1 values(:col1,:col2)""", data)
        cursor.execute("commit")
        cursor.execute("select trim(col1),col2  from tb_python_ydbrd_sit_29_1   order by col2 ")
        result = cursor.fetchall()
        print('result_3', result)
        self.assertEqual(result, [(None, 0), ('fdsfdf', 1), ('中国', 2), ('fdfdf', 3), ('fdfdf', 999)])
        cursor.execute("commit")
        cursor.execute("select trim(col1),col2  from tb_python_ydbrd_sit_29_1   order by col2 ")
        result = cursor.fetchall()
        print('result_4', result)
        self.assertEqual(result, [(None, 0), ('fdsfdf', 1), ('中国', 2), ('fdfdf', 3), ('fdfdf', 999)])
        # I5.Binding ROWID Values
        cursor.execute("""select rowid, col1, col2  from tb_python_ydbrd_sit_29_1  where col2 = :col2""", [1])
        rowid, col1, col2 = cursor.fetchone()
        print("rowid", rowid)
        cursor.execute(
            """update tb_python_ydbrd_sit_29_1 set  col1 = :rowid""", [rowid])
        cursor.execute("select trim(col1),col2  from tb_python_ydbrd_sit_29_1   order by col2 ")
        result = cursor.fetchall()
        print('result_5', result)
        # I6.删表，清理环境
        cursor.execute("drop table tb_python_ydbrd_sit_29_1")
        cursor.close()
        conn.close()

if __name__ == "__main__":
    test_base.run_test_cases()

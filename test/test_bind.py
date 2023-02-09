from decimal import Decimal

import test_base

class TestCase(test_base.TestBaseCase):
    def setUp(self):
        super().setUp()
        self.cursor.execute("drop table if exists test_bind")
        self.cursor.execute("create table test_bind(id int , name varchar(256))")

    def test_bind_param(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_param_heap_1")
        cursor.execute("create table test_bind_param_heap_1(a int, b double, c int)")
        self.assertEqual(1, self.cursor.rowcount)
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

    
if __name__ == "__main__":
    test_base.run_test_cases()

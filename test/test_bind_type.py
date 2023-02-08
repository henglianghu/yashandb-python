from decimal import Decimal
import datetime

import test_base

class TestCase(test_base.TestBaseCase):

    def test_bind_type(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_type")
        cursor.execute("create table test_bind_type(c1 tinyint, c2 smallint, c3 int, c4 bigint, c5 float, c6 double, c7 number(10, 3))")
        cursor.execute("insert into test_bind_type values(?, ?, ?, ?, ?, ?, ?)", ('127', 456, 789.123, 9999, '123.456', 678.912, 1234567.1234))
        cursor.execute("select * from test_bind_type")
        row = cursor.fetchone()
        data = (127, 456, 789, 9999, 123.45600128173828, 678.912, Decimal('1234567.123'))
        for index, item in enumerate(row):
            if index == 4 or index == 5: 
                self.assertAlmostEqual(item, data[index])
            else:
                self.assertEqual(item, data[index])

        cursor.execute("truncate table test_bind_type")
        cursor.execute("insert into test_bind_type values(?, ?, ?, ?, ?, ?, ?)", ('', '', '', '', '', '', ''))
        cursor.execute("select * from test_bind_type")
        row = cursor.fetchone()
        data = (None, None, None, None, None, None, None)
        self.assertEqual(data, row)
        cursor.execute("drop table if exists test_bind_type")

    def test_bind_boolean(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_boolean")
        cursor.execute("create table test_bind_boolean(c1 int, c2 boolean)")
        cursor.execute("insert into test_bind_boolean values(?, ?)", (1, True))
        cursor.execute("select * from test_bind_boolean")
        row = cursor.fetchone()
        data = (1, True)
        self.assertEqual(data, row)
        cursor.execute("drop table if exists test_bind_boolean")
    
    def test_bind_one(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_one")
        cursor.execute("create table test_bind_one(c1 int)")
        cursor.execute("insert into test_bind_one values(?)", (True,))
        cursor.execute("select * from test_bind_one")
        row = cursor.fetchone()
        data = (1,)
        self.assertEqual(data, row)
        cursor.execute("drop table if exists test_bind_one")
    
    def test_bind_bits(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_bits")
        cursor.execute("create table test_bind_bits(c1 bit(17), c2 bit(32))")
        cursor.execute("insert into test_bind_bits values(?, ?)", (65535, 365088519))
        self.connection.commit()
        cursor.execute("select * from test_bind_bits")
        row = cursor.fetchone()
        data = ('1111111111111111', '10101110000101100111100000111')
        self.assertEqual(data, row)
        cursor.execute("drop table if exists test_bind_bits")
    
    def test_bind_ds_insterval(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_intval")
        cursor.execute("create table test_bind_intval(a interval year(4) to month, b interval day(4) to second(5))")
        cursor.execute("insert into test_bind_intval values(interval '2021-3' year(4) to month, interval '25 14:15:20.000005' day to second)")
        cursor.execute("select * from test_bind_intval")
        row = cursor.fetchone()
        data = ('+2021-03', '+25 14:15:20.000010')
        self.assertEqual(data, row)
        cursor.execute("drop table if exists test_bind_intval")

    def test_bind_int_value(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_int_value")
        cursor.execute("create table test_int_value(c1 bigint)")
        cursor.execute("insert into test_int_value values(?)", (-9223372036854775808,))
        cursor.execute("insert into test_int_value values(?)", (9223372036854775807,))
        cursor.execute("select * from test_int_value")
        row = cursor.fetchall()
        data = [(-9223372036854775808,), (9223372036854775807,)]
        self.assertEqual(data, row)
        try:
            cursor.execute("insert into test_int_value values(?)", (9223372036854775808,))
        except OverflowError as e:
            self.assertEqual(type(e), OverflowError)
        else:
            self.fail('OverflowError not rasied')
        cursor.execute("drop table if exists test_int_value")
    
    def test_bind_date_time(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_date_time")
        cursor.execute("create table test_bind_date_time(c1 int, c2 timestamp)")
        bindDateTime = datetime.datetime(2023, 2, 7, 15, 27, 55, 207261)
        cursor.execute("insert into test_bind_date_time values(?, ?)", (1, bindDateTime))
        cursor.execute("select * from test_bind_date_time")
        expectedRow = (1, bindDateTime)
        row = cursor.fetchone()
        self.assertEqual(expectedRow, row)
        cursor.execute("drop table if exists test_bind_date_time")
    
    def test_bind_date(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_date")
        cursor.execute("create table test_bind_date(c1 int, c2 date)")
        bindDate = datetime.date(2023, 2, 7)
        cursor.execute("insert into test_bind_date values(?, ?)", (2, bindDate))
        cursor.execute("select * from test_bind_date")
        expectedRow = (2, bindDate)
        row = cursor.fetchone()
        self.assertEqual(expectedRow, row)
        cursor.execute("drop table if exists test_bind_date")
    
    def test_bind_time(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_type_time")
        cursor.execute("create table test_bind_type_time(c1 int, c2 time)")
        bindTime = datetime.time(18, 2, 58, 123456)
        cursor.execute("insert into test_bind_type_time values(?, ?)", (3, bindTime))
        cursor.execute("select * from test_bind_type_time")
        expectedRow = (3, bindTime)
        row = cursor.fetchone()
        self.assertEqual(expectedRow, row)
        cursor.execute("drop table if exists test_bind_type_time")

    def test_fetch_long_str(self):
        cursor = self.connection.cursor()
        cursor.execute("select lpad('abcdef', 32000, '*') from dual")
        rows = cursor.fetchone()
        self.assertEqual(len(rows[0]), 32000)
    
    def test_sdv_tb_python_ydbrd_6583_28_4(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists tb_python_ydbrd_6583_28_4")
        cursor.execute("create table tb_python_ydbrd_6583_28_4(col1 raw(80), col2 int)")

        i = 1
        values = [b'', b'http://c.biancheng.net/python/', bytes('C语言中文网8岁了', encoding='UTF-8')]
        for value in values:
            cursor.execute("insert into tb_python_ydbrd_6583_28_4 values(?, ?)", (value, i, ))
            i = i + 1
        cursor.execute("select col1 from tb_python_ydbrd_6583_28_4")
        results = cursor.fetchall()
        print(results)
        expectRs = [(None,), (b'http://c.biancheng.net/python/',), 
            (b'C\xe8\xaf\xad\xe8\xa8\x80\xe4\xb8\xad\xe6\x96\x87\xe7\xbd\x918\xe5\xb2\x81\xe4\xba\x86',)]
        self.assertEqual(results, expectRs)
  
        value1 = b'http://c.biancheng.net/python/'
        cursor.execute("update tb_python_ydbrd_6583_28_4 set col1 = :1 where col2 = :2", (value1, 2,))
        cursor.execute("select col1, col2  from  tb_python_ydbrd_6583_28_4 order by col2")
        results = cursor.fetchmany(20)
        expectRs = [(None, 1), (b'http://c.biancheng.net/python/', 2), 
            (b'C\xe8\xaf\xad\xe8\xa8\x80\xe4\xb8\xad\xe6\x96\x87\xe7\xbd\x918\xe5\xb2\x81\xe4\xba\x86', 3)]
        self.assertEqual(results, expectRs)

        cursor.execute("delete from  tb_python_ydbrd_6583_28_4  where col1 = ?", (value1,))
        cursor.execute("select col1,col2  from tb_python_ydbrd_6583_28_4 order by col2")
        results = cursor.fetchmany(20)
        expectRs = [(None, 1), (b'C\xe8\xaf\xad\xe8\xa8\x80\xe4\xb8\xad\xe6\x96\x87\xe7\xbd\x918\xe5\xb2\x81\xe4\xba\x86', 3)]
        self.assertEqual(results, expectRs)

        cursor.execute("drop table if exists tb_python_ydbrd_6583_28_4")
    

if __name__ == "__main__":
    test_base.run_test_cases()

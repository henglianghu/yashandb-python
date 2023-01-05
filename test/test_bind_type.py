from decimal import Decimal

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
            self.assertEquals(type(e), OverflowError)
        else:
            self.fail('OverflowError not rasied')
        cursor.execute("drop table if exists test_int_value")

    def test_fetch_long_str(self):
        cursor = self.connection.cursor()
        cursor.execute("select lpad('abcdef', 32000, '*') from dual")
        rows = cursor.fetchone()
        self.assertEqual(len(rows[0]), 32000)
        cursor.execute("select lpad('abcdef', 32000, '*') from dual")
    

if __name__ == "__main__":
    test_base.run_test_cases()

import random
from sqlite3 import connect
import test_base
import yaspy

class TestCase(test_base.TestBaseCase):
    def test_lob(self):
        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("create table test_lob_1(id int,c1 clob)")
        self.cursor.execute("insert into test_lob_1 values(1, 'b12a')")
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        data = (1, 'b12a')
        self.assertEqual(data, row)
        self.cursor.execute("delete from test_lob_1")
        data = (2, 'aaaaa')
        self.cursor.execute("insert into test_lob_1 values(?,?)", data)
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.assertEqual(data, row)
        self.cursor.execute("delete from test_lob_1")
        self.cursor.execute("insert into test_lob_1 values(3, lpad('00',20000,'abcd'))")
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("create table test_lob_1(id int, c1 blob)")
        self.cursor.execute("insert into test_lob_1 values(1,'b12aaa')")
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.assertEqual((1, 'B12AAA'), row)
        self.cursor.execute("delete from test_lob_1")
        self.cursor.execute("insert into test_lob_1 values(?, ?)", (2, 'aaaa'))
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.assertEqual((2, 'AAAA'), row)
        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("create table test_lob_1(c1 clob,c2 blob)")
        self.cursor.execute("insert into test_lob_1 values('aaa', 'b12a')")
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        data = ('aaa', 'B12A')
        self.assertEqual(data, row)
        self.cursor.execute("delete from test_lob_1")
        self.cursor.execute("insert into test_lob_1 values(NULL, NULL)")
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("drop table if exists tb_python_blob_01_1")
        self.cursor.execute("create table tb_python_blob_01_1(col1 blob)")
        self.cursor.execute("insert into tb_python_blob_01_1 values('a')")
        self.cursor.execute("insert into tb_python_blob_01_1 values('')")
        self.cursor.execute("insert into tb_python_blob_01_1 values('123')")
        self.cursor.execute("select col1 from tb_python_blob_01_1")
        result=self.cursor.fetchall()
        seed = "1234567890"
        str1 = []
        for i in range(32):
            str1.append(random.choice(seed))
            StringS = ''.join(str1) 
        try:
            self.cursor.execute("insert into tb_python_blob_01_1 values(?)", (StringS,))
        except Exception as e:
           error = str(e)
           if('illegal conversion from CLOB to BLOB' not in error):
             raise Exception('failed')
        seed = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+=-"
        str1 = []
        for i in range(32001):
            str1.append(random.choice(seed))
            StringS = ''.join(str1) 
        try:
            self.cursor.execute("insert into tb_python_blob_01_1 values(?)", (StringS,))
        except Exception as e:
           error = str(e)
           if('illegal conversion from CLOB to BLOB' not in error):
             raise Exception('failed')
        self.cursor.execute("select col1 from tb_python_blob_01_1")
        result=self.cursor.fetchone()
        values = [b'', b'http://c.biancheng.net/python/']
        for value in values:
            try:
                self.cursor.execute("insert into tb_python_blob_01_1 values(?)", (value,))
            except Exception as e:
               error = str(e)
               if('invalid hex number' not in error):
                 raise Exception('failed')
            self.cursor.execute("select col1 from tb_python_blob_01_1")
            row = self.cursor.fetchone()
        self.cursor.execute("drop table tb_python_blob_01_1")
        
    if __name__ == "__main__":
        test_base.run_test_cases()

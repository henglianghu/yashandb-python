import os
import random
import test_base
import yaspy

class TestCase(test_base.TestBaseCase):
    def test_lob(self):
        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("create table test_lob_1(id int, c1 clob)")
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
        self.cursor.execute("insert into test_lob_1 values(?, ?)", (1, b'b12aaa'))
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.assertEqual((1, b'b12aaa'), row)
        self.cursor.execute("delete from test_lob_1") 
        self.cursor.execute("insert into test_lob_1 values(?, ?)", (2, b'aaaa'))
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.assertEqual((2, b'aaaa'), row)
        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("create table test_lob_1(c1 clob, c2 blob)")
        self.cursor.execute("insert into test_lob_1 values(?, ?)", ('aaa', b'b12a'))
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        data = ('aaa', b'b12a')
        self.assertEqual(data, row)
        self.cursor.execute("delete from test_lob_1")
        self.cursor.execute("insert into test_lob_1 values(NULL, NULL)")
        self.cursor.execute("select * from test_lob_1")
        row = self.cursor.fetchone()
        self.assertEqual(row, (None,None))

        self.cursor.execute("drop table if exists test_lob_1")
        self.cursor.execute("drop table if exists tb_python_blob_01_1")
        self.cursor.execute("create table tb_python_blob_01_1(col1 blob)")
        self.cursor.execute("insert into tb_python_blob_01_1 values(?)", (b'a',))
        self.cursor.execute("insert into tb_python_blob_01_1 values('')")
        self.cursor.execute("insert into tb_python_blob_01_1 values(?)", (b'123',))
        self.cursor.execute("select col1 from tb_python_blob_01_1")
        result=self.cursor.fetchall()
        self.assertEqual(result, [(b'a',), (None,), (b'123',)])
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
           if('invalid hex number' not in error):
             raise Exception('failed')
        self.cursor.execute("select col1 from tb_python_blob_01_1")
        result=self.cursor.fetchone()
        values = [b'', b'http://c.biancheng.net/python/']
        for value in values:
            self.cursor.execute("insert into tb_python_blob_01_1 values(?)", (value,))
            self.cursor.execute("select col1 from tb_python_blob_01_1")
            row = self.cursor.fetchone()
        self.cursor.execute("drop table tb_python_blob_01_1")
        self.cursor.execute("drop table if exists tb_python_clob_02_1")
        self.cursor.execute("create table tb_python_clob_02_1(col1 clob,col2 int)")
        self.cursor.execute("insert into tb_python_clob_02_1 values('a',2147483647)")
        self.cursor.execute("insert into tb_python_clob_02_1 values('',-2147483648)")
        self.cursor.execute("insert into tb_python_clob_02_1 values(null,-2147483648)")
        self.cursor.execute("insert into tb_python_clob_02_1 values(123,0)")
        self.cursor.execute("insert into tb_python_clob_02_1 values(-1.797693134862315807,8)")
        self.cursor.execute("insert into tb_python_clob_02_1 values(1/0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000001,21)")
        self.cursor.execute("select col1,col2  from tb_python_clob_02_1 order by col2")
        result=self.cursor.fetchall()
        self.assertEqual(result, [(None,-2147483648), (None,-2147483648), ('123', 0), ('-1.797693134862315807', 8), ('1.0000000000000000000000000000000000E+89', 21), ('a', 2147483647)])
        self.cursor.execute("drop table if exists tb_python_clob_02_1")
        self.cursor.execute("drop table if exists tb_python_clob_06_1")
        self.cursor.execute("create table tb_python_clob_06_1(col1 clob,col2 int)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('abcdef',-1)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('',2)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('9012345678',3)")
        self.cursor.execute("insert into tb_python_clob_06_1 values(null,0)")
        values = ['1234567890adefkl','0','9','df']
        i = 12
        for value in values:
            self.cursor.execute("insert into tb_python_clob_06_1 values(?,?)", (value,i))
            i = i + 1
        self.cursor.execute("select col1,col2  from tb_python_clob_06_1 order by col2")
        result=self.cursor.fetchall()
        self.assertEqual(result, [('abcdef', -1), (None, 0), (None, 2), ('9012345678', 3), ('1234567890adefkl', 12), ('0', 13), ('9', 14), ('df', 15)])
        self.cursor.execute("drop table if exists tb_python_clob_02_1")
        
        self.cursor.execute("drop table if exists tb_python_clob_06_1")
        self.cursor.execute("create table tb_python_clob_06_1(col1 clob,col2 int)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('abcdef',-1)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('',2)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('9012345678',3)")
        self.cursor.execute("insert into tb_python_clob_06_1 values(null,0)")
        values = ['1234567890adefkl','0','9','df','aa']
        i = 12
        for value in values:
            self.cursor.execute("insert into tb_python_clob_06_1 values(?,?)", (value,i))
            i = i + 1
        self.cursor.execute("select col1,col2  from tb_python_clob_06_1 order by col2")
        result=self.cursor.fetchall()
        self.assertEqual(result, [('abcdef', -1), (None, 0), (None, 2), ('9012345678', 3), ('1234567890adefkl', 12), ('0', 13), ('9', 14), ('df', 15), ('aa', 16)])

        self.cursor.execute("drop table if exists tb_python_clob_06_1")
        self.cursor.execute("create table tb_python_clob_06_1(col1 clob,col2 int)")
        self.cursor.execute("insert into tb_python_clob_06_1 values('中国、。，。反对反对’;',0)")
        self.cursor.execute("commit")
        self.cursor.execute("select col1,col2  from tb_python_clob_06_1 order by col2")
        result=self.cursor.fetchall()
        self.assertEqual(result,[('中国、。，。反对反对’;', 0)])

    def test_sdv_tb_python_clob_03(self):
        self.cursor.execute("drop table if exists tb_python_clob_03_1")
        #I1.创建表
        self.cursor.execute("create table tb_python_clob_03_1(col1 clob, col2 int)")
        #I2.插入数据
        self.cursor.execute("insert into tb_python_clob_03_1 values(null, 0)")
        self.cursor.execute("insert into tb_python_clob_03_1 values('AQD~!@#<F11>`abcdf', 1)")
        self.cursor.execute("insert into tb_python_clob_03_1 values('中国@china~~~~！@#￥%……&*（）——+', 2)")

        #python的传参方式插入
        values = ['1234567890abcdef','中国@china~~~~！@#￥%……&*（）——+','9876543210','AQD~!@i<fdfdf>#abcdf','']
        i = 3
        for value in values:
            self.cursor.execute("insert into tb_python_clob_03_1 values(?,?)", (value, i))
            i = i + 1
        
        #I3.python插入时，绑定的时单双引号等 
        values = ['\'fdsfdf我123','"对方的"adfdf12',"'dfd'''''f!"]
        for value in values:
            self.cursor.execute("insert into tb_python_clob_03_1 values(?, ?)", (value, i))
            i = i + 1

        self.cursor.execute("select col1, col2 from tb_python_clob_03_1 order by col2")
        result=self.cursor.fetchall()
        expectRow = [(None, 0), ('AQD~!@#<F11>`abcdf', 1), ('中国@china~~~~！@#￥%……&*（）——+', 2), 
        ('1234567890abcdef', 3), ('中国@china~~~~！@#￥%……&*（）——+', 4), ('9876543210', 5), 
        ('AQD~!@i<fdfdf>#abcdf', 6), (None, 7), ("'fdsfdf我123", 8), ('"对方的"adfdf12', 9), ("'dfd'''''f!", 10)]
        self.assertEqual(result, expectRow)
        #I4.删表
        self.cursor.execute("drop table if exists tb_python_clob_03_1")
    
    def test_lob_none(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_lob_none")
        cursor.execute("create table test_lob_none(c1 clob, c2 blob)")
        cursor.execute("insert into test_lob_none values('', '')")
        cursor.execute("insert into test_lob_none values(null, null)")
        cursor.execute("select * from test_lob_none")
        rows = cursor.fetchall()
        expectRows = [(None, None), (None, None)]
        self.assertEqual(rows, expectRows)
    
    def test_fetch_blob(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_blob_fetch")
        cursor.execute("create table test_blob_fetch(c1 int, c2 blob)")
        binaryData = b'Some Binary Data'
        cursor.execute("insert into test_blob_fetch values(?, ?)", (1, binaryData))
        cursor.execute("select * from test_blob_fetch")
        fetchRow = cursor.fetchone()
        expectRow = (1, b'Some Binary Data')
        self.assertEqual(fetchRow, expectRow)
        cursor.execute("drop table if exists test_blob_fetch")
    
    def test_fetch_clob(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_fetch_clob")
        cursor.execute("create table test_fetch_clob(id int, c1 clob)")
        cursor.execute("insert into test_fetch_clob values(1, lpad('abcdef', 32000, '*'))")
        cursor.execute("update test_fetch_clob set c1 = c1||c1")
        cursor.execute("select c1 from test_fetch_clob")
        rows = cursor.fetchone()
        self.assertEqual(len(rows[0]), 64000)
        cursor.execute("drop table if exists test_fetch_clob")
    
    def test_nclob_fetch1(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists tb_python_ydbrd_6583_51_1")
        cursor.execute("create table tb_python_ydbrd_6583_51_1(col1 int,col2 nclob)")
        cursor.execute("insert into tb_python_ydbrd_6583_51_1 values(1,lpad('a',32000,'b'))")
        cursor.execute("insert into tb_python_ydbrd_6583_51_1 values(2,null)")
        cursor.execute("insert into tb_python_ydbrd_6583_51_1 values(3,'abc')")
        cursor.execute("select col2 from tb_python_ydbrd_6583_51_1 where col1 = 3")
        
        fetchRow = cursor.fetchone()
        expectRow = ('abc',)
        self.assertEqual(fetchRow, expectRow)

        values = ['aa中国!@#', '爱心❤[爱心]', None, '']
        i = 4
        for value in values:
            cursor.execute("insert into tb_python_ydbrd_6583_51_1 values(?,?)", (i, value,))
            i = i+ 1
        cursor.execute("select col2 from tb_python_ydbrd_6583_51_1 where col1 = 4")
        
        fetchRow = cursor.fetchone()
        expectRow = ('aa中国!@#',)
        self.assertEqual(fetchRow, expectRow)

        cursor.execute("drop table tb_python_ydbrd_6583_51_1")
        cursor.close()


if __name__ == "__main__":
    test_base.run_test_cases()

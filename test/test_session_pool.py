import threading

import test_base
import yaspy


class TestCase(test_base.TestBaseCase):
    def test_pool_repr(self):
        pool = yaspy.SessionPool(self.user, self.passwd, self.getDsn())
        self.assertEqual(str(pool), f"yaspy.SessionPool to {self.user}@{self.getDsn()}")

    def test_pool_create_close(self):
        # 默认参数创建
        pool = yaspy.SessionPool(self.user, self.passwd, self.getDsn())
        self.assertEqual(pool.dsn, self.getDsn())
        self.assertEqual(pool.username, self.user)
        self.assertEqual(pool.password, self.passwd)
        self.assertEqual(pool.min, 1)
        self.assertEqual(pool.max, 4)
        self.assertEqual(pool.increment, 1)
        pool.close()
        # 指定参数创建
        pool = yaspy.SessionPool(
            dsn=self.getDsn(), user=self.user, password=self.passwd, min=4, max=8, increment=2, getmode=1
        )
        self.assertEqual(pool.dsn, self.getDsn())
        self.assertEqual(pool.username, self.user)
        self.assertEqual(pool.password, self.passwd)
        self.assertEqual(pool.min, 4)
        self.assertEqual(pool.max, 8)
        self.assertEqual(pool.increment, 2)
        pool.close()
        
    def test_connection_acquire_relase(self):
        pool = yaspy.SessionPool(dsn=self.getDsn(), user=self.user, password=self.passwd)
        # 对象赋值方式使用
        conn1 = pool.acquire()
        with conn1.cursor() as cursor:
            cursor.execute("select 'conn1' from dual")
            self.assertEqual(cursor.fetchall(), [("conn1",)])
        pool.release(conn1)
        conn2 = pool.acquire()
        with conn2.cursor() as cursor:
            cursor.execute("select 'conn2' from dual")
            self.assertEqual(cursor.fetchall(), [("conn2",)])
        pool.release(connection=conn2)
        # 通过上下文管理方式自动管理连接对象
        with pool.acquire() as conn3:
            with conn3.cursor() as cursor:
                cursor.execute("select 'conn3' from dual")
                self.assertEqual(cursor.fetchall(), [("conn3",)])
        # 测试同时获取超过连接池最大限制（默认4）连接数量
        conn4 = pool.acquire()
        with conn4.cursor() as cursor:
            cursor.execute("select 'conn4' from dual")
            self.assertEqual(cursor.fetchall(), [("conn4",)])
        conn5 = pool.acquire()
        with conn5.cursor() as cursor:
            cursor.execute("select 'conn5' from dual")
            self.assertEqual(cursor.fetchall(), [("conn5",)])
        conn6 = pool.acquire()
        with conn6.cursor() as cursor:
            cursor.execute("select 'conn6' from dual")
            self.assertEqual(cursor.fetchall(), [("conn6",)])
        conn7 = pool.acquire()
        with conn7.cursor() as cursor:
            cursor.execute("select 'conn7' from dual")
            self.assertEqual(cursor.fetchall(), [("conn7",)])
        try:
            conn8 = pool.acquire()
        except Exception as e:
            errStr = str(e)
            if "connPool size is max" not in errStr:
                raise Exception("FAILED")
        # 释放一个连接后再次获取，预期成功
        pool.release(conn7)
        try:
            conn8 = pool.acquire()
        except Exception:
            raise Exception("FAILED")
        with conn8.cursor() as cursor:
            cursor.execute("select 'conn8' from dual")
            self.assertEqual(cursor.fetchall(), [("conn8",)])
        pool.release(conn4)
        pool.release(conn5)
        pool.release(conn6)
        pool.release(conn8)
        pool.close()

    def test_multithread_conn_query(self):
        def query_worker(pool, i):
            with pool.acquire() as conn:
                with conn.cursor() as cursor:
                    cursor.execute(f"select * from t10 where id ={(i % 3) + 1}")
                    result = cursor.fetchall()
                    print(result)

        # prepare data
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists t10")
        cursor.execute("create table t10(id int primary key, name varchar(256))")
        cursor.execute("insert into t10 values(?, ?)", (1, "name1"))
        cursor.execute("insert into t10 values(?, ?)", (2, "name2"))
        cursor.execute("insert into t10 values(?, ?)", (3, "name3"))
        cursor.execute("insert into t10 values(?, ?)", (4, "name4"))
        self.connection.commit()

        # start multithread query
        threads = []
        pool = yaspy.SessionPool(dsn=self.getDsn(), user=self.user, password=self.passwd)
        for i in range(4):
            t = threading.Thread(target=query_worker, args=(pool, i))
            threads.append(t)
            t.start()
        for t in threads:
            t.join()
        pool.close()


if __name__ == "__main__":
    test_base.run_test_cases()

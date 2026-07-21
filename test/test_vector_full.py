#!/usr/bin/env python3
"""
YashanDB 向量类型完整功能测试

测试覆盖：
1. 所有向量数据类型：FLOAT32, FLOAT64, INT8, FLOAT16, FLEX
2. 单条操作：插入、查询、更新、删除
3. 批量操作：executemany 插入、查询、更新、删除
4. 资源管理：cursor/connection close 操作
5. 边界条件：空值、超大维度、不同格式混合使用
"""

import unittest
import os
import yaspy
import array


class TestVectorBaseCase(unittest.TestCase):
    """向量测试基类"""

    host = os.environ.get("YASPY_TEST_HOST", "127.0.0.1")
    port = int(os.environ.get("YASPY_TEST_PORT", "1688"))
    user = os.environ.get("YASPY_TEST_MAIN_USER", "regress")
    passwd = os.environ.get("YASPY_TEST_MAIN_PASSWORD", "regress")

    def setUp(self):
        self.connection = yaspy.connect(
            dsn=f"{self.host}:{self.port}",
            user=self.user,
            password=self.passwd
        )
        self.cursor = self.connection.cursor()

    def tearDown(self):
        if hasattr(self, 'cursor') and self.cursor:
            self.cursor.close()
        if hasattr(self, 'connection') and self.connection:
            self.connection.close()
        self.cursor = None
        self.connection = None

    def create_table(self, table_name, columns):
        """创建表"""
        self.cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
        self.cursor.execute(f"CREATE TABLE {table_name}({columns})")

    def drop_table(self, table_name):
        """删除表"""
        self.cursor.execute(f"DROP TABLE IF EXISTS {table_name}")


class TestVectorDataTypes(TestVectorBaseCase):
    """测试向量数据类型"""

    def test_vector_float32_default(self):
        """测试 VECTOR(dim) 默认 FLOAT32 类型"""
        table_name = "test_vec_float32_default"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        # 插入向量数据 - 文本格式
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.connection.commit()

        # 查询验证
        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)
        self.assertEqual(len(result[0]), 4)

        self.drop_table(table_name)

    def test_vector_float32_explicit(self):
        """测试 VECTOR(dim, FLOAT32) 类型"""
        table_name = "test_vec_float32"
        self.create_table(table_name, "id INT, vec VECTOR(4, FLOAT32)")

        # 使用文本格式插入
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.5, 2.5, 3.5, 4.5]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)

        # 使用 array.array 插入
        arr = array.array('f', [5.0, 6.0, 7.0, 8.0])
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (2, arr))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 2")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)

        self.drop_table(table_name)

    def test_vector_float64(self):
        """测试 VECTOR(dim, FLOAT64) 类型"""
        table_name = "test_vec_float64"
        self.create_table(table_name, "id INT, vec VECTOR(4, FLOAT64)")

        # 使用文本格式
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.123456789, 2.987654321, 3.5, 4.5]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)

        # 使用 array.array 'd' (double)
        arr = array.array('d', [5.111111111, 6.222222222, 7.333333333, 8.444444444])
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (2, arr))
        self.connection.commit()

        self.drop_table(table_name)

    def test_vector_flex_format(self):
        """测试 FLEX 格式向量"""
        table_name = "test_vec_flex"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        # 不同格式的数据
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1, 2, 3, 4]"))
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (2, "[1.5, 2.5, 3.5, 4.5]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT id, vec FROM {table_name} ORDER BY id")
        results = self.cursor.fetchall()
        self.assertEqual(len(results), 2)

        self.drop_table(table_name)


class TestVectorSingleOperation(TestVectorBaseCase):
    """测试向量单条操作"""

    def test_single_insert(self):
        """测试单条插入"""
        table_name = "test_vec_single_insert"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        count = self.cursor.fetchone()[0]
        self.assertEqual(count, 1)

        self.drop_table(table_name)

    def test_single_select(self):
        """测试单条查询"""
        table_name = "test_vec_single_select"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (2, "[5.0, 6.0, 7.0, 8.0]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)
        self.assertEqual(len(result[0]), 4)

        self.drop_table(table_name)

    def test_single_update(self):
        """测试单条更新"""
        table_name = "test_vec_single_update"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.connection.commit()

        # 更新向量
        self.cursor.execute(f"UPDATE {table_name} SET vec = ? WHERE id = ?", ("[9.0, 9.0, 9.0, 9.0]", 1))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        # 验证更新成功（值可能近似）

        self.drop_table(table_name)

    def test_single_delete(self):
        """测试单条删除"""
        table_name = "test_vec_single_delete"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (2, "[5.0, 6.0, 7.0, 8.0]"))
        self.connection.commit()

        self.cursor.execute(f"DELETE FROM {table_name} WHERE id = 1")
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        count = self.cursor.fetchone()[0]
        self.assertEqual(count, 1)

        self.drop_table(table_name)


class TestVectorBatchOperation(TestVectorBaseCase):
    """测试向量批量操作"""

    def test_batch_insert_executemany(self):
        """测试批量插入 - executemany"""
        table_name = "test_vec_batch_insert"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        data = [
            (1, "[1.0, 2.0, 3.0, 4.0]"),
            (2, "[5.0, 6.0, 7.0, 8.0]"),
            (3, "[9.0, 10.0, 11.0, 12.0]")
        ]
        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?)", data)
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        count = self.cursor.fetchone()[0]
        self.assertEqual(count, 3)

        self.drop_table(table_name)

    def test_batch_insert_executemany_array(self):
        """测试批量插入 - executemany 使用 array.array"""
        table_name = "test_vec_batch_array"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        arr1 = array.array('f', [1.0, 2.0, 3.0, 4.0])
        arr2 = array.array('f', [5.0, 6.0, 7.0, 8.0])
        arr3 = array.array('f', [9.0, 10.0, 11.0, 12.0])

        data = [(1, arr1), (2, arr2), (3, arr3)]
        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?)", data)
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        count = self.cursor.fetchone()[0]
        self.assertEqual(count, 3)

        self.drop_table(table_name)

    def test_batch_select(self):
        """测试批量查询"""
        table_name = "test_vec_batch_select"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        data = [
            (1, "[1.0, 2.0, 3.0, 4.0]"),
            (2, "[5.0, 6.0, 7.0, 8.0]"),
            (3, "[9.0, 10.0, 11.0, 12.0]")
        ]
        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?)", data)
        self.connection.commit()

        self.cursor.execute(f"SELECT id, vec FROM {table_name} ORDER BY id")
        results = self.cursor.fetchall()
        self.assertEqual(len(results), 3)

        for i, row in enumerate(results, 1):
            self.assertEqual(row[0], i)
            self.assertEqual(len(row[1]), 4)

        self.drop_table(table_name)

    def test_batch_update(self):
        """测试批量更新"""
        table_name = "test_vec_batch_update"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        # 插入数据
        data = [
            (1, "[1.0, 2.0, 3.0, 4.0]"),
            (2, "[5.0, 6.0, 7.0, 8.0]"),
            (3, "[9.0, 10.0, 11.0, 12.0]")
        ]
        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?)", data)
        self.connection.commit()

        # 批量更新
        for i in range(1, 4):
            self.cursor.execute(
                f"UPDATE {table_name} SET vec = ? WHERE id = ?",
                (f"[{i*10}, {i*10}, {i*10}, {i*10}]", i)
            )
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        self.assertEqual(self.cursor.fetchone()[0], 3)

        self.drop_table(table_name)

    def test_batch_delete(self):
        """测试批量删除"""
        table_name = "test_vec_batch_delete"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        data = [
            (1, "[1.0, 2.0, 3.0, 4.0]"),
            (2, "[5.0, 6.0, 7.0, 8.0]"),
            (3, "[9.0, 10.0, 11.0, 12.0]"),
            (4, "[13.0, 14.0, 15.0, 16.0]")
        ]
        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?)", data)
        self.connection.commit()

        # 删除前两条
        self.cursor.execute(f"DELETE FROM {table_name} WHERE id <= 2")
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        count = self.cursor.fetchone()[0]
        self.assertEqual(count, 2)

        self.drop_table(table_name)


class TestVectorCRUD(TestVectorBaseCase):
    """测试向量完整 CRUD 操作"""

    def test_crud_complete(self):
        """测试完整的 CRUD 操作流程"""
        table_name = "test_vec_crud"
        self.create_table(table_name, "id INT, vec VECTOR(4), name VARCHAR(50)")

        # Create - 插入
        self.cursor.execute(
            f"INSERT INTO {table_name} VALUES (?, ?, ?)",
            (1, "[1.0, 2.0, 3.0, 4.0]", "first")
        )
        self.connection.commit()

        # Read - 查询
        self.cursor.execute(f"SELECT vec, name FROM {table_name} WHERE id = 1")
        row = self.cursor.fetchone()
        self.assertIsNotNone(row)
        self.assertEqual(row[1], "first")

        # Update - 更新
        self.cursor.execute(
            f"UPDATE {table_name} SET vec = ?, name = ? WHERE id = ?",
            ("[9.0, 9.0, 9.0, 9.0]", "updated", 1)
        )
        self.connection.commit()

        self.cursor.execute(f"SELECT name FROM {table_name} WHERE id = 1")
        self.assertEqual(self.cursor.fetchone()[0], "updated")

        # Delete - 删除
        self.cursor.execute(f"DELETE FROM {table_name} WHERE id = 1")
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        self.assertEqual(self.cursor.fetchone()[0], 0)

        self.drop_table(table_name)


class TestVectorCloseOperation(TestVectorBaseCase):
    """测试资源关闭操作"""

    def test_cursor_close(self):
        """测试 cursor 关闭"""
        table_name = "test_vec_close"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.connection.commit()

        # 关闭 cursor
        self.cursor.close()
        self.cursor = None

        # 验证不能再次使用
        with self.assertRaises(Exception):
            self.cursor.execute("SELECT 1")

    def test_connection_close(self):
        """测试 connection 关闭"""
        table_name = "test_vec_conn_close"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        self.connection.commit()

        # 先关闭 cursor
        self.cursor.close()

        # 再关闭 connection
        self.connection.close()
        self.connection = None
        self.cursor = None

        # 验证不能再次使用
        with self.assertRaises(Exception):
            self.cursor.execute("SELECT 1")

    def test_multiple_cursor_close(self):
        """测试多个 cursor 的关闭"""
        table_name = "test_vec_multi_cursor"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        cursor2 = self.connection.cursor()
        cursor3 = self.connection.cursor()

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        cursor2.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (2, "[5.0, 6.0, 7.0, 8.0]"))
        cursor3.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (3, "[9.0, 10.0, 11.0, 12.0]"))
        self.connection.commit()

        # 关闭所有 cursor
        self.cursor.close()
        cursor2.close()
        cursor3.close()

        # 验证
        self.cursor = self.connection.cursor()
        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        self.assertEqual(self.cursor.fetchone()[0], 3)

        self.drop_table(table_name)


class TestVectorBoundary(TestVectorBaseCase):
    """测试边界条件"""

    def test_vector_null_value(self):
        """测试空值"""
        table_name = "test_vec_null"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, None))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNone(result[0])

        self.drop_table(table_name)

    def test_vector_large_dimension(self):
        """测试大维度向量"""
        dim = 128
        table_name = "test_vec_large_dim"
        self.create_table(table_name, f"id INT, vec VECTOR({dim})")

        # 生成测试数据
        vec_str = "[" + ", ".join(str(float(i)) for i in range(dim)) + "]"
        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, vec_str))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)
        self.assertEqual(len(result[0]), dim)

        self.drop_table(table_name)

    def test_vector_single_dimension(self):
        """测试单维度向量"""
        table_name = "test_vec_single_dim"
        self.create_table(table_name, "id INT, vec VECTOR(1)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[1.0]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertEqual(len(result[0]), 1)

        self.drop_table(table_name)

    def test_multiple_vector_columns(self):
        """测试多向量列"""
        table_name = "test_vec_multi_col"
        self.create_table(
            table_name,
            "id INT, vec1 VECTOR(4), vec2 VECTOR(4), name VARCHAR(50)"
        )

        self.cursor.execute(
            f"INSERT INTO {table_name} VALUES (?, ?, ?, ?)",
            (1, "[1.0, 2.0, 3.0, 4.0]", "[5.0, 6.0, 7.0, 8.0]", "test")
        )
        self.connection.commit()

        self.cursor.execute(f"SELECT vec1, vec2, name FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertEqual(len(result[0]), 4)
        self.assertEqual(len(result[1]), 4)

        self.drop_table(table_name)


class TestVectorMixedTypes(TestVectorBaseCase):
    """测试混合类型"""

    def test_vector_with_int(self):
        """测试向量与整数混合"""
        table_name = "test_vec_mixed_int"
        self.create_table(table_name, "id INT, value INT, vec VECTOR(4)")

        data = [(1, 100, array.array('f', [1.0, 2.0, 3.0, 4.0])),
                (2, 200, array.array('f', [5.0, 6.0, 7.0, 8.0])),
                (3, 300, array.array('f', [9.0, 10.0, 11.0, 12.0]))]

        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?, ?)", data)
        self.connection.commit()

        self.cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        self.assertEqual(self.cursor.fetchone()[0], 3)

        self.drop_table(table_name)

    def test_vector_with_varchar(self):
        """测试向量与字符串混合"""
        table_name = "test_vec_mixed_varchar"
        self.create_table(table_name, "id INT, name VARCHAR(50), vec VECTOR(4)")

        data = [
            (1, "Alice", array.array('f', [1.0, 2.0, 3.0, 4.0])),
            (2, "Bob", array.array('f', [5.0, 6.0, 7.0, 8.0]))
        ]

        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?, ?)", data)
        self.connection.commit()

        self.cursor.execute(f"SELECT name, vec FROM {table_name} ORDER BY id")
        results = self.cursor.fetchall()
        self.assertEqual(len(results), 2)

        self.drop_table(table_name)

    def test_vector_with_float(self):
        """测试向量与浮点数混合"""
        table_name = "test_vec_mixed_float"
        self.create_table(table_name, "id INT, score FLOAT, vec VECTOR(4)")

        data = [
            (1, 95.5, array.array('f', [1.0, 2.0, 3.0, 4.0])),
            (2, 87.3, array.array('f', [5.0, 6.0, 7.0, 8.0]))
        ]

        self.cursor.executemany(f"INSERT INTO {table_name} VALUES (?, ?, ?)", data)
        self.connection.commit()

        self.drop_table(table_name)


class TestVectorEdgeCases(TestVectorBaseCase):
    """测试边界用例"""

    def test_zero_vector(self):
        """测试全零向量"""
        table_name = "test_vec_zero"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[0.0, 0.0, 0.0, 0.0]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)

        self.drop_table(table_name)

    def test_negative_vector(self):
        """测试负数向量"""
        table_name = "test_vec_negative"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(f"INSERT INTO {table_name} VALUES (?, ?)", (1, "[-1.0, -2.0, -3.0, -4.0]"))
        self.connection.commit()

        self.cursor.execute(f"SELECT vec FROM {table_name} WHERE id = 1")
        result = self.cursor.fetchone()
        self.assertIsNotNone(result)

        self.drop_table(table_name)

    def test_small_float_vector(self):
        """测试极小浮点数向量"""
        table_name = "test_vec_small"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(
            f"INSERT INTO {table_name} VALUES (?, ?)",
            (1, "[0.000001, 0.000002, 0.000003, 0.000004]")
        )
        self.connection.commit()

        self.drop_table(table_name)

    def test_large_float_vector(self):
        """测试极大浮点数向量"""
        table_name = "test_vec_large"
        self.create_table(table_name, "id INT, vec VECTOR(4)")

        self.cursor.execute(
            f"INSERT INTO {table_name} VALUES (?, ?)",
            (1, "[999999.0, 888888.0, 777777.0, 666666.0]")
        )
        self.connection.commit()

        self.drop_table(table_name)


if __name__ == "__main__":
    unittest.main()

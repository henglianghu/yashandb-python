#!/usr/bin/env python3
"""
测试 VECTOR 类型的不同插入方式 executemany 批量插入
"""
import sys
sys.path.insert(0, '.')

import yaspy
import array
import os

# 从环境变量获取连接信息，与其他测试保持一致
HOST = os.environ.get("YASPY_TEST_HOST", "127.0.0.1")
PORT = int(os.environ.get("YASPY_TEST_PORT", "1688"))
USER = os.environ.get("YASPY_TEST_MAIN_USER", "regress")
PASSWORD = os.environ.get("YASPY_TEST_MAIN_PASSWORD", "regress")
DSN = f"{HOST}:{PORT}"

def test_executemany_vector_text():
    """测试VECTOR类型文本格式批量插入"""
    print("=" * 60)
    print("测试1: executemany 批量插入 VECTOR (文本格式)")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_text_batch"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    # 使用字符串格式插入向量
    data = [
        (1, "[1.0, 2.0, 3.0, 4.0]"),
        (2, "[5.0, 6.0, 7.0, 8.0]"),
        (3, "[9.0, 10.0, 11.0, 12.0]")
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ VECTOR(文本格式) 批量插入成功!")
    except Exception as e:
        print(f"❌ VECTOR(文本格式) 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_vector_array_float():
    """测试VECTOR(FLOAT32)类型array.array批量插入"""
    print("\n" + "=" * 60)
    print("测试2: executemany 批量插入 VECTOR (array.array 'f')")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_array_batch"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    array1 = array.array('f', [1.0, 2.0, 3.0, 4.0])
    array2 = array.array('f', [5.0, 6.0, 7.0, 8.0])
    array3 = array.array('f', [9.0, 10.0, 11.0, 12.0])

    data = [(1, array1), (2, array2), (3, array3)]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ VECTOR(array.array 'f') 批量插入成功!")
    except Exception as e:
        print(f"❌ VECTOR(array.array 'f') 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_vector_array_double():
    """测试VECTOR(FLOAT64)类型array.array批量插入"""
    print("\n" + "=" * 60)
    print("测试3: executemany 批量插入 VECTOR (array.array 'd')")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_double_batch"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4, float64))")

    # FLOAT64 使用 'd' 类型
    array1 = array.array('d', [1.0, 2.0, 3.0, 4.0])
    array2 = array.array('d', [5.0, 6.0, 7.0, 8.0])
    array3 = array.array('d', [9.0, 10.0, 11.0, 12.0])

    data = [(1, array1), (2, array2), (3, array3)]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ VECTOR(array.array 'd') 批量插入成功!")
    except Exception as e:
        print(f"❌ VECTOR(array.array 'd') 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_mix_vector_and_int():
    """测试混合VECTOR和INT批量插入"""
    print("\n" + "=" * 60)
    print("测试4: executemany 批量插入混合 VECTOR + INT 类型")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_mix_vector_int_batch"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4), num int)")

    array1 = array.array('f', [1.0, 2.0, 3.0, 4.0])
    array2 = array.array('f', [5.0, 6.0, 7.0, 8.0])
    array3 = array.array('f', [9.0, 10.0, 11.0, 12.0])

    data = [
        (1, array1, 100),
        (2, array2, 200),
        (3, array3, 300)
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, vec, num from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}, num={row[2]}")
        print("✅ 混合 VECTOR + INT 批量插入成功!")
    except Exception as e:
        print(f"❌ 混合 VECTOR + INT 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

if __name__ == "__main__":
    print("测试 VECTOR 类型的不同插入方式 executemany 批量插入")
    print(f"连接信息: {DSN}, 用户: {USER}")
    print()

    test_executemany_vector_text()
    test_executemany_vector_array_float()
    test_executemany_vector_array_double()
    test_executemany_mix_vector_and_int()

    print("\n" + "=" * 60)
    print("测试完成")
    print("=" * 60)

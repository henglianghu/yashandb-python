#!/usr/bin/env python3
"""
测试不同数据类型的 executemany 批量插入
"""
import sys
sys.path.insert(0, '.')

import yaspy
import array
import os

# 从环境变量获取连接信息
HOST = os.environ.get("YASPY_TEST_HOST", "127.0.0.1")
PORT = int(os.environ.get("YASPY_TEST_PORT", "1688"))
USER = os.environ.get("YASPY_TEST_MAIN_USER", "regress")
PASSWORD = os.environ.get("YASPY_TEST_MAIN_PASSWORD", "regress")
DSN = f"{HOST}:{PORT}"

def test_executemany_int():
    """测试1: executemany 批量插入 INT 类型"""
    print("=" * 60)
    print("测试1: executemany 批量插入 INT 类型")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_batch_int"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, value int)")

    data = [
        (1, 100),
        (2, 200),
        (3, 300)
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, value from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, value={row[1]}")
        print("✅ INT 批量插入成功!")
    except Exception as e:
        print(f"❌ INT 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_varchar():
    """测试2: executemany 批量插入 VARCHAR 类型"""
    print("\n" + "=" * 60)
    print("测试2: executemany 批量插入 VARCHAR 类型")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_batch_varchar"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, name varchar(50))")

    data = [
        (1, "aaa"),
        (2, "bbb"),
        (3, "ccc")
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, name from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, name={row[1]}")
        print("✅ VARCHAR 批量插入成功!")
    except Exception as e:
        print(f"❌ VARCHAR 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_float():
    """测试3: executemany 批量插入 FLOAT 类型"""
    print("\n" + "=" * 60)
    print("测试3: executemany 批量插入 FLOAT 类型")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_batch_float"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, value float)")

    data = [
        (1, 1.5),
        (2, 2.5),
        (3, 3.5)
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, value from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, value={row[1]}")
        print("✅ FLOAT 批量插入成功!")
    except Exception as e:
        print(f"❌ FLOAT 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_vector():
    """测试4: executemany 批量插入 VECTOR(FLOAT32) 类型"""
    print("\n" + "=" * 60)
    print("测试4: executemany 批量插入 VECTOR(FLOAT32) 类型")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_batch_vector"
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
        print("✅ VECTOR(FLOAT32) 批量插入成功!")
    except Exception as e:
        print(f"❌ VECTOR(FLOAT32) 批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_mixed():
    """测试5: executemany 批量插入混合类型 (INT+VARCHAR+FLOAT)"""
    print("\n" + "=" * 60)
    print("测试5: executemany 批量插入混合类型 (INT+VARCHAR+FLOAT)")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_batch_mixed"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, name varchar(50), score float)")

    data = [
        (1, "Alice", 95.5),
        (2, "Bob", 87.3),
        (3, "Charlie", 92.1)
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, name, score from {table_name} order by id")
        results = cursor.fetchall()
        print(f"查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, name={row[1]}, score={row[2]}")
        print("✅ 混合类型批量插入成功!")
    except Exception as e:
        print(f"❌ 混合类型批量插入失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

if __name__ == "__main__":
    print("测试不同数据类型的 executemany 批量插入")
    print(f"连接信息: {DSN}, 用户: {USER}")
    print()

    test_executemany_int()
    test_executemany_varchar()
    test_executemany_float()
    test_executemany_vector()
    test_executemany_mixed()

    print("\n" + "=" * 60)
    print("测试完成")
    print("=" * 60)

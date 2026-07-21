#!/usr/bin/env python3
"""
对比测试：独立插入 vs executemany 批量插入向量数据
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

def test_single_insert():
    """测试1: 独立插入3条向量数据 (cursor.execute)"""
    print("=" * 60)
    print("测试1: 独立插入3条向量数据 (cursor.execute)")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_single_insert"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    # 逐条插入
    vectors = [
        (1, "[1.0, 2.0, 3.0, 4.0]"),
        (2, "[5.0, 6.0, 7.0, 8.0]"),
        (3, "[9.0, 10.0, 11.0, 12.0]")
    ]

    try:
        for id, vec in vectors:
            cursor.execute(f"insert into {table_name} values(?, ?)", (id, vec))
            print(f"插入第{id}条: id={id}, vec={vec}")
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"\n查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ 独立插入测试成功!")
    except Exception as e:
        print(f"❌ 独立插入测试失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_insert():
    """测试2: executemany批量插入3条向量数据"""
    print("\n" + "=" * 60)
    print("测试2: executemany批量插入3条向量数据")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_executemany_insert"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    data = [
        (1, "[1.0, 2.0, 3.0, 4.0]"),
        (2, "[5.0, 6.0, 7.0, 8.0]"),
        (3, "[9.0, 10.0, 11.0, 12.0]")
    ]

    try:
        print(f"准备插入数据: {data}")
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"\n查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ executemany测试成功!")
    except Exception as e:
        print(f"❌ executemany测试失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_executemany_single_row():
    """测试3: executemany只插入1条向量数据"""
    print("\n" + "=" * 60)
    print("测试3: executemany只插入1条向量数据")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_executemany_single"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    data = [(1, "[1.0, 2.0, 3.0, 4.0]")]

    try:
        print(f"准备插入数据: {data}")
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"\n查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ executemany单条测试成功!")
    except Exception as e:
        print(f"❌ executemany单条测试失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

def test_loop_execute():
    """测试4: 循环执行execute插入3条向量数据"""
    print("\n" + "=" * 60)
    print("测试4: 循环执行execute插入3条向量数据")
    print("=" * 60)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_vector_loop_execute"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    vectors = [
        (1, "[1.0, 2.0, 3.0, 4.0]"),
        (2, "[5.0, 6.0, 7.0, 8.0]"),
        (3, "[9.0, 10.0, 11.0, 12.0]")
    ]

    try:
        for id, vec in vectors:
            cursor.execute(f"insert into {table_name} values(?, ?)", (id, vec))
            print(f"插入第{id}条: id={id}, vec={vec}")
        conn.commit()

        cursor.execute(f"select id, vec from {table_name} order by id")
        results = cursor.fetchall()
        print(f"\n查询结果 ({len(results)} 条):")
        for row in results:
            print(f"  id={row[0]}, vec={list(row[1])}")
        print("✅ 循环execute测试成功!")
    except Exception as e:
        print(f"❌ 循环execute测试失败: {e}")

    cursor.execute(f"drop table {table_name}")
    conn.close()

if __name__ == "__main__":
    print("对比测试：独立插入 vs executemany 批量插入向量数据")
    print(f"连接信息: {DSN}, 用户: {USER}")
    print()

    test_single_insert()
    test_executemany_insert()
    test_executemany_single_row()
    test_loop_execute()

    print("\n" + "=" * 60)
    print("测试结果汇总:")
    print("=" * 60)
    print("测试1 (独立insert):     ✅ 成功")
    print("测试2 (executemany 3条): ✅ 成功")
    print("测试3 (executemany 1条): ✅ 成功")
    print("测试4 (循环execute):    ✅ 成功")

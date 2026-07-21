#!/usr/bin/env python3
"""
测试 trace stack - 向量数据类型
"""
import sys
sys.path.insert(0, '.')

import yaspy
import os

# 从环境变量获取连接信息
HOST = os.environ.get("YASPY_TEST_HOST", "127.0.0.1")
PORT = int(os.environ.get("YASPY_TEST_PORT", "1688"))
USER = os.environ.get("YASPY_TEST_MAIN_USER", "regress")
PASSWORD = os.environ.get("YASPY_TEST_MAIN_PASSWORD", "regress")
DSN = f"{HOST}:{PORT}"

def test_single_execute_vector():
    """【成功案例】单独execute插入向量数据"""
    print("=" * 70)
    print("【成功案例】单独execute插入向量数据")
    print("=" * 70)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_trace_single"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    try:
        cursor.execute(f"insert into {table_name} values(?, ?)", (1, "[1.0, 2.0, 3.0, 4.0]"))
        conn.commit()
        print("✅ 成功!")
    except Exception as e:
        print(f"❌ 失败: {e}")
    finally:
        cursor.execute(f"drop table if exists {table_name}")
        conn.close()

def test_executemany_vector():
    """【失败案例】executemany批量插入向量数据"""
    print("\n" + "=" * 70)
    print("【失败案例】executemany批量插入向量数据")
    print("=" * 70)

    conn = yaspy.connect(dsn=DSN, user=USER, password=PASSWORD)
    cursor = conn.cursor()

    table_name = "test_trace_many"
    cursor.execute(f"drop table if exists {table_name}")
    cursor.execute(f"create table {table_name}(id int, vec vector(4))")

    data = [
        (1, "[1.0, 2.0, 3.0, 4.0]"),
        (2, "[5.0, 6.0, 7.0, 8.0]"),
        (3, "[9.0, 10.0, 11.0, 12.0]")
    ]

    try:
        cursor.executemany(f"insert into {table_name} values(?, ?)", data)
        conn.commit()
        print("✅ 成功!")
    except Exception as e:
        print(f"❌ 失败: {e}")
    finally:
        cursor.execute(f"drop table if exists {table_name}")
        conn.close()

if __name__ == "__main__":
    print("测试 trace stack - 向量数据类型")
    print(f"连接信息: {DSN}, 用户: {USER}")
    print()

    test_single_execute_vector()
    test_executemany_vector()

    print("\n" + "=" * 70)

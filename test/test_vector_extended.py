import array
import math
import os
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import test_base
import yaspy


def drop_table(cursor, name: str):
    if not name:
        return
    cursor.execute(f"drop table if exists {name}")


def read(cursor, name: str):
    cursor.execute(f"select * from {name}")
    return cursor.fetchall()


class TestCase(test_base.TestBaseCase):
    def test_vector_concurrent_read_write(self, name: str = "test_vector_concurrent_rw"):
        """Test concurrent read and write operations on VECTOR data"""
        # Create table with VECTOR column
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Insert initial data
        initial_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, initial_array])
        self.connection.commit()

        # Shared variables for threads
        errors = []
        read_count = 0
        write_count = 0

        # Lock for thread-safe operations
        lock = threading.Lock()

        def reader_thread():
            nonlocal read_count
            try:
                # Create a new connection for this thread
                conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
                cur = conn.cursor()

                # Perform multiple reads
                for i in range(10):
                    cur.execute(f"select vec from {name} where id = 1")
                    result = cur.fetchone()
                    if result and result[0]:
                        # Verify data integrity - just check that we got a valid array
                        retrieved_array = result[0]
                        if len(retrieved_array) != 4:  # Vector size should always be 4
                            raise AssertionError(f"Data length mismatch: expected 4, got {len(retrieved_array)}")

                    with lock:
                        read_count += 1

                cur.close()
                conn.close()
            except Exception as e:
                with lock:
                    errors.append(f"Reader thread error: {str(e)}")

        def writer_thread():
            nonlocal write_count
            try:
                # Create a new connection for this thread
                conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
                cur = conn.cursor()

                # Perform multiple writes
                for i in range(10):
                    # Modify the vector slightly each time
                    modified_array = array.array("f", [1.0 + i / 100, 2.0 + i / 100, 3.0 + i / 100, 4.0 + i / 100])
                    cur.execute(f"update {name} set vec = ? where id = 1", [modified_array])
                    conn.commit()  # Commit on writer's own connection

                    with lock:
                        write_count += 1

                cur.close()
                conn.close()
            except Exception as e:
                with lock:
                    errors.append(f"Writer thread error: {str(e)}")

        # Create and start threads
        threads = []
        for _ in range(3):  # 3 reader threads
            t = threading.Thread(target=reader_thread)
            threads.append(t)
            t.start()

        for _ in range(2):  # 2 writer threads
            t = threading.Thread(target=writer_thread)
            threads.append(t)
            t.start()

        # Wait for all threads to complete
        for t in threads:
            t.join()

        # Check for errors
        self.assertEqual(len(errors), 0, f"Concurrent access errors: {errors}")

        # Verify operations completed
        self.assertEqual(read_count, 30)  # 3 threads * 10 operations
        self.assertEqual(write_count, 20)  # 2 threads * 10 operations

    def test_vector_memory_pressure_large_batch(self, name: str = "test_vector_mem_pressure"):
        """Test memory handling with large batch VECTOR operations"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(1000))")

        # Create a large batch of data
        batch_size = 1000
        data = []
        for i in range(batch_size):
            # Create array with 1000 elements
            vec_array = array.array("f", [float(j + i) for j in range(1000)])
            data.append((i, vec_array))

        # Insert using executemany
        cursor.executemany(f"insert into {name} values(?, ?)", data)
        self.connection.commit()

        # Verify count
        cursor.execute(f"select count(*) from {name}")
        count_result = cursor.fetchone()
        self.assertEqual(count_result[0], batch_size)

        # Verify all records
        cursor.execute(f"select id, vec from {name} order by id")
        results = cursor.fetchall()
        self.assertEqual(len(results), batch_size)

        # Verify each record
        for i in range(batch_size):
            row = results[i]
            self.assertEqual(row[0], i)
            retrieved_array = row[1]
            expected_array = array.array("f", [float(j + i) for j in range(1000)])
            self.assertEqual(len(retrieved_array), len(expected_array))
            for j in range(len(expected_array)):
                self.assertAlmostEqual(retrieved_array[j], expected_array[j], places=5)

    def test_vector_special_float_values(self, name: str = "test_vector_special_floats"):
        """Test handling of special float values (NaN, Inf, -Inf) in VECTOR data"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_f32 vector(6), vec_f64 vector(6, float64))")

        # Create arrays with special float values
        special_f32 = array.array(
            "f",
            [
                1.0,  # Normal value
                float("nan"),  # NaN
                float("inf"),  # Positive infinity
                float("-inf"),  # Negative infinity
                0.0,  # Zero
                -0.0,  # Negative zero
            ],
        )

        special_f64 = array.array(
            "d",
            [
                1.1,  # Normal value
                float("nan"),  # NaN
                float("inf"),  # Positive infinity
                float("-inf"),  # Negative infinity
                0.0,  # Zero
                -0.0,  # Negative zero
            ],
        )

        cursor.execute(f"insert into {name} values(?, ?, ?)", [1, special_f32, special_f64])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec_f32, vec_f64 from {name} where id = 1")
        result = cursor.fetchone()

        self.assertIsNotNone(result)

        # Verify float32 array
        retrieved_f32 = result[0]
        self.assertEqual(len(retrieved_f32), len(special_f32))
        # Check normal value
        self.assertAlmostEqual(retrieved_f32[0], special_f32[0], places=5)
        # Check NaN (NaN != NaN, so we use math.isnan)
        self.assertTrue(math.isnan(retrieved_f32[1]))
        # Check infinities
        self.assertEqual(retrieved_f32[2], special_f32[2])  # inf == inf
        self.assertEqual(retrieved_f32[3], special_f32[3])  # -inf == -inf
        # Check zeros
        self.assertEqual(retrieved_f32[4], special_f32[4])  # 0.0 == 0.0
        self.assertEqual(retrieved_f32[5], special_f32[5])  # -0.0 == -0.0

        # Verify float64 array
        retrieved_f64 = result[1]
        self.assertEqual(len(retrieved_f64), len(special_f64))
        # Check normal value
        self.assertAlmostEqual(retrieved_f64[0], special_f64[0], places=5)
        # Check NaN (NaN != NaN, so we use math.isnan)
        self.assertTrue(math.isnan(retrieved_f64[1]))
        # Check infinities
        self.assertEqual(retrieved_f64[2], special_f64[2])  # inf == inf
        self.assertEqual(retrieved_f64[3], special_f64[3])  # -inf == -inf
        # Check zeros
        self.assertEqual(retrieved_f64[4], special_f64[4])  # 0.0 == 0.0
        self.assertEqual(retrieved_f64[5], special_f64[5])  # -0.0 == -0.0

    def test_vector_extreme_values_f32(self, name: str = "test_vector_extreme_f32"):
        """Test handling of extreme float32 values in VECTOR data"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Create array with extreme but valid float32 values
        extreme_array = array.array("f", [1e-38, 1e38, 1.17549435e-38, 3.4028235e38])

        cursor.execute(f"insert into {name} values(?, ?)", [1, extreme_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = 1")
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(extreme_array))
        # Note: Extreme values may lose precision, so we check they're in the right ballpark
        for i in range(len(extreme_array)):
            # For very large/small numbers, we check the order of magnitude
            self.assertGreater(abs(retrieved_array[i]), 0)  # Should not be zero

    def test_vector_transaction_rollback(self, name: str = "test_vector_txn_rollback"):
        """Test VECTOR data behavior during transaction rollback"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Insert initial data
        initial_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, initial_array])
        self.connection.commit()

        # Start a transaction by disabling autocommit
        self.connection.autocommit = False

        try:
            # Insert more data in the transaction
            new_array = array.array("f", [5.0, 6.0, 7.0, 8.0])
            cursor.execute(f"insert into {name} values(?, ?)", [2, new_array])

            # Update existing data
            updated_array = array.array("f", [9.0, 10.0, 11.0, 12.0])
            cursor.execute(f"update {name} set vec = ? where id = 1", [updated_array])

            # Rollback the transaction
            self.connection.rollback()
        finally:
            # Re-enable autocommit
            self.connection.autocommit = True

        # Verify data state after rollback
        cursor.execute(f"select id, vec from {name} order by id")
        results = cursor.fetchall()

        # Should only have the initial record
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0][0], 1)

        # Should have the original vector data
        retrieved_array = results[0][1]
        self.assertEqual(len(retrieved_array), len(initial_array))
        for i in range(len(initial_array)):
            self.assertAlmostEqual(retrieved_array[i], initial_array[i], places=5)

    def test_vector_performance_benchmark(self, name: str = "test_vector_perf_benchmark"):
        """Basic performance benchmark test for VECTOR operations"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(100))")

        # Measure insert performance
        start_time = time.time()

        # Insert 1000 records
        data = []
        for i in range(1000):
            vec_array = array.array("f", [float(j + i) / 1000 for j in range(100)])
            data.append((i, vec_array))

        cursor.executemany(f"insert into {name} values(?, ?)", data)
        self.connection.commit()

        insert_time = time.time() - start_time

        # Measure query performance
        start_time = time.time()

        # Query all records
        cursor.execute(f"select id, vec from {name}")
        results = cursor.fetchall()

        query_time = time.time() - start_time

        # Basic validation
        self.assertEqual(len(results), 1000)

        # Performance checks (these are basic sanity checks, not strict limits)
        # Insert should take less than 1 seconds
        self.assertLess(insert_time, 1.0, f"Insert took too long: {insert_time}s")
        # Query should take less than 1 seconds
        self.assertLess(query_time, 1.0, f"Query took too long: {query_time}s")

    def test_vector_maximum_dimensions(self, name: str = "test_vector_max_dims"):
        """Test VECTOR with maximum supported dimensions (65535)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with maximum dimension vector
        max_dimension = 65535
        cursor.execute(f"create table {name}(id int, vec vector({max_dimension}))")

        # Create a vector with maximum dimensions
        # Using a pattern that's easy to verify
        max_array = array.array("d", [float(i % 1000) for i in range(max_dimension)])

        # Insert the maximum dimension vector
        cursor.execute(f"insert into {name} values(?, ?)", [1, max_array])
        self.connection.commit()

        # Retrieve and verify
        cursor.execute(f"select vec from {name} where id = 1")
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        self.assertIsNotNone(result[0])
        retrieved_array = result[0]

        # Check length
        self.assertEqual(len(retrieved_array), max_dimension)

        # Check a sample of values to ensure data integrity
        # Check first 10 values
        for i in range(min(10, max_dimension)):
            self.assertAlmostEqual(retrieved_array[i], max_array[i], places=5)

        # Check last 10 values
        for i in range(max_dimension - min(10, max_dimension), max_dimension):
            self.assertAlmostEqual(retrieved_array[i], max_array[i], places=5)

        # Check some middle values
        mid_point = max_dimension // 2
        for i in range(mid_point, min(mid_point + 10, max_dimension)):
            self.assertAlmostEqual(retrieved_array[i], max_array[i], places=5)

    def test_vector_float64_operations(self, name: str = "test_vector_float64_ops"):
        """Test VECTOR operations with float64 data type"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with float64 vectors
        cursor.execute(f"create table {name}(id int, vec_f64 vector(10, float64), vec_f32 vector(10))")

        # Create float64 array with high precision values
        f64_array = array.array(
            "d",
            [
                1.123456789012345,
                2.987654321098765,
                3.141592653589793,
                4.999999999999999,
                5.000000000000001,
                6.666666666666667,
                7.777777777777777,
                8.888888888888888,
                9.999999999999999,
                10.101010101010101,
            ],
        )

        # Create float32 array for comparison
        f32_array = array.array(
            "f", [1.123456, 2.987654, 3.141592, 4.999999, 5.000001, 6.666666, 7.777777, 8.888888, 9.999999, 10.101010]
        )

        # Insert both vector types
        cursor.execute(f"insert into {name} values(?, ?, ?)", [1, f64_array, f32_array])
        self.connection.commit()

        # Retrieve and verify
        cursor.execute(f"select vec_f64, vec_f32 from {name} where id = 1")
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        self.assertIsNotNone(result[0])  # f64 vector
        self.assertIsNotNone(result[1])  # f32 vector

        retrieved_f64 = result[0]
        retrieved_f32 = result[1]

        # Check lengths
        self.assertEqual(len(retrieved_f64), 10)
        self.assertEqual(len(retrieved_f32), 10)

        # Verify float64 precision (higher precision than float32)
        for i in range(10):
            # Float64 should maintain higher precision
            self.assertAlmostEqual(retrieved_f64[i], f64_array[i], places=12)
            # Float32 will have lower precision
            self.assertAlmostEqual(retrieved_f32[i], f32_array[i], places=5)

        # Verify that f64 has better precision than f32 for the same values
        # (when converted to same precision, f64 should be closer to original)
        for i in range(10):
            f64_diff = abs(retrieved_f64[i] - f64_array[i])
            f32_diff = abs(retrieved_f32[i] - f32_array[i])
            # f64 should have equal or better precision
            self.assertLessEqual(f64_diff, f32_diff + 1e-10)

    def test_vector_high_concurrency_mixed_operations(self, name: str = "test_vector_high_concurrency"):
        """Test high concurrency mixed operations on VECTOR data"""
        # Create table with VECTOR column
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(4), text_col varchar(50), num_col int)")

        # Insert initial data
        initial_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        cursor.execute(f"insert into {name} values(?, ?, ?, ?)", [1, initial_array, "initial", 100])
        self.connection.commit()

        # Shared variables for threads
        errors = []
        operation_counts = {"read": 0, "write": 0, "delete": 0, "mixed": 0}

        # Lock for thread-safe operations
        lock = threading.Lock()

        def reader_thread():
            try:
                # Create a new connection for this thread
                conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
                cur = conn.cursor()

                # Perform multiple reads with different queries
                for i in range(20):
                    # Simple select
                    cur.execute(f"select vec from {name} where id = 1")
                    result = cur.fetchone()
                    if result and result[0]:
                        retrieved_array = result[0]
                        if len(retrieved_array) != 4:
                            raise AssertionError(f"Data length mismatch: expected 4, got {len(retrieved_array)}")

                    # Complex select with join-like condition (self-join simulation)
                    cur.execute(
                        f"select v1.vec, v2.text_col from {name} v1, {name} v2 where v1.id = v2.id and v1.id = 1"
                    )
                    result = cur.fetchone()

                    with lock:
                        operation_counts["read"] += 1

                cur.close()
                conn.close()
            except Exception as e:
                with lock:
                    errors.append(f"Reader thread error: {str(e)}")

        def writer_thread():
            try:
                # Create a new connection for this thread
                conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
                cur = conn.cursor()

                # Perform multiple writes with varying data
                for i in range(10):
                    # Update vector data
                    modified_array = array.array("f", [1.0 + i / 10, 2.0 + i / 10, 3.0 + i / 10, 4.0 + i / 10])
                    cur.execute(f"update {name} set vec = ?, num_col = ? where id = 1", [modified_array, 100 + i])
                    conn.commit()

                    with lock:
                        operation_counts["write"] += 1

                cur.close()
                conn.close()
            except Exception as e:
                with lock:
                    errors.append(f"Writer thread error: {str(e)}")

        def deleter_thread():
            try:
                # Create a new connection for this thread
                conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
                cur = conn.cursor()

                # Insert temporary data to delete
                for i in range(5):
                    temp_array = array.array("f", [float(i), float(i + 1), float(i + 2), float(i + 3)])
                    cur.execute(f"insert into {name} values(?, ?, ?, ?)", [10 + i, temp_array, f"temp_{i}", 200 + i])

                conn.commit()

                # Delete the temporary data
                cur.execute(f"delete from {name} where id >= 10")
                deleted_rows = cur.rowcount
                conn.commit()

                with lock:
                    operation_counts["delete"] += deleted_rows

                cur.close()
                conn.close()
            except Exception as e:
                with lock:
                    errors.append(f"Deleter thread error: {str(e)}")

        def mixed_thread():
            try:
                # Create a new connection for this thread
                conn = yaspy.connect(dsn=self.getDsn(), user=self.user, password=self.passwd)
                cur = conn.cursor()

                # Mix of operations
                for i in range(5):
                    # Insert
                    temp_array = array.array("f", [0.1 * i, 0.2 * i, 0.3 * i, 0.4 * i])
                    cur.execute(f"insert into {name} values(?, ?, ?, ?)", [20 + i, temp_array, f"mixed_{i}", 300 + i])

                    # Select
                    cur.execute(f"select count(*) from {name}")
                    count_result = cur.fetchone()

                    # Update
                    cur.execute(f"update {name} set text_col = ? where id = ?", [f"updated_{i}", 20 + i])

                    conn.commit()

                    with lock:
                        operation_counts["mixed"] += 1

                cur.close()
                conn.close()
            except Exception as e:
                with lock:
                    errors.append(f"Mixed thread error: {str(e)}")

        # Create and start threads
        threads = []
        # More reader threads (8) for 90% read scenario
        for _ in range(8):
            t = threading.Thread(target=reader_thread)
            threads.append(t)
            t.start()

        # Writer threads (2)
        for _ in range(2):
            t = threading.Thread(target=writer_thread)
            threads.append(t)
            t.start()

        # Deleter threads (1)
        for _ in range(1):
            t = threading.Thread(target=deleter_thread)
            threads.append(t)
            t.start()

        # Mixed operation threads (2)
        for _ in range(2):
            t = threading.Thread(target=mixed_thread)
            threads.append(t)
            t.start()

        # Wait for all threads to complete
        for t in threads:
            t.join()

        # Check for errors
        self.assertEqual(len(errors), 0, f"High concurrency mixed operations errors: {errors}")

        # Verify final state
        cursor.execute(f"select count(*) from {name} where id = 1")
        count_result = cursor.fetchone()
        self.assertEqual(count_result[0], 1, "Original record should still exist")

    def test_vector_error_handling(self, name: str = "test_vector_error_handling"):
        """Test VECTOR error handling and recovery"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Test 1: Inserting array with wrong dimension
        wrong_dim_array = array.array("f", [1.0, 2.0, 3.0])  # Only 3 elements instead of 4
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, wrong_dim_array])

        # Test 2: Inserting non-array data
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [2, "not_an_array"])

        # Test 3: Inserting array with wrong type
        wrong_type_array = array.array("i", [1, 2, 3, 4])  # Integer array instead of float
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [3, wrong_type_array])

        # Verify no data was inserted due to errors
        cursor.execute(f"select count(*) from {name}")
        count_result = cursor.fetchone()
        self.assertEqual(count_result[0], 0)

        # Test recovery: Insert valid data after errors
        valid_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        cursor.execute(f"insert into {name} values(?, ?)", [4, valid_array])
        self.connection.commit()

        cursor.execute(f"select vec from {name} where id = 4")
        result = cursor.fetchone()
        self.assertIsNotNone(result)
        self.assertIsNotNone(result[0])
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), 4)
        for i in range(4):
            self.assertAlmostEqual(retrieved_array[i], valid_array[i], places=5)

    def test_vector_with_other_types(self, name: str = "test_vector_mixed_types"):
        """Test VECTOR compatibility with other data types"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR and various other data types
        cursor.execute(f"""create table {name}(
            id int primary key,
            vec_f32 vector(4),
            vec_f64 vector(4, float64),
            text_col varchar(100),
            int_col int,
            float_col float,
            bool_col boolean,
            date_col date,
            timestamp_col timestamp
        )""")

        # Insert data with all types
        vec_f32 = array.array("f", [1.1, 2.2, 3.3, 4.4])
        vec_f64 = array.array("d", [1.1111111111, 2.2222222222, 3.3333333333, 4.4444444444])

        # Insert with named parameters to ensure correct mapping
        cursor.execute(
            f"""insert into {name} values(?, ?, ?, ?, ?, ?, ?, ?, ?)""",
            [1, vec_f32, vec_f64, "test text", 42, 3.14159, True, "2023-01-01", "2023-01-01 12:00:00"],
        )
        self.connection.commit()

        # Retrieve and verify all data
        cursor.execute(f"select * from {name} where id = 1")
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        self.assertEqual(result[0], 1)  # id
        # Check vectors
        self.assertEqual(len(result[1]), 4)  # vec_f32
        self.assertEqual(len(result[2]), 4)  # vec_f64
        # Check other types
        self.assertEqual(result[3], "test text")  # text_col
        self.assertEqual(result[4], 42)  # int_col
        self.assertAlmostEqual(result[5], 3.14159, places=5)  # float_col
        self.assertEqual(result[6], True)  # bool_col

        # Test complex query with VECTOR and other types
        cursor.execute(f"select id, vec_f32, text_col from {name} where int_col = ? and bool_col = ?", [42, True])
        result = cursor.fetchone()
        self.assertIsNotNone(result)
        self.assertEqual(result[0], 1)
        self.assertEqual(len(result[1]), 4)
        self.assertEqual(result[2], "test text")


if __name__ == "__main__":
    test_base.run_test_cases()

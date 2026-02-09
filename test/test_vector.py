import array
import os
import re
import sys

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
    def test_vector_create_by_array(self, name: str = "test_vector_array"):
        """Test creating VECTOR variable from Python array"""
        cursor = self.connection.cursor()

        # Create a float32 array
        float_array = array.array("f", [1.0, 2.0, 3.0, 4.0])

        # Create VECTOR variable using array
        vector_var = cursor.var(yaspy.VECTOR)
        vector_var.setvalue(float_array)

        # Get value back
        retrieved_array = vector_var.getvalue()

        # Check if the retrieved array matches the original
        self.assertEqual(len(retrieved_array), len(float_array))
        for i in range(len(float_array)):
            self.assertAlmostEqual(retrieved_array[i], float_array[i], places=5)

    def test_vector_create_by_array_double(self, name: str = "test_vector_array_double"):
        """Test creating VECTOR variable from Python double array"""
        cursor = self.connection.cursor()

        # Create a float64 array
        double_array = array.array("d", [1.1, 2.2, 3.3, 4.4])

        # Create VECTOR variable using array
        vector_var = cursor.var(yaspy.VECTOR)
        vector_var.setvalue(double_array)

        # Get value back
        retrieved_array = vector_var.getvalue()

        # Check if the retrieved array matches the original
        self.assertEqual(len(retrieved_array), len(double_array))
        for i in range(len(double_array)):
            self.assertAlmostEqual(retrieved_array[i], double_array[i], places=5)

    def test_vector_insert_select(self, name: str = "test_vector_insert_select"):
        """Test inserting and selecting VECTOR data from database"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 4, default float32)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Create array and insert
        float_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, float_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(float_array))
        for i in range(len(float_array)):
            self.assertAlmostEqual(retrieved_array[i], float_array[i], places=5)

    def test_vector_insert_select_float64(self, name: str = "test_vector_insert_select_f64"):
        """Test inserting and selecting VECTOR data from database with float64 format"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 4, float64 format)
        cursor.execute(f"create table {name}(id int, vec vector(4, float64))")

        # Create array and insert
        double_array = array.array("d", [1.1, 2.2, 3.3, 4.4])
        cursor.execute(f"insert into {name} values(?, ?)", [1, double_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(double_array))
        for i in range(len(double_array)):
            self.assertAlmostEqual(retrieved_array[i], double_array[i], places=5)

    def test_vector_null_value(self, name: str = "test_vector_null"):
        """Test handling of NULL VECTOR values"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 3)
        cursor.execute(f"create table {name}(id int, vec vector(3))")

        # Insert NULL value
        cursor.execute(f"insert into {name} values(?, ?)", [1, None])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        self.assertIsNone(result[0])

    def test_vector_large_dimension(self, name: str = "test_vector_large_dim"):
        """Test VECTOR with large dimension array"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 1000)
        cursor.execute(f"create table {name}(id int, vec vector(1000))")

        # Create large array
        large_array = array.array("f", [float(i) for i in range(1000)])

        # Insert large array
        cursor.execute(f"insert into {name} values(?, ?)", [1, large_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(large_array))
        for i in range(len(large_array)):
            self.assertAlmostEqual(retrieved_array[i], large_array[i], places=5)

    def test_vector_multiple_rows(self, name: str = "test_vector_multi_rows"):
        """Test inserting and selecting multiple VECTOR rows"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 3)
        cursor.execute(f"create table {name}(id int, vec vector(3))")

        # Create arrays
        array1 = array.array("f", [1.0, 2.0, 3.0])
        array2 = array.array("f", [4.0, 5.0, 6.0])
        array3 = array.array("f", [7.0, 8.0, 9.0])

        # Insert multiple rows
        cursor.execute(f"insert into {name} values(?, ?)", [1, array1])
        cursor.execute(f"insert into {name} values(?, ?)", [2, array2])
        cursor.execute(f"insert into {name} values(?, ?)", [3, array3])
        self.connection.commit()

        # Select all and verify
        cursor.execute(f"select id, vec from {name} order by id")
        results = cursor.fetchall()

        self.assertEqual(len(results), 3)

        # Verify first row
        self.assertEqual(results[0][0], 1)
        retrieved_array1 = results[0][1]
        self.assertEqual(len(retrieved_array1), len(array1))
        for i in range(len(array1)):
            self.assertAlmostEqual(retrieved_array1[i], array1[i], places=5)

        # Verify second row
        self.assertEqual(results[1][0], 2)
        retrieved_array2 = results[1][1]
        self.assertEqual(len(retrieved_array2), len(array2))
        for i in range(len(array2)):
            self.assertAlmostEqual(retrieved_array2[i], array2[i], places=5)

        # Verify third row
        self.assertEqual(results[2][0], 3)
        retrieved_array3 = results[2][1]
        self.assertEqual(len(retrieved_array3), len(array3))
        for i in range(len(array3)):
            self.assertAlmostEqual(retrieved_array3[i], array3[i], places=5)

    def test_vector_min_dimension(self, name: str = "test_vector_min_dim"):
        """Test VECTOR with minimum dimension (1)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (minimum dimension 1)
        cursor.execute(f"create table {name}(id int, vec vector(1))")

        # Create array with minimum dimension
        min_array = array.array("f", [3.14159])

        # Insert minimum dimension array
        cursor.execute(f"insert into {name} values(?, ?)", [1, min_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(min_array))
        self.assertAlmostEqual(retrieved_array[0], min_array[0], places=5)

    def test_vector_max_dimension(self, name: str = "test_vector_max_dim"):
        """Test VECTOR with maximum dimension (65535)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (maximum dimension)
        cursor.execute(f"create table {name}(id int, vec vector(65535))")

        # Create array with maximum dimension
        max_array = array.array("f", [1.0] * 65535)

        # Insert max dimension array
        cursor.execute(f"insert into {name} values(?, ?)", [1, max_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(max_array))
        # Check a few sample values to avoid excessive comparisons
        for i in [0, 100, 1000, 10000, 65534]:
            self.assertAlmostEqual(retrieved_array[i], max_array[i], places=5)

    def test_vector_different_formats(self, name: str = "test_vector_formats"):
        """Test VECTOR with different data formats"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR columns with different formats
        cursor.execute(f"create table {name}(id int, vec_f32 vector(4), vec_f64 vector(4, float64))")

        # Create float32 and float64 arrays
        float_array = array.array("f", [1.1, 2.2, 3.3, 4.4])  # float32
        double_array = array.array("d", [1.1, 2.2, 3.3, 4.4])  # float64

        # Insert both arrays
        cursor.execute(f"insert into {name} values(?, ?, ?)", [1, float_array, double_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec_f32, vec_f64 from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)

        # Verify float32 array
        retrieved_float = result[0]
        self.assertEqual(len(retrieved_float), len(float_array))
        for i in range(len(float_array)):
            self.assertAlmostEqual(retrieved_float[i], float_array[i], places=5)

        # Verify float64 array
        retrieved_double = result[1]
        self.assertEqual(len(retrieved_double), len(double_array))
        for i in range(len(double_array)):
            self.assertAlmostEqual(retrieved_double[i], double_array[i], places=5)

    def test_vector_max_dimension_and_format(self, name: str = "test_vector_max_dim_format"):
        """Test VECTOR with maximum dimension (65535) and maximum format (float64)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (maximum dimension, float64 format)
        cursor.execute(f"create table {name}(id int, vec vector(65535, float64))")

        # Create array with maximum dimension using float64 format
        max_array = array.array("d", [1.5] * 65535)

        # Insert max dimension array
        cursor.execute(f"insert into {name} values(?, ?)", [1, max_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(max_array))
        # Check a few sample values to avoid excessive comparisons
        for i in [0, 100, 1000, 10000, 65534]:
            self.assertAlmostEqual(retrieved_array[i], max_array[i], places=5)

    def test_vector_case_insensitive_format(self, name: str = "test_vector_case_format"):
        """Test VECTOR with case insensitive format specification"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 4, uppercase FLOAT64 format)
        cursor.execute(f"create table {name}(id int, vec vector(4, FLOAT64))")

        # Create array and insert
        double_array = array.array("d", [1.1, 2.2, 3.3, 4.4])
        cursor.execute(f"insert into {name} values(?, ?)", [1, double_array])
        self.connection.commit()

        # Select and verify
        cursor.execute(f"select vec from {name} where id = ?", [1])
        result = cursor.fetchone()

        self.assertIsNotNone(result)
        retrieved_array = result[0]
        self.assertEqual(len(retrieved_array), len(double_array))
        for i in range(len(double_array)):
            self.assertAlmostEqual(retrieved_array[i], double_array[i], places=5)

    def test_vector_char_conversion(self, name: str = "test_vector_char_conv"):
        """Test implicit conversion from CHAR to VECTOR type"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create CHAR variable with VECTOR string format
        char_var = cursor.var(yaspy.CHAR, size=100)
        char_value = "[1.0, 2.0, 3.0, 4.0]"
        char_var.setvalue(char_value)

        # Insert CHAR variable into VECTOR column (should trigger implicit conversion)
        cursor.execute(f"insert into {name} values(?, ?)", [1, char_var])
        self.connection.commit()

        # Select and verify VECTOR value
        cursor.execute(f"select vec_col from {name} where id = ?", [1])
        result = cursor.fetchone()
        self.assertIsNotNone(result)

        retrieved_array = result[0]
        expected_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        self.assertEqual(len(retrieved_array), len(expected_array))
        for i in range(len(expected_array)):
            self.assertAlmostEqual(retrieved_array[i], expected_array[i], places=5)

    def test_vector_varchar_conversion(self, name: str = "test_vector_varchar_conv"):
        """Test implicit conversion from VARCHAR to VECTOR type"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create VARCHAR variable with VECTOR string format
        varchar_var = cursor.var(yaspy.VARCHAR, size=100)
        varchar_value = "[1.1, 2.2, 3.3, 4.4]"
        varchar_var.setvalue(varchar_value)

        # Insert VARCHAR variable into VECTOR column (should trigger implicit conversion)
        cursor.execute(f"insert into {name} values(?, ?)", [1, varchar_var])
        self.connection.commit()

        # Select and verify VECTOR value
        cursor.execute(f"select vec_col from {name} where id = ?", [1])
        result = cursor.fetchone()
        self.assertIsNotNone(result)

        retrieved_array = result[0]
        expected_array = array.array("f", [1.1, 2.2, 3.3, 4.4])
        self.assertEqual(len(retrieved_array), len(expected_array))
        for i in range(len(expected_array)):
            self.assertAlmostEqual(retrieved_array[i], expected_array[i], places=5)

    def test_vector_clob_conversion(self, name: str = "test_vector_clob_conv"):
        """Test implicit conversion from CLOB to VECTOR type"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create CLOB variable with VECTOR string format
        clob_var = cursor.var(yaspy.CLOB)
        clob_value = "[1.5, 2.5, 3.5, 4.5]"
        clob_var.setvalue(clob_value)

        # Insert CLOB variable into VECTOR column (should trigger implicit conversion)
        cursor.execute(f"insert into {name} values(?, ?)", [1, clob_var])
        self.connection.commit()

        # Select and verify VECTOR value
        cursor.execute(f"select vec_col from {name} where id = ?", [1])
        result = cursor.fetchone()
        self.assertIsNotNone(result)

        retrieved_array = result[0]
        expected_array = array.array("f", [1.5, 2.5, 3.5, 4.5])
        self.assertEqual(len(retrieved_array), len(expected_array))
        for i in range(len(expected_array)):
            self.assertAlmostEqual(retrieved_array[i], expected_array[i], places=5)

    def test_vector_invalid_char_conversion(self, name: str = "test_vector_invalid_char_conv"):
        """Test invalid implicit conversion from CHAR to VECTOR type (invalid format)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create CHAR variable with invalid VECTOR string format
        char_var = cursor.var(yaspy.CHAR, size=100)
        char_value = "invalid_vector_format"  # Invalid format
        char_var.setvalue(char_value)

        # Try to insert CHAR variable into VECTOR column - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, char_var])
            self.connection.commit()

    def test_vector_invalid_varchar_conversion(self, name: str = "test_vector_invalid_varchar_conv"):
        """Test invalid implicit conversion from VARCHAR to VECTOR type (invalid format)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create VARCHAR variable with invalid VECTOR string format
        varchar_var = cursor.var(yaspy.VARCHAR, size=100)
        varchar_value = "[1.0, 2.0, 3.0]"  # Wrong dimension
        varchar_var.setvalue(varchar_value)

        # Try to insert VARCHAR variable into VECTOR column - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, varchar_var])
            self.connection.commit()

    def test_vector_invalid_clob_conversion(self, name: str = "test_vector_invalid_clob_conv"):
        """Test invalid implicit conversion from CLOB to VECTOR type (invalid format)"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create CLOB variable with invalid VECTOR string format
        clob_var = cursor.var(yaspy.CLOB)
        clob_value = "[1.0, 2.0, 3.0, 4.0, 5.0]"  # Too many elements
        clob_var.setvalue(clob_value)

        # Try to insert CLOB variable into VECTOR column - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, clob_var])
            self.connection.commit()

    def test_vector_int_conversion_not_supported(self, name: str = "test_vector_int_conv"):
        """Test that implicit conversion from INT to VECTOR type is not supported"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create INT variable
        int_var = cursor.var(yaspy.INTEGER)
        int_value = 12345
        int_var.setvalue(int_value)

        # Try to insert INT variable into VECTOR column - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, int_var])
            self.connection.commit()

    def test_vector_float_conversion_not_supported(self, name: str = "test_vector_float_conv"):
        """Test that implicit conversion from FLOAT to VECTOR type is not supported"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create FLOAT variable
        float_var = cursor.var(yaspy.FLOAT)
        float_value = 3.14159
        float_var.setvalue(float_value)

        # Try to insert FLOAT variable into VECTOR column - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, float_var])
            self.connection.commit()

    def test_vector_bigint_conversion_not_supported(self, name: str = "test_vector_bigint_conv"):
        """Test that implicit conversion from BIGINT to VECTOR type is not supported"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Create BIGINT variable
        bigint_var = cursor.var(yaspy.BIGINT)
        bigint_value = 123456789012345
        bigint_var.setvalue(bigint_value)

        # Try to insert BIGINT variable into VECTOR column - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"insert into {name} values(?, ?)", [1, bigint_var])
            self.connection.commit()

    def test_vector_function_return_char(self, name: str = "test_vector_func_char"):
        """Test VECTOR function returning CHAR type by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that directly assigns vector data to CHAR
        cursor.execute(f"""
            CREATE OR REPLACE FUNCTION get_vector_as_char(p_id int) RETURN CHAR IS
                v_char char(100);
            BEGIN
                SELECT vec_col INTO v_char FROM {name} WHERE id = p_id;
                RETURN v_char;
            END;
        """)

        # Create CHAR variable to receive returned value
        char_var = cursor.var(yaspy.CHAR, size=101)

        # Execute function and get result into CHAR variable
        cursor.execute("BEGIN ? := get_vector_as_char(1); END;", [char_var])

        # Check the returned CHAR value - it should contain the vector representation
        returned_value = char_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [1.0, 2.0, 3.0, 4.0]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

        # Clean up function
        cursor.execute("DROP FUNCTION get_vector_as_char")

    def test_vector_function_return_varchar(self, name: str = "test_vector_func_varchar"):
        """Test VECTOR function returning VARCHAR type by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [1.1, 2.2, 3.3, 4.4])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that directly assigns vector data to VARCHAR
        cursor.execute(f"""
            CREATE OR REPLACE FUNCTION get_vector_as_varchar(p_id int) RETURN VARCHAR IS
                v_varchar varchar(100);
            BEGIN
                SELECT vec_col INTO v_varchar FROM {name} WHERE id = p_id;
                RETURN v_varchar;
            END;
        """)

        # Create VARCHAR variable to receive returned value
        varchar_var = cursor.var(yaspy.VARCHAR, size=100)

        # Execute function and get result into VARCHAR variable
        cursor.execute("BEGIN ? := get_vector_as_varchar(1); END;", [varchar_var])

        # Check the returned VARCHAR value - it should contain the vector representation
        returned_value = varchar_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [1.1, 2.2, 3.3, 4.4]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

        # Clean up function
        cursor.execute("DROP FUNCTION get_vector_as_varchar")

    def test_vector_function_return_clob(self, name: str = "test_vector_func_clob"):
        """Test VECTOR function returning CLOB type by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [1.5, 2.5, 3.5, 4.5])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that directly assigns vector data to CLOB
        cursor.execute(f"""
            CREATE OR REPLACE FUNCTION get_vector_as_clob(p_id int) RETURN CLOB IS
                v_clob clob;
            BEGIN
                SELECT vec_col INTO v_clob FROM {name} WHERE id = p_id;
                RETURN v_clob;
            END;
        """)

        # Create CLOB variable to receive returned value
        clob_var = cursor.var(yaspy.CLOB)

        # Execute function and get result into CLOB variable
        cursor.execute("BEGIN ? := get_vector_as_clob(1); END;", [clob_var])

        # Check the returned CLOB value - it should contain the vector representation
        returned_value = clob_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [1.5, 2.5, 3.5, 4.5]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

        # Clean up function
        cursor.execute("DROP FUNCTION get_vector_as_clob")

    def test_vector_function_return_vector(self, name: str = "test_vector_func_vector"):
        """Test VECTOR function returning VECTOR type by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [2.0, 3.0, 4.0, 5.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that directly assigns vector data to VECTOR
        cursor.execute(f"""
            CREATE OR REPLACE FUNCTION get_vector_as_vector(p_id int) RETURN vector IS
                v_vec vector(4);
            BEGIN
                SELECT vec_col INTO v_vec FROM {name} WHERE id = p_id;
                RETURN v_vec;
            END;
        """)

        # Create VECTOR variable to receive returned value
        vector_var = cursor.var(yaspy.VECTOR)

        # Execute function and get result into VECTOR variable
        cursor.execute("BEGIN ? := get_vector_as_vector(1); END;", [vector_var])

        # Check the returned VECTOR value
        returned_array = vector_var.getvalue()
        expected_array = array.array("f", [2.0, 3.0, 4.0, 5.0])
        self.assertEqual(len(returned_array), len(expected_array))
        for i in range(len(expected_array)):
            self.assertAlmostEqual(returned_array[i], expected_array[i], places=5)

        # Clean up function
        cursor.execute("DROP FUNCTION get_vector_as_vector")

    def test_vector_procedure_out_char(self, name: str = "test_vector_proc_out_char"):
        """Test VECTOR procedure with CHAR OUT parameter by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [5.0, 6.0, 7.0, 8.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that directly assigns vector data to CHAR OUT parameter
        cursor.execute(f"""
            CREATE OR REPLACE PROCEDURE get_vector_out_char(p_id int, vec_out OUT CHAR) IS
            BEGIN
                SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
            END;
        """)

        # Create CHAR variable to receive OUT parameter
        char_var = cursor.var(yaspy.CHAR, size=100)

        # Call procedure with CHAR OUT parameter
        cursor.callproc("get_vector_out_char", [1, char_var])

        # Check the returned CHAR value - it should contain the vector representation
        returned_value = char_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [5.0, 6.0, 7.0, 8.0]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

        # Clean up procedure
        cursor.execute("DROP PROCEDURE get_vector_out_char")

    def test_vector_procedure_out_varchar(self, name: str = "test_vector_proc_out_varchar"):
        """Test VECTOR procedure with VARCHAR OUT parameter by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [5.1, 6.1, 7.1, 8.1])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that directly assigns vector data to VARCHAR OUT parameter
        cursor.execute(f"""
            CREATE OR REPLACE PROCEDURE get_vector_out_varchar(p_id int, vec_out OUT VARCHAR) IS
            BEGIN
                SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
            END;
        """)

        # Create VARCHAR variable to receive OUT parameter
        varchar_var = cursor.var(yaspy.VARCHAR, size=100)

        # Call procedure with VARCHAR OUT parameter
        cursor.callproc("get_vector_out_varchar", [1, varchar_var])

        # Check the returned VARCHAR value - it should contain the vector representation
        returned_value = varchar_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [5.1, 6.1, 7.1, 8.1]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

        # Clean up procedure
        cursor.execute("DROP PROCEDURE get_vector_out_varchar")

    def test_vector_procedure_out_clob(self, name: str = "test_vector_proc_out_clob"):
        """Test VECTOR procedure with CLOB OUT parameter by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [5.5, 6.5, 7.5, 8.5])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that directly assigns vector data to CLOB OUT parameter
        cursor.execute(f"""
            CREATE OR REPLACE PROCEDURE get_vector_out_clob(p_id int, vec_out OUT CLOB) IS
            BEGIN
                SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
            END;
        """)

        # Create CLOB variable to receive OUT parameter
        clob_var = cursor.var(yaspy.CLOB)

        # Call procedure with CLOB OUT parameter
        cursor.callproc("get_vector_out_clob", [1, clob_var])

        # Check the returned CLOB value - it should contain the vector representation
        returned_value = clob_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [5.5, 6.5, 7.5, 8.5]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

        # Clean up procedure
        cursor.execute("DROP PROCEDURE get_vector_out_clob")

    def test_vector_procedure_out_vector(self, name: str = "test_vector_proc_out_vector"):
        """Test VECTOR procedure with VECTOR OUT parameter by directly assigning vector column data"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [6.0, 7.0, 8.0, 9.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that directly assigns vector data to VECTOR OUT parameter
        cursor.execute(f"""
            CREATE OR REPLACE PROCEDURE get_vector_out_vector(p_id int, vec_out OUT vector) IS
            BEGIN
                SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
            END;
        """)

        # Create VECTOR variable to receive OUT parameter
        vector_var = cursor.var(yaspy.VECTOR)

        # Call procedure with VECTOR OUT parameter
        cursor.callproc("get_vector_out_vector", [1, vector_var])

        # Check the returned VECTOR value
        returned_array = vector_var.getvalue()
        expected_array = array.array("f", [6.0, 7.0, 8.0, 9.0])
        self.assertEqual(len(returned_array), len(expected_array))
        for i in range(len(expected_array)):
            self.assertAlmostEqual(returned_array[i], expected_array[i], places=5)

        # Clean up procedure
        cursor.execute("DROP PROCEDURE get_vector_out_vector")

    def test_vector_function_return_invalid_int(self, name: str = "test_vector_func_invalid_int"):
        """Test VECTOR function returning INT type which should fail"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [7.0, 8.0, 9.0, 10.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that tries to assign vector data to INT - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"""
                CREATE OR REPLACE FUNCTION get_vector_as_int(p_id int) RETURN INT IS
                    v_int int;
                BEGIN
                    SELECT vec_col INTO v_int FROM {name} WHERE id = p_id;
                    RETURN v_int;
                END;
            """)

    def test_vector_function_return_invalid_float(self, name: str = "test_vector_func_invalid_float"):
        """Test VECTOR function returning FLOAT type which should fail"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [11.0, 12.0, 13.0, 14.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that tries to assign vector data to FLOAT - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"""
                CREATE OR REPLACE FUNCTION get_vector_as_float(p_id int) RETURN FLOAT IS
                    v_float float;
                BEGIN
                    SELECT vec_col INTO v_float FROM {name} WHERE id = p_id;
                    RETURN v_float;
                END;
            """)

    def test_vector_function_return_invalid_bigint(self, name: str = "test_vector_func_invalid_bigint"):
        """Test VECTOR function returning BIGINT type which should fail"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [15.0, 16.0, 17.0, 18.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a function that tries to assign vector data to BIGINT - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"""
                CREATE OR REPLACE FUNCTION get_vector_as_bigint(p_id int) RETURN BIGINT IS
                    v_bigint bigint;
                BEGIN
                    SELECT vec_col INTO v_bigint FROM {name} WHERE id = p_id;
                    RETURN v_bigint;
                END;
            """)

    def test_vector_procedure_out_invalid_int(self, name: str = "test_vector_proc_out_invalid_int"):
        """Test VECTOR procedure with INT OUT parameter which should fail"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [19.0, 20.0, 21.0, 22.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that tries to assign vector data to INT OUT parameter - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"""
                CREATE OR REPLACE PROCEDURE get_vector_out_int(p_id int, vec_out OUT INT) IS
                BEGIN
                    SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
                END;
            """)

    def test_vector_procedure_out_invalid_float(self, name: str = "test_vector_proc_out_invalid_float"):
        """Test VECTOR procedure with FLOAT OUT parameter which should fail"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [23.0, 24.0, 25.0, 26.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that tries to assign vector data to FLOAT OUT parameter - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"""
                CREATE OR REPLACE PROCEDURE get_vector_out_float(p_id int, vec_out OUT FLOAT) IS
                BEGIN
                    SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
                END;
            """)

    def test_vector_procedure_out_invalid_bigint(self, name: str = "test_vector_proc_out_invalid_bigint"):
        """Test VECTOR procedure with BIGINT OUT parameter which should fail"""
        cursor = self.connection.cursor()

        # Create table with VECTOR column
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [27.0, 28.0, 29.0, 30.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create a procedure that tries to assign vector data to BIGINT OUT parameter - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"""
                CREATE OR REPLACE PROCEDURE get_vector_out_bigint(p_id int, vec_out OUT BIGINT) IS
                BEGIN
                    SELECT vec_col INTO vec_out FROM {name} WHERE id = p_id;
                END;
            """)

    def test_vector_insert_returning_char(self, name: str = "test_vector_insert_returning_char"):
        """Test VECTOR insert returning CHAR type by directly assigning vector column data"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [9.0, 10.0, 11.0, 12.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create CHAR variable to receive returned value
        char_var = cursor.var(yaspy.CHAR, size=100)

        # Update a row and return the vector directly into CHAR variable
        cursor.execute(f"update {name} set id = 3 where id = 1 returning vec_col into ?", [char_var])

        # Check the returned CHAR value - it should contain the vector representation
        returned_value = char_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [9.0, 10.0, 11.0, 12.0]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

    def test_vector_insert_returning_varchar(self, name: str = "test_vector_insert_returning_varchar"):
        """Test VECTOR insert returning VARCHAR type by directly assigning vector column data"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [13.0, 14.0, 15.0, 16.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create VARCHAR variable to receive returned value
        varchar_var = cursor.var(yaspy.VARCHAR, size=100)

        # Update a row and return the vector directly into VARCHAR variable
        cursor.execute(f"update {name} set id = 4 where id = 1 returning vec_col into ?", [varchar_var])

        # Check the returned VARCHAR value - it should contain the vector representation
        returned_value = varchar_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [13.0, 14.0, 15.0, 16.0]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

    def test_vector_insert_returning_clob(self, name: str = "test_vector_insert_returning_clob"):
        """Test VECTOR insert returning CLOB type by directly assigning vector column data"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [17.0, 18.0, 19.0, 20.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create CLOB variable to receive returned value
        clob_var = cursor.var(yaspy.CLOB)

        # Update a row and return the vector directly into CLOB variable
        cursor.execute(f"update {name} set id = 5 where id = 1 returning vec_col into ?", [clob_var])

        # Check the returned CLOB value - it should contain the vector representation
        returned_value = clob_var.getvalue()
        # The exact format may vary, but it should contain the vector values
        # Extract floating point numbers from the string and compare with approximate equality
        numbers = re.findall(r"[+-]?\d+\.\d+[Ee][+-]?\d+|[+-]?\d*\.\d+", returned_value)
        expected_values = [17.0, 18.0, 19.0, 20.0]
        self.assertEqual(len(numbers), len(expected_values))
        for i, expected in enumerate(expected_values):
            actual = float(numbers[i])
            self.assertAlmostEqual(actual, expected, places=1)

    def test_vector_insert_returning_vector(self, name: str = "test_vector_insert_returning_vector"):
        """Test VECTOR insert returning VECTOR type by directly assigning vector column data"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [21.0, 22.0, 23.0, 24.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create VECTOR variable to receive returned value
        vector_var = cursor.var(yaspy.VECTOR)

        # Update a row and return the vector directly into VECTOR variable
        cursor.execute(f"update {name} set id = 6 where id = 1 returning vec_col into ?", [vector_var])

        # Check the returned VECTOR value
        returned_array = vector_var.getvalue()
        expected_array = array.array("f", [21.0, 22.0, 23.0, 24.0])
        self.assertEqual(len(returned_array), len(expected_array))
        for i in range(len(expected_array)):
            self.assertAlmostEqual(returned_array[i], expected_array[i], places=5)

    def test_vector_insert_returning_invalid_int(self, name: str = "test_vector_insert_returning_invalid_int"):
        """Test that VECTOR insert returning into INT variable fails"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(id int, vec_col vector(4))")

        # Insert a vector
        vector_array = array.array("f", [25.0, 26.0, 27.0, 28.0])
        cursor.execute(f"insert into {name} values(?, ?)", [1, vector_array])
        self.connection.commit()

        # Create INT variable to receive returned value
        int_var = cursor.var(yaspy.INTEGER)

        # Try to update a row and return the vector directly into INT variable - should raise DatabaseError
        with self.assertRaises(yaspy.DatabaseError):
            cursor.execute(f"update {name} set id = 7 where id = 1 returning vec_col into ?", [int_var])

    def test_vector_executemany_positional_binding(self, name: str = "test_vector_executemany_pos"):
        """Test bulk insert of VECTOR data using executemany with positional binding"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 4)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Create arrays for bulk insert
        array1 = array.array("f", [1.0, 2.0, 3.0, 4.0])
        array2 = array.array("f", [5.0, 6.0, 7.0, 8.0])
        array3 = array.array("f", [9.0, 10.0, 11.0, 12.0])

        # Bulk insert using executemany with positional binding (sequences)
        data = [(1, array1), (2, array2), (3, array3)]
        cursor.executemany(f"insert into {name} values(?, ?)", data)
        self.connection.commit()

        # Select all and verify
        cursor.execute(f"select id, vec from {name} order by id")
        results = cursor.fetchall()

        self.assertEqual(len(results), 3)

        # Verify first row
        self.assertEqual(results[0][0], 1)
        retrieved_array1 = results[0][1]
        self.assertEqual(len(retrieved_array1), len(array1))
        for i in range(len(array1)):
            self.assertAlmostEqual(retrieved_array1[i], array1[i], places=5)

        # Verify second row
        self.assertEqual(results[1][0], 2)
        retrieved_array2 = results[1][1]
        self.assertEqual(len(retrieved_array2), len(array2))
        for i in range(len(array2)):
            self.assertAlmostEqual(retrieved_array2[i], array2[i], places=5)

        # Verify third row
        self.assertEqual(results[2][0], 3)
        retrieved_array3 = results[2][1]
        self.assertEqual(len(retrieved_array3), len(array3))
        for i in range(len(array3)):
            self.assertAlmostEqual(retrieved_array3[i], array3[i], places=5)

    def test_vector_executemany_named_binding(self, name: str = "test_vector_executemany_named"):
        """Test bulk insert of VECTOR data using executemany with named binding"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 4)
        cursor.execute(f"create table {name}(id int, vec vector(4))")

        # Create arrays for bulk insert
        array1 = array.array("f", [1.1, 2.1, 3.1, 4.1])
        array2 = array.array("f", [5.1, 6.1, 7.1, 8.1])
        array3 = array.array("f", [9.1, 10.1, 11.1, 12.1])

        # Bulk insert using executemany with named binding (dictionaries)
        data = [{"id": 1, "vec": array1}, {"id": 2, "vec": array2}, {"id": 3, "vec": array3}]
        cursor.executemany(f"insert into {name} values(:id, :vec)", data)
        self.connection.commit()

        # Select all and verify
        cursor.execute(f"select id, vec from {name} order by id")
        results = cursor.fetchall()

        self.assertEqual(len(results), 3)

        # Verify first row
        self.assertEqual(results[0][0], 1)
        retrieved_array1 = results[0][1]
        self.assertEqual(len(retrieved_array1), len(array1))
        for i in range(len(array1)):
            self.assertAlmostEqual(retrieved_array1[i], array1[i], places=5)

        # Verify second row
        self.assertEqual(results[1][0], 2)
        retrieved_array2 = results[1][1]
        self.assertEqual(len(retrieved_array2), len(array2))
        for i in range(len(array2)):
            self.assertAlmostEqual(retrieved_array2[i], array2[i], places=5)

        # Verify third row
        self.assertEqual(results[2][0], 3)
        retrieved_array3 = results[2][1]
        self.assertEqual(len(retrieved_array3), len(array3))
        for i in range(len(array3)):
            self.assertAlmostEqual(retrieved_array3[i], array3[i], places=5)

    def test_vector_executemany_mixed_types(self, name: str = "test_vector_executemany_mixed"):
        """Test bulk insert of VECTOR data with mixed data types using executemany"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column and other data types
        cursor.execute(f"create table {name}(id int, name varchar(20), vec vector(3), score float)")

        # Create arrays for bulk insert
        array1 = array.array("f", [1.0, 2.0, 3.0])
        array2 = array.array("f", [4.0, 5.0, 6.0])
        array3 = array.array("f", [7.0, 8.0, 9.0])

        # Bulk insert using executemany with positional binding
        data = [(1, "first", array1, 95.5), (2, "second", array2, 87.2), (3, "third", array3, 92.8)]
        cursor.executemany(f"insert into {name} values(?, ?, ?, ?)", data)
        self.connection.commit()

        # Select all and verify
        cursor.execute(f"select id, name, vec, score from {name} order by id")
        results = cursor.fetchall()

        self.assertEqual(len(results), 3)

        # Verify first row
        self.assertEqual(results[0][0], 1)
        self.assertEqual(results[0][1], "first")
        self.assertAlmostEqual(results[0][3], 95.5, places=2)
        retrieved_array1 = results[0][2]
        self.assertEqual(len(retrieved_array1), len(array1))
        for i in range(len(array1)):
            self.assertAlmostEqual(retrieved_array1[i], array1[i], places=5)

        # Verify second row
        self.assertEqual(results[1][0], 2)
        self.assertEqual(results[1][1], "second")
        self.assertAlmostEqual(results[1][3], 87.2, places=2)
        retrieved_array2 = results[1][2]
        self.assertEqual(len(retrieved_array2), len(array2))
        for i in range(len(array2)):
            self.assertAlmostEqual(retrieved_array2[i], array2[i], places=5)

        # Verify third row
        self.assertEqual(results[2][0], 3)
        self.assertEqual(results[2][1], "third")
        self.assertAlmostEqual(results[2][3], 92.8, places=2)
        retrieved_array3 = results[2][2]
        self.assertEqual(len(retrieved_array3), len(array3))
        for i in range(len(array3)):
            self.assertAlmostEqual(retrieved_array3[i], array3[i], places=5)

    def test_vector_executemany_1000_rows(self, name: str = "test_vector_executemany_1000"):
        """Test bulk insert of VECTOR data with 1000 rows using executemany"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR column (dimension 4)
        cursor.execute(f"create table {name}(id int, category varchar(10), vec vector(4), score float)")

        # Create a dataset with 1000 rows
        data = []
        categories = ["CAT_A", "CAT_B", "CAT_C", "CAT_D", "CAT_E"]
        expected_arrays = []  # Store expected arrays for verification
        expected_scores = []  # Store expected scores for verification

        for i in range(1000):
            category = categories[i % 5]
            # Create array with 4 elements for each row
            array_data = array.array("f", [i * 0.1, i * 0.2, i * 0.3, i * 0.4])
            score = i * 0.25
            data.append((i + 1, category, array_data, score))
            expected_arrays.append(array_data)
            expected_scores.append(score)

        # Bulk insert using executemany with positional binding
        cursor.executemany(f"insert into {name} values(?, ?, ?, ?)", data)
        self.connection.commit()

        # Select all and verify count
        cursor.execute(f"select count(*) from {name}")
        count_result = cursor.fetchone()
        self.assertEqual(count_result[0], 1000)

        # Verify distribution of categories
        cursor.execute(f"select category, count(*) from {name} group by category order by category")
        category_results = cursor.fetchall()
        self.assertEqual(len(category_results), 5)
        for category_result in category_results:
            self.assertEqual(category_result[1], 200)  # Each category should have 200 rows

        # Verify all rows by fetching them in order
        cursor.execute(f"select id, category, vec, score from {name} order by id")
        results = cursor.fetchall()

        # Should have 1000 rows
        self.assertEqual(len(results), 1000)

        # Verify each row
        categories = ["CAT_A", "CAT_B", "CAT_C", "CAT_D", "CAT_E"]
        for i in range(1000):
            row = results[i]
            expected_id = i + 1
            expected_category = categories[i % 5]
            expected_array = expected_arrays[i]
            expected_score = expected_scores[i]

            # Verify id
            self.assertEqual(row[0], expected_id)

            # Verify category
            self.assertEqual(row[1], expected_category)

            # Verify score
            self.assertAlmostEqual(row[3], expected_score, places=2)

            # Verify vector array
            retrieved_array = row[2]
            self.assertEqual(len(retrieved_array), len(expected_array))
            for j in range(len(expected_array)):
                self.assertAlmostEqual(retrieved_array[j], expected_array[j], places=5)

    def test_vector_description_metadata(self, name: str = "test_vector_description"):
        """Test that VECTOR type description metadata is correctly returned"""
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # Create table with VECTOR columns of different formats
        cursor.execute(f"create table {name}(id int, vec_f32 vector(4), vec_f64 vector(4, float64))")

        # Insert test data
        float_array = array.array("f", [1.0, 2.0, 3.0, 4.0])
        double_array = array.array("d", [1.1, 2.2, 3.3, 4.4])
        cursor.execute(f"insert into {name} values(?, ?, ?)", [1, float_array, double_array])
        self.connection.commit()

        # Query and check description
        cursor.execute(f"select id, vec_f32, vec_f64 from {name}")
        description = cursor.description

        # Verify column count
        self.assertEqual(len(description), 3)

        # Verify first column (id - INT)
        self.assertEqual(description[0][0], 'ID')  # name
        # Note: description[0][1] returns the underlying database type code (4 for INTEGER)
        # while yaspy.INTEGER is an ApiType object, so we compare the type code directly
        self.assertEqual(description[0][1], 4)  # type code for INTEGER
        self.assertIsNotNone(description[0][2])  # display_size should not be None for INT
        self.assertIsNotNone(description[0][3])  # size should not be None for INT
        # For INTEGER type, precision may be None (when desc.precision == 255)
        # We won't assert anything specific about precision and scale for INTEGER type
        self.assertEqual(description[0][6], 1)  # nullable

        # Verify second column (vec_f32 - VECTOR)
        self.assertEqual(description[1][0], 'VEC_F32')  # name
        # Note: description[1][1] returns the underlying database type code (42 for VECTOR)
        # while yaspy.VECTOR is an ApiType object, so we compare the type code directly
        self.assertEqual(description[1][1], 42)  # type code for VECTOR
        self.assertIsNone(description[1][2])  # display_size should be None for VECTOR (aligned with Oracle)
        self.assertIsNone(description[1][3])  # size should be None for VECTOR (aligned with Oracle)
        self.assertIsNone(description[1][4])  # precision should be None for VECTOR (aligned with Oracle)
        self.assertIsNone(description[1][5])  # scale should be None for VECTOR (aligned with Oracle)
        self.assertEqual(description[1][6], 1)  # nullable

        # Verify third column (vec_f64 - VECTOR)
        self.assertEqual(description[2][0], 'VEC_F64')  # name
        # Note: description[2][1] returns the underlying database type code (42 for VECTOR)
        # while yaspy.VECTOR is an ApiType object, so we compare the type code directly
        self.assertEqual(description[2][1], 42)  # type code for VECTOR
        self.assertIsNone(description[2][2])  # display_size should be None for VECTOR (aligned with Oracle)
        self.assertIsNone(description[2][3])  # size should be None for VECTOR (aligned with Oracle)
        self.assertIsNone(description[2][4])  # precision should be None for VECTOR (aligned with Oracle)
        self.assertIsNone(description[2][5])  # scale should be None for VECTOR (aligned with Oracle)
        self.assertEqual(description[2][6], 1)  # nullable

        # Also verify that we can still fetch the data correctly
        cursor.execute(f"select id, vec_f32, vec_f64 from {name}")
        row = cursor.fetchone()
        
        # Verify data types
        self.assertIsInstance(row[0], int)  # id should be int
        self.assertIsInstance(row[1], array.array)  # vec_f32 should be array
        self.assertIsInstance(row[2], array.array)  # vec_f64 should be array
        
        # Verify array contents
        self.assertEqual(len(row[1]), 4)  # vec_f32 should have 4 elements
        self.assertEqual(len(row[2]), 4)  # vec_f64 should have 4 elements
        
        # Verify values
        for i in range(4):
            self.assertAlmostEqual(row[1][i], float_array[i], places=5)
            self.assertAlmostEqual(row[2][i], double_array[i], places=5)


if __name__ == "__main__":
    test_base.run_test_cases()

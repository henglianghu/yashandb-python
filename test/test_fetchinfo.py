import test_base
import yaspy


class TestCase(test_base.TestBaseCase):
    def test_fetchinfo_backward_compatibility_all_types(self):
        # Create a table with all supported data types
        self.cursor.execute("DROP TABLE IF EXISTS test_all_types")
        self.cursor.execute("""
            CREATE TABLE test_all_types (
                c_bool BOOLEAN,
                c_tinyint TINYINT,
                c_smallint SMALLINT,
                c_integer INTEGER,
                c_bigint BIGINT,
                c_float FLOAT,
                c_double DOUBLE,
                c_number NUMBER(10,2),
                c_char CHAR(10),
                c_varchar VARCHAR(50),
                c_nchar NCHAR(10),
                c_nvarchar NVARCHAR(50),
                c_binary BINARY(10),
                c_bit BIT(8),
                c_date DATE,
                c_time TIME,
                c_timestamp TIMESTAMP,
                c_timedelta INTERVAL DAY TO SECOND,
                c_yeardelta INTERVAL YEAR TO MONTH,
                c_rowid ROWID,
                c_json JSON,
                c_blob BLOB,
                c_clob CLOB,
                c_nclob NCLOB
            )
        """)

        # Insert a sample row
        self.cursor.execute("""
            INSERT INTO test_all_types VALUES (
                TRUE,
                1,
                123,
                12345,
                1234567890,
                123.45,
                123.456789,
                123456.78,
                'char_value',
                'varchar_value',
                'nchar_val',
                'nvarchar_val',
                '1010101010',
                127,
                TO_DATE('2023-01-01', 'YYYY-MM-DD'),
                '12:34:56',
                TO_TIMESTAMP('2023-01-01 12:34:56'),
                INTERVAL '1' DAY,
                INTERVAL '1' YEAR,
                NULL,  -- ROWID is typically auto-generated
                '{"key": "value"}',
                '101010100100101001010100101020312afed',
                'clob_data',
                'nclob_data'
            )
        """)
        self.connection.commit()

        # Execute a query with all column types
        self.cursor.execute("SELECT * FROM test_all_types")

        # Get description
        description = self.cursor.description

        # Check that we have the expected number of columns
        self.assertEqual(len(description), 24)

        # Define expected column information (using numeric type codes)
        # These values correspond to YapiType enum in yacapi.h
        expected_columns = [
            ("C_BOOL", 1),  # YAPI_TYPE_BOOL
            ("C_TINYINT", 2),  # YAPI_TYPE_TINYINT
            ("C_SMALLINT", 3),  # YAPI_TYPE_SMALLINT
            ("C_INTEGER", 4),  # YAPI_TYPE_INTEGER
            ("C_BIGINT", 5),  # YAPI_TYPE_BIGINT
            ("C_FLOAT", 10),  # YAPI_TYPE_FLOAT
            ("C_DOUBLE", 11),  # YAPI_TYPE_DOUBLE
            ("C_NUMBER", 12),  # YAPI_TYPE_NUMBER
            ("C_CHAR", 24),  # YAPI_TYPE_CHAR
            ("C_VARCHAR", 26),  # YAPI_TYPE_VARCHAR
            ("C_NCHAR", 25),  # YAPI_TYPE_NCHAR
            ("C_NVARCHAR", 27),  # YAPI_TYPE_NVARCHAR
            ("C_BINARY", 28),  # YAPI_TYPE_BINARY
            ("C_BIT", 31),  # YAPI_TYPE_BIT
            ("C_DATE", 13),  # YAPI_TYPE_DATE
            ("C_TIME", 15),  # YAPI_TYPE_SHORTTIME
            ("C_TIMESTAMP", 16),  # YAPI_TYPE_TIMESTAMP
            ("C_TIMEDELTA", 20),  # YAPI_TYPE_DS_INTERVAL
            ("C_YEARDELTA", 19),  # YAPI_TYPE_YM_INTERVAL
            ("C_ROWID", 32),  # YAPI_TYPE_ROWID
            ("C_JSON", 35),  # YAPI_TYPE_JSON
            ("C_BLOB", 30),  # YAPI_TYPE_BLOB
            ("C_CLOB", 29),  # YAPI_TYPE_CLOB
            ("C_NCLOB", 33),  # YAPI_TYPE_NCLOB
        ]

        # Check each column description can be accessed like a tuple (indexing)
        for i, (expected_name, expected_type) in enumerate(expected_columns):
            col_desc = description[i]

            # Test indexing access (forward)
            self.assertEqual(col_desc[0], expected_name)  # name
            self.assertEqual(col_desc[1], expected_type)  # type
            # display_size, internal_size, precision, scale, null_ok are checked below

            # Test indexing access (reverse)
            self.assertEqual(col_desc[-7], expected_name)  # name
            self.assertEqual(col_desc[-6], expected_type)  # type
            # display_size = col_desc[-5]
            # internal_size = col_desc[-4]
            # precision = col_desc[-3]
            # scale = col_desc[-2]
            # null_ok = col_desc[-1]

            # Test attribute access
            self.assertEqual(col_desc.name, expected_name)
            self.assertEqual(col_desc.type, expected_type)
            # display_size, internal_size, precision, scale, null_ok are checked below
            self.assertIsNone(col_desc.vector_dimension)
            self.assertIsNone(col_desc.vector_format)

            # Test length
            self.assertEqual(len(col_desc), 7)

            # For non-VECTOR types, vector_dimension and vector_format should be None
            self.assertIsNone(col_desc.vector_dimension)
            self.assertIsNone(col_desc.vector_format)

            # Test unpacking (a, b, c = fetchinfo)
            name, type_code, display_size, internal_size, precision, scale, null_ok = col_desc
            self.assertEqual(name, expected_name)
            self.assertEqual(type_code, expected_type)
            # display_size, internal_size, precision, scale, null_ok are checked below

        # Test iteration over description (variable binding resolution)
        for i, col_desc in enumerate(description):
            # Check that each column description behaves like a sequence
            self.assertEqual(len(col_desc), 7)

            # Check that we can access all elements by index (forward)
            name = col_desc[0]
            type_code = col_desc[1]
            display_size = col_desc[2]
            internal_size = col_desc[3]
            precision = col_desc[4]
            scale = col_desc[5]
            null_ok = col_desc[6]

            # Check that we can access all elements by index (reverse)
            reverse_name = col_desc[-7]
            reverse_type = col_desc[-6]
            reverse_display_size = col_desc[-5]
            reverse_internal_size = col_desc[-4]
            reverse_precision = col_desc[-3]
            reverse_scale = col_desc[-2]
            reverse_null_ok = col_desc[-1]

            # Verify forward and reverse indexing give the same results
            self.assertEqual(name, reverse_name)
            self.assertEqual(type_code, reverse_type)
            self.assertEqual(display_size, reverse_display_size)
            self.assertEqual(internal_size, reverse_internal_size)
            self.assertEqual(precision, reverse_precision)
            self.assertEqual(scale, reverse_scale)
            self.assertEqual(null_ok, reverse_null_ok)

            # Check that attribute access works
            self.assertEqual(col_desc.name, name)
            self.assertEqual(col_desc.type, type_code)
            self.assertEqual(col_desc.display_size, display_size)
            self.assertEqual(col_desc.internal_size, internal_size)
            self.assertEqual(col_desc.precision, precision)
            self.assertEqual(col_desc.scale, scale)
            self.assertEqual(col_desc.null_ok, null_ok)
            self.assertIsNone(col_desc.vector_dimension)
            self.assertIsNone(col_desc.vector_format)

            # Test unpacking (a, b, c = fetchinfo)
            (
                unpacked_name,
                unpacked_type,
                unpacked_display_size,
                unpacked_internal_size,
                unpacked_precision,
                unpacked_scale,
                unpacked_null_ok,
            ) = col_desc
            self.assertEqual(unpacked_name, col_desc.name)
            self.assertEqual(unpacked_type, col_desc.type)
            self.assertEqual(unpacked_display_size, col_desc.display_size)
            self.assertEqual(unpacked_internal_size, col_desc.internal_size)
            self.assertEqual(unpacked_precision, col_desc.precision)
            self.assertEqual(unpacked_scale, col_desc.scale)
            self.assertEqual(unpacked_null_ok, col_desc.null_ok)

        # Clean up
        self.cursor.execute("DROP TABLE IF EXISTS test_all_types")

    def test_fetchinfo_vector_types(self):
        # Skip this test if VECTOR type is not supported
        try:
            # Create a table with VECTOR column
            self.cursor.execute("DROP TABLE IF EXISTS test_vector_fetchinfo")
            self.cursor.execute("CREATE TABLE test_vector_fetchinfo (id INT, vec VECTOR(8, FLOAT32))")

            # Execute a query with VECTOR column
            self.cursor.execute("SELECT vec FROM test_vector_fetchinfo WHERE id = 1")

            # Get description
            description = self.cursor.description

            # Check that we have the expected number of columns
            self.assertEqual(len(description), 1)

            # Get the VECTOR column description
            vector_desc = description[0]

            # For VECTOR types, vector_dimension and vector_format should have values
            self.assertEqual(vector_desc.vector_dimension, 8)
            self.assertEqual(vector_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT32)
            # Also check that vector_format equals the numeric value
            self.assertEqual(vector_desc.vector_format, 2)  # YAPI_VECTOR_FORMAT_FLOAT32

            # Test indexing access (forward)
            self.assertEqual(vector_desc[0], "VEC")  # name
            self.assertEqual(vector_desc[1], 42)  # type (YAPI_TYPE_VECTOR)
            # display_size, internal_size, precision, scale, null_ok are checked below

            # Test indexing access (reverse)
            self.assertEqual(vector_desc[-7], "VEC")  # name
            self.assertEqual(vector_desc[-6], 42)  # type (YAPI_TYPE_VECTOR)
            # display_size = vector_desc[-5]
            # internal_size = vector_desc[-4]
            # precision = vector_desc[-3]
            # scale = vector_desc[-2]
            # null_ok = vector_desc[-1]

            # Test attribute access
            self.assertEqual(vector_desc.name, "VEC")
            self.assertEqual(vector_desc.type, 42)  # YAPI_TYPE_VECTOR
            # display_size, internal_size, precision, scale, null_ok are checked below
            self.assertEqual(vector_desc.vector_dimension, 8)
            self.assertEqual(vector_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT32)

            # Test length
            self.assertEqual(len(vector_desc), 7)

            # Test iteration (variable binding resolution)
            for i, col_desc in enumerate(description):
                self.assertEqual(len(col_desc), 7)
                self.assertEqual(col_desc.vector_dimension, 8)
                self.assertEqual(col_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT32)

                # Check that we can access all elements by index (forward)
                name = col_desc[0]
                type_code = col_desc[1]
                display_size = col_desc[2]
                internal_size = col_desc[3]
                precision = col_desc[4]
                scale = col_desc[5]
                null_ok = col_desc[6]

                # Check that we can access all elements by index (reverse)
                reverse_name = col_desc[-7]
                reverse_type = col_desc[-6]
                reverse_display_size = col_desc[-5]
                reverse_internal_size = col_desc[-4]
                reverse_precision = col_desc[-3]
                reverse_scale = col_desc[-2]
                reverse_null_ok = col_desc[-1]

                # Verify forward and reverse indexing give the same results
                self.assertEqual(name, reverse_name)
                self.assertEqual(type_code, reverse_type)
                self.assertEqual(display_size, reverse_display_size)
                self.assertEqual(internal_size, reverse_internal_size)
                self.assertEqual(precision, reverse_precision)
                self.assertEqual(scale, reverse_scale)
                self.assertEqual(null_ok, reverse_null_ok)

                # Check that attribute access works
                self.assertEqual(col_desc.name, name)
                self.assertEqual(col_desc.type, type_code)
                self.assertEqual(col_desc.display_size, display_size)
                self.assertEqual(col_desc.internal_size, internal_size)
                self.assertEqual(col_desc.precision, precision)
                self.assertEqual(col_desc.scale, scale)
                self.assertEqual(col_desc.null_ok, null_ok)
                self.assertEqual(col_desc.vector_dimension, 8)
                self.assertEqual(col_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT32)

                # Test unpacking (a, b, c = fetchinfo)
                (
                    unpacked_name,
                    unpacked_type,
                    unpacked_display_size,
                    unpacked_internal_size,
                    unpacked_precision,
                    unpacked_scale,
                    unpacked_null_ok,
                ) = col_desc
                self.assertEqual(unpacked_name, col_desc.name)
                self.assertEqual(unpacked_type, col_desc.type)
                self.assertEqual(unpacked_display_size, col_desc.display_size)
                self.assertEqual(unpacked_internal_size, col_desc.internal_size)
                self.assertEqual(unpacked_precision, col_desc.precision)
                self.assertEqual(unpacked_scale, col_desc.scale)
                self.assertEqual(unpacked_null_ok, col_desc.null_ok)
                self.assertEqual(col_desc.vector_dimension, 8)
                self.assertEqual(col_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT32)
                print(col_desc.vector_dimension, col_desc.vector_format, type(col_desc.vector_format))

                # Test backward compatibility - comparing with tuple
                expected_tuple = (
                    col_desc.name,
                    col_desc.type,
                    col_desc.display_size,
                    col_desc.internal_size,
                    col_desc.precision,
                    col_desc.scale,
                    col_desc.null_ok,
                )
                self.assertEqual(col_desc, expected_tuple)
                self.assertEqual(expected_tuple, col_desc)

                # Test backward compatibility - comparing with list
                expected_list = list(expected_tuple)
                self.assertEqual(col_desc, expected_list)
                self.assertEqual(expected_list, col_desc)

                # Test inequality
                wrong_tuple = ("wrong",) + expected_tuple[1:]
                self.assertNotEqual(col_desc, wrong_tuple)
                self.assertNotEqual(wrong_tuple, col_desc)

            # Clean up
            self.cursor.execute("DROP TABLE IF EXISTS test_vector_fetchinfo")

            # Test FLOAT64 format
            # Create a table with VECTOR column
            self.cursor.execute("DROP TABLE IF EXISTS test_vector_fetchinfo_float64")
            self.cursor.execute("CREATE TABLE test_vector_fetchinfo_float64 (id INT, vec VECTOR(8, FLOAT64))")

            # Execute a query with VECTOR column
            self.cursor.execute("SELECT vec FROM test_vector_fetchinfo_float64 WHERE id = 1")

            # Get description
            description = self.cursor.description

            # Check that we have the expected number of columns
            self.assertEqual(len(description), 1)

            # Get the VECTOR column description
            vector_desc = description[0]

            # For VECTOR types, vector_dimension and vector_format should have values
            self.assertEqual(vector_desc.vector_dimension, 8)
            self.assertEqual(vector_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT64)
            # Also check that vector_format equals the numeric value
            self.assertEqual(vector_desc.vector_format, 3)  # YAPI_VECTOR_FORMAT_FLOAT64

            # Test iteration (variable binding resolution)
            for i, col_desc in enumerate(description):
                self.assertEqual(len(col_desc), 7)
                self.assertEqual(col_desc.vector_dimension, 8)
                self.assertEqual(col_desc.vector_format, yaspy.VECTOR_FORMAT_FLOAT64)
                # Also check that vector_format equals the numeric value
                self.assertEqual(col_desc.vector_format, 3)  # YAPI_VECTOR_FORMAT_FLOAT64

            # Clean up
            self.cursor.execute("DROP TABLE IF EXISTS test_vector_fetchinfo_float64")
        except yaspy.DatabaseError as e:
            # If VECTOR type is not supported, skip the test
            self.skipTest("VECTOR type not supported: " + str(e))


if __name__ == "__main__":
    test_base.run_test_cases()

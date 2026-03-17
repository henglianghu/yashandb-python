#------------------------------------------------------------------------------
# Copyright (c) 2021, Oracle and/or its affiliates. All rights reserved.
#------------------------------------------------------------------------------

"""
3700 - Module for testing all variable types.
"""

import datetime
import decimal
import time
import unittest

import yaspy
import test_env

class TestCase(test_env.BaseTestCase):

    def _test_positive_set_and_get(self, var_type, value_to_set, expected_value):
        var = self.cursor.var(var_type)
        var.setvalue(value_to_set)
        result = var.getvalue()
        self.assertEqual(result, expected_value)

    def _test_negative_set_and_get(self, var_type, value_to_set):
        var = self.cursor.var(var_type)
        self.assertRaises((TypeError, yaspy.DatabaseError),
                           var.setvalue, value_to_set)

    def test_3700_DB_TYPE_NUMBER(self):
        "3700 - setting values on variables of type DB_TYPE_NUMBER"
        self._test_positive_set_and_get(yaspy.NUMBER, 5, '5')
        self._test_positive_set_and_get(yaspy.NUMBER, 3.5, '3.5')
        self._test_positive_set_and_get(yaspy.NUMBER,
                                        decimal.Decimal("24.8"), '24.8')
        # self._test_positive_set_and_get(yaspy.NUMBER, True, '1')
        # self._test_positive_set_and_get(yaspy.NUMBER, False, '0')
        self._test_positive_set_and_get(yaspy.NUMBER, None, None)
        # self._test_negative_set_and_get(yaspy.NUMBER, "abc")

    def test_3701_DB_TYPE_BINARY_INTEGER(self):
        "3701 - setting values on variables of type DB_TYPE_BINARY_INTEGER"
        self._test_positive_set_and_get(yaspy.INTEGER, 5, 5)
        # self._test_positive_set_and_get(yaspy.INTEGER, 3.5, 3)
        # self._test_positive_set_and_get(yaspy.INTEGER, decimal.Decimal("24.8"), 24)
        # self._test_positive_set_and_get(yaspy.INTEGER, True, 1)
        # self._test_positive_set_and_get(yaspy.INTEGER, False, 0)
        self._test_positive_set_and_get(yaspy.INTEGER, None, None)
        # self._test_negative_set_and_get(yaspy.INTEGER, "abc")

    def test_3702_DB_TYPE_VARCHAR(self):
        "3702 - setting values on variables of type DB_TYPE_VARCHAR"
        value = "A VARCHAR string"
        self._test_positive_set_and_get(yaspy.VARCHAR, value, value)
        value = b"A raw string for VARCHAR"
        self._test_positive_set_and_get(yaspy.VARCHAR, value, value.decode())
        self._test_positive_set_and_get(yaspy.VARCHAR, None, None)
        # self._test_negative_set_and_get(yaspy.VARCHAR, 5)

    def test_3703_DB_TYPE_NVARCHAR(self):
        "3703 - setting values on variables of type DB_TYPE_NVARCHAR"
        value = "A NVARCHAR string"
        self._test_positive_set_and_get(yaspy.NVARCHAR, value, value)
        value = b"A raw string for NVARCHAR"
        self._test_positive_set_and_get(yaspy.NVARCHAR, value, value.decode())
        self._test_positive_set_and_get(yaspy.NVARCHAR, None, None)
        # self._test_negative_set_and_get(yaspy.DB_TYPE_NVARCHAR, 5)

    def test_3704_DB_TYPE_CHAR(self):
        "3704 - setting values on variables of type DB_TYPE_CHAR"
        value = "A CHAR string"
        self._test_positive_set_and_get(yaspy.CHAR, value, value)
        value = b"A raw string for CHAR"
        self._test_positive_set_and_get(yaspy.CHAR, value,
                                        value.decode())
        self._test_positive_set_and_get(yaspy.CHAR, None, None)
        # self._test_negative_set_and_get(yaspy.DB_TYPE_CHAR, 5)

    def test_3705_DB_TYPE_NCHAR(self):
        "3705 - setting values on variables of type DB_TYPE_NCHAR"
        value = "A NCHAR string"
        self._test_positive_set_and_get(yaspy.NCHAR, value, value)
        value = b"A raw string for NCHAR"
        self._test_positive_set_and_get(yaspy.CHAR, value,
                                        value.decode())
        self._test_positive_set_and_get(yaspy.NCHAR, None, None)
        # self._test_negative_set_and_get(yaspy.DB_TYPE_NCHAR, 5)

    @unittest.skip
    def test_3706_DB_TYPE_LONG(self):
        "3706 - setting values on variables of type DB_TYPE_LONG"
        value = "Long Data" * 15000
        self._test_positive_set_and_get(yaspy.DB_TYPE_LONG, value, value)
        value = b"Raw data for LONG" * 15000
        self._test_positive_set_and_get(yaspy.DB_TYPE_LONG, value,
                                        value.decode())
        self._test_positive_set_and_get(yaspy.DB_TYPE_LONG, None, None)
        self._test_negative_set_and_get(yaspy.DB_TYPE_LONG, 5)

    def test_3707_DB_TYPE_RAW(self):
        "3707 - setting values on variables of type DB_TYPE_RAW"
        value = b'Raw Data'
        self._test_positive_set_and_get(yaspy.BINARY, value, value)
        value = "String data for RAW"
        self._test_positive_set_and_get(yaspy.BINARY, value,
                                        value.encode())
        self._test_positive_set_and_get(yaspy.BINARY, None, None)
        # self._test_negative_set_and_get(yaspy.BINARY, 5)

    @unittest.skip
    def test_3708_DB_TYPE_LONG_RAW(self):
        "3708 - setting values on variables of type DB_TYPE_LONG_RAW"
        value = b'Long Raw Data' * 15000
        self._test_positive_set_and_get(yaspy.DB_TYPE_LONG_RAW, value,
                                        value)
        value = "String data for LONG RAW" * 15000
        self._test_positive_set_and_get(yaspy.DB_TYPE_LONG_RAW, value,
                                        value.encode())
        self._test_positive_set_and_get(yaspy.DB_TYPE_LONG_RAW, None, None)
        self._test_negative_set_and_get(yaspy.DB_TYPE_LONG_RAW, 5)

    def test_3709_DB_TYPE_DATE(self):
        "3709 - setting values on variables of type DB_TYPE_DATE"
        self._test_positive_set_and_get(yaspy.DATE,
                                        datetime.date(2017, 5, 6),
                                        datetime.date(2017, 5, 6))
        value = datetime.datetime(2017, 5, 6, 9, 36, 0)
        self._test_positive_set_and_get(yaspy.DATE, value, datetime.date(2017, 5, 6))
        self._test_positive_set_and_get(yaspy.DATE, None, None)
        # self._test_negative_set_and_get(yaspy.DATE, 5)

    def test_3710_DB_TYPE_TIMESTAMP(self):
        "3710 - setting values on variables of type DB_TYPE_TIMESTAMP"
        self._test_positive_set_and_get(yaspy.DATETIME,
                                        datetime.date(2017, 5, 6),
                                        datetime.datetime(2017, 5, 6))
        value = datetime.datetime(2017, 5, 6, 9, 36, 0, 300000)
        self._test_positive_set_and_get(yaspy.DATETIME, value,
                                        value)
        self._test_positive_set_and_get(yaspy.DATETIME, None, None)
        # self._test_negative_set_and_get(yaspy.DATETIME, 5)

    @unittest.skip
    def test_3711_DB_TYPE_TIMESTAMP_TZ(self):
        "3711 - setting values on variables of type DB_TYPE_TIMESTAMP_TZ"
        self._test_positive_set_and_get(yaspy.DB_TYPE_TIMESTAMP_TZ,
                                        datetime.date(2017, 5, 6),
                                        datetime.datetime(2017, 5, 6))
        value = datetime.datetime(2017, 5, 6, 9, 36, 0, 300000)
        self._test_positive_set_and_get(yaspy.DB_TYPE_TIMESTAMP_TZ, value,
                                        value)
        self._test_positive_set_and_get(yaspy.DB_TYPE_TIMESTAMP_TZ,
                                        None, None)
        self._test_negative_set_and_get(yaspy.DB_TYPE_TIMESTAMP_TZ, 5)

    @unittest.skip
    def test_3712_DB_TYPE_TIMESTAMP_LTZ(self):
        "3712 - setting values on variables of type DB_TYPE_TIMESTAMP_LTZ"
        self._test_positive_set_and_get(yaspy.DB_TYPE_TIMESTAMP_LTZ,
                                        datetime.date(2017, 5, 6),
                                        datetime.datetime(2017, 5, 6))
        value = datetime.datetime(2017, 5, 6, 9, 36, 0, 300000)
        self._test_positive_set_and_get(yaspy.DB_TYPE_TIMESTAMP_LTZ, value,
                                        value)
        self._test_positive_set_and_get(yaspy.DB_TYPE_TIMESTAMP_LTZ,
                                        None, None)
        self._test_negative_set_and_get(yaspy.DB_TYPE_TIMESTAMP_LTZ, 5)

    def test_3713_DB_TYPE_BLOB(self):
        "3713 - setting values on variables of type DB_TYPE_BLOB"
        value = b'Short temp BLOB value'
        # temp_blob = self.connection.createlob(yaspy.DB_TYPE_BLOB)
        # temp_blob.write(value)
        temp_blob = value
        self._test_positive_set_and_get(yaspy.BLOB, temp_blob,
                                        value)
        # self._test_negative_set_and_get(yaspy.CLOB, temp_blob)
        # self._test_negative_set_and_get(yaspy.NCLOB, temp_blob)
        value = b'Short BLOB value'
        self._test_positive_set_and_get(yaspy.BLOB, value, value)
        self._test_positive_set_and_get(yaspy.BLOB, None, None)
        # self._test_negative_set_and_get(yaspy.BLOB, 5)

    def test_3714_DB_TYPE_CLOB(self):
        "3714 - setting values on variables of type DB_TYPE_CLOB"
        value = 'Short temp CLOB value'
        # temp_clob = self.connection.createlob(yaspy.CLOB)
        # temp_clob.write(value)
        temp_clob = value
        self._test_positive_set_and_get(yaspy.CLOB, temp_clob,
                                        value)
        # self._test_negative_set_and_get(yaspy.BLOB, temp_clob)
        # self._test_negative_set_and_get(yaspy.NCLOB, temp_clob)
        value = 'Short CLOB value'
        self._test_positive_set_and_get(yaspy.CLOB, value, value)
        self._test_positive_set_and_get(yaspy.CLOB, None, None)
        # self._test_negative_set_and_get(yaspy.CLOB, 5)

    def test_3715_DB_TYPE_NCLOB(self):
        "3715 - setting values on variables of type DB_TYPE_NCLOB"
        value = 'Short temp NCLOB value'
        # temp_nclob = self.connection.createlob(yaspy.DB_TYPE_NCLOB)
        # temp_nclob.write(value)
        temp_nclob = value
        self._test_positive_set_and_get(yaspy.NCLOB, temp_nclob,
                                        value)
        # self._test_negative_set_and_get(yaspy.BLOB, temp_nclob)
        # self._test_negative_set_and_get(yaspy.CLOB, temp_nclob)
        value = 'Short NCLOB Value'
        self._test_positive_set_and_get(yaspy.NCLOB, value, value)
        self._test_positive_set_and_get(yaspy.NCLOB, None, None)
        # self._test_negative_set_and_get(yaspy.NCLOB, 5)

    def test_3716_DB_TYPE_BINARY_FLOAT(self):
        "3716 - setting values on variables of type DB_TYPE_BINARY_FLOAT"
        # self._test_positive_set_and_get(yaspy.DB_TYPE_BINARY_FLOAT, 5, 5.0)
        self._test_positive_set_and_get(yaspy.FLOAT, 3.5, 3.5)
        # self._test_positive_set_and_get(yaspy.FLOAT, decimal.Decimal("24.5"), 24.5)
        # self._test_positive_set_and_get(yaspy.DB_TYPE_BINARY_FLOAT, True, 1.0)
        # self._test_positive_set_and_get(yaspy.DB_TYPE_BINARY_FLOAT, False, 0.0)
        self._test_positive_set_and_get(yaspy.FLOAT, None, None)
        # self._test_negative_set_and_get(yaspy.DB_TYPE_BINARY_FLOAT, "abc")

    def test_3717_DB_TYPE_BINARY_DOUBLE(self):
        "3717 - setting values on variables of type DB_TYPE_BINARY_DOUBLE"
        # self._test_positive_set_and_get(yaspy.DOUBLE, 5, 5.0)
        self._test_positive_set_and_get(yaspy.DOUBLE, 3.5, 3.5)
        # self._test_positive_set_and_get(yaspy.DOUBLE, decimal.Decimal("192.125"), 192.125)
        # self._test_positive_set_and_get(yaspy.DOUBLE, True, 1.0)
        # self._test_positive_set_and_get(yaspy.DOUBLE, False, 0.0)
        self._test_positive_set_and_get(yaspy.DOUBLE, None, None)
        # self._test_negative_set_and_get(yaspy.DOUBLE, "abc")

    def test_3718_DB_TYPE_BOOLEAN(self):
        "3718 - setting values on variables of type DB_TYPE_BOOLEAN"
        self._test_positive_set_and_get(yaspy.BOOL, 5, True)
        self._test_positive_set_and_get(yaspy.BOOL, 2.0, True)
        self._test_positive_set_and_get(yaspy.BOOL, "abc", True)
        self._test_positive_set_and_get(yaspy.BOOL, decimal.Decimal("24.8"), True)
        self._test_positive_set_and_get(yaspy.BOOL, 0.0, False)
        self._test_positive_set_and_get(yaspy.BOOL, 0, False)
        self._test_positive_set_and_get(yaspy.BOOL, None, None)

    def test_3719_DB_TYPE_INTERVAL_DS(self):
        "3719 - setting values on variables of type DB_TYPE_INTERVAL_DS"
        value = datetime.timedelta(days=5, seconds=56000, microseconds=123780)
        self._test_positive_set_and_get(yaspy.TIMEDELTA, value, value)
        self._test_positive_set_and_get(yaspy.TIMEDELTA, None, None)
        # self._test_negative_set_and_get(yaspy.TIMEDELTA, 5)

    @unittest.skip
    def test_3720_DB_TYPE_ROWID(self):
        "3720 - setting values on variables of type DB_TYPE_ROWID"
        self._test_negative_set_and_get(yaspy.ROWID, 12345)
        self._test_negative_set_and_get(yaspy.ROWID, "523lkhlf")

    @unittest.skip
    def test_3721_DB_TYPE_OBJECT(self):
        "3721 - setting values on variables of type DB_TYPE_OBJECT"
        obj_type = self.connection.gettype("UDT_OBJECT")
        obj = obj_type.newobject()
        plain_obj = self.get_db_object_as_plain_object(obj)
        self._test_positive_set_and_get(yaspy.DB_TYPE_OBJECT,
                                        obj, plain_obj, "UDT_OBJECT")
        self._test_positive_set_and_get(obj_type, obj, plain_obj)
        self._test_positive_set_and_get(yaspy.DB_TYPE_OBJECT,
                                        None, None, "UDT_OBJECT")
        self._test_positive_set_and_get(obj_type, None, None)
        self._test_negative_set_and_get(yaspy.DB_TYPE_OBJECT,
                                        "abc", "UDT_OBJECT")
        self._test_negative_set_and_get(yaspy.DB_TYPE_OBJECT, obj,
                                        "UDT_OBJECTARRAY")
        wrong_obj_type = self.connection.gettype("UDT_OBJECTARRAY")
        self._test_negative_set_and_get(wrong_obj_type, obj)

    def test_3722_DB_TYPE_JSON(self):
        "3722 - setting values on variables of type DB_TYPE_JSON"
        json_data = [
            5,
            25.25,
            decimal.Decimal("10.25"),
            True,
            False,
            datetime.datetime(2017, 5, 6),
            datetime.datetime(2017, 5, 6, 9, 36, 0, 300000),
            datetime.timedelta(days=5, seconds=56000, microseconds=123780),
            {},
            'String',
            b'Some bytes',
            {'keyA': 1, 'KeyB': 'Melbourne'},
            [],
            [1, "A"],
            {"name": None},
            {"name": "John"},
            {"age": 30},
            {"Permanent": True},
            {
                "employee": {
                    "name":"John",
                    "age": 30,
                    "city": "Delhi",
                    "Parmanent": True
                }
            },
            {
                "employees": ["John", "Matthew", "James"]
            },
            {
                "employees": [
                    {
                        "employee1": {"name": "John", "city": "Delhi"}
                    },
                    {
                        "employee2": {"name": "Matthew", "city": "Mumbai"}
                    },
                    {
                        "employee3": {"name": "James", "city": "Bangalore"}
                    }
                ]
            }
        ]
        # self._test_positive_set_and_get(yaspy.DB_TYPE_JSON, json_data, json_data)
        self._test_positive_set_and_get(yaspy.JSON, None, None)

    @unittest.skip
    def test_3723_DB_TYPE_CURSOR(self):
        "3723 - test setting values on variables of type DB_TYPE_CURSOR"
        cursor = self.connection.cursor()
        var = self.cursor.var(yaspy.DB_TYPE_CURSOR)
        var.setvalue(0, cursor)
        self._test_positive_set_and_get(yaspy.DB_TYPE_CURSOR, None, None)
        self._test_negative_set_and_get(yaspy.DB_TYPE_CURSOR, 5)

if __name__ == "__main__":
    test_env.run_test_cases()

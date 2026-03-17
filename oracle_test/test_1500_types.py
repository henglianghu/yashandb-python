#------------------------------------------------------------------------------
# Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
#------------------------------------------------------------------------------

"""
1500 - Module for testing comparisons with database types and API types,
including the synonyms retained for backwards compatibility. This module also
tests for pickling/unpickling of database types and API types.
"""

import pickle
import unittest

import yaspy
import test_env

class TestCase(test_env.BaseTestCase):
    requires_connection = False

    def __test_compare(self, db_type, api_type):
        self.assertEqual(db_type, db_type)
        self.assertEqual(db_type, api_type)
        self.assertEqual(api_type, db_type)
        self.assertNotEqual(db_type, 5)
        self.assertNotEqual(db_type, yaspy.DB_TYPE_OBJECT)

    def __test_pickle(self, typ):
        self.assertIs(typ, pickle.loads(pickle.dumps(typ)))

    @unittest.skip
    def test_1500_DB_TYPE_BFILE(self):
        "1500 - test yaspy.DB_TYPE_BFILE comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_BFILE, yaspy.BFILE)
        self.__test_pickle(yaspy.DB_TYPE_BFILE)

    @unittest.skip
    def test_1501_DB_TYPE_BINARY_DOUBLE(self):
        "1501 - test yaspy.DB_TYPE_BINARY_DOUBLE comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_BINARY_DOUBLE, yaspy.NUMBER)
        self.assertEqual(yaspy.DB_TYPE_BINARY_DOUBLE,
                yaspy.NATIVE_FLOAT)
        self.__test_pickle(yaspy.DB_TYPE_BINARY_DOUBLE)

    @unittest.skip
    def test_1502_DB_TYPE_BINARY_FLOAT(self):
        "1502 - test yaspy.DB_TYPE_BINARY_FLOAT comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_BINARY_FLOAT, yaspy.NUMBER)
        self.__test_pickle(yaspy.DB_TYPE_BINARY_FLOAT)

    @unittest.skip
    def test_1503_DB_TYPE_BINARY_INTEGER(self):
        "1503 - test yaspy.DB_TYPE_BINARY_INTEGER comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_BINARY_INTEGER, yaspy.NUMBER)
        self.assertEqual(yaspy.DB_TYPE_BINARY_INTEGER,
                yaspy.NATIVE_INT)
        self.__test_pickle(yaspy.DB_TYPE_BINARY_INTEGER)

    @unittest.skip
    def test_1504_DB_TYPE_BLOB(self):
        "1504 - test yaspy.DB_TYPE_BLOB comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_BLOB, yaspy.BLOB)
        self.__test_pickle(yaspy.DB_TYPE_BLOB)

    @unittest.skip
    def test_1505_DB_TYPE_BOOLEAN(self):
        "1505 - test yaspy.DB_TYPE_BOOLEAN comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_BOOLEAN, yaspy.BOOLEAN)
        self.__test_pickle(yaspy.DB_TYPE_BOOLEAN)

    @unittest.skip
    def test_1506_DB_TYPE_CHAR(self):
        "1506 - test yaspy.DB_TYPE_CHAR comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_CHAR, yaspy.STRING)
        self.assertEqual(yaspy.DB_TYPE_CHAR, yaspy.FIXED_CHAR)
        self.__test_pickle(yaspy.DB_TYPE_CHAR)

    @unittest.skip
    def test_1507_DB_TYPE_CLOB(self):
        "1507 - test yaspy.DB_TYPE_CLOB comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_CLOB, yaspy.CLOB)
        self.__test_pickle(yaspy.DB_TYPE_CLOB)

    @unittest.skip
    def test_1508_DB_TYPE_CURSOR(self):
        "1508 - test yaspy.DB_TYPE_CURSOR comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_CURSOR, yaspy.CURSOR)
        self.__test_pickle(yaspy.DB_TYPE_CURSOR)

    @unittest.skip
    def test_1509_DB_TYPE_DATE(self):
        "1509 - test yaspy.DB_TYPE_DATE comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_DATE, yaspy.DATETIME)
        self.__test_pickle(yaspy.DB_TYPE_DATE)

    @unittest.skip
    def test_1510_DB_TYPE_INTERVAL_DS(self):
        "1510 - test yaspy.DB_TYPE_INTERVAL_DS comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_INTERVAL_DS, yaspy.INTERVAL)
        self.__test_pickle(yaspy.DB_TYPE_INTERVAL_DS)

    @unittest.skip
    def test_1511_DB_TYPE_LONG(self):
        "1511 - test yaspy.DB_TYPE_LONG comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_LONG, yaspy.STRING)
        self.assertEqual(yaspy.DB_TYPE_LONG, yaspy.LONG_STRING)
        self.__test_pickle(yaspy.DB_TYPE_LONG)

    @unittest.skip
    def test_1512_DB_TYPE_LONG_RAW(self):
        "1512 - test yaspy.DB_TYPE_LONG_RAW comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_LONG_RAW, yaspy.BINARY)
        self.assertEqual(yaspy.DB_TYPE_LONG_RAW, yaspy.LONG_BINARY)
        self.__test_pickle(yaspy.DB_TYPE_LONG_RAW)

    @unittest.skip
    def test_1513_DB_TYPE_NCHAR(self):
        "1513 - test yaspy.DB_TYPE_NCHAR comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_NCHAR, yaspy.STRING)
        self.assertEqual(yaspy.DB_TYPE_NCHAR, yaspy.FIXED_NCHAR)
        self.__test_pickle(yaspy.DB_TYPE_NCHAR)

    @unittest.skip
    def test_1514_DB_TYPE_NCLOB(self):
        "1514 - test yaspy.DB_TYPE_NCLOB comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_NCLOB, yaspy.NCLOB)
        self.__test_pickle(yaspy.DB_TYPE_NCLOB)

    @unittest.skip
    def test_1515_DB_TYPE_NUMBER(self):
        "1515 - test yaspy.DB_TYPE_NUMBER comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_NUMBER, yaspy.NUMBER)
        self.__test_pickle(yaspy.DB_TYPE_NUMBER)

    @unittest.skip
    def test_1516_DB_TYPE_NVARCHAR(self):
        "1516 - test yaspy.DB_TYPE_NVARCHAR comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_NVARCHAR, yaspy.STRING)
        self.assertEqual(yaspy.DB_TYPE_NVARCHAR, yaspy.NCHAR)
        self.__test_pickle(yaspy.DB_TYPE_NVARCHAR)

    @unittest.skip
    def test_1517_DB_TYPE_OBJECT(self):
        "1517 - test yaspy.DB_TYPE_OBJECT comparisons and pickling"
        self.assertEqual(yaspy.DB_TYPE_OBJECT, yaspy.OBJECT)
        self.__test_pickle(yaspy.DB_TYPE_OBJECT)

    @unittest.skip
    def test_1518_DB_TYPE_RAW(self):
        "1518 - test yaspy.DB_TYPE_RAW comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_RAW, yaspy.BINARY)
        self.__test_pickle(yaspy.DB_TYPE_RAW)

    @unittest.skip
    def test_1519_DB_TYPE_ROWID(self):
        "1519 - test yaspy.DB_TYPE_ROWID comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_ROWID, yaspy.ROWID)
        self.__test_pickle(yaspy.DB_TYPE_ROWID)

    @unittest.skip
    def test_1520_DB_TYPE_TIMESTAMP(self):
        "1520 - test yaspy.DB_TYPE_TIMESTAMP comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_TIMESTAMP, yaspy.DATETIME)
        self.assertEqual(yaspy.DB_TYPE_TIMESTAMP, yaspy.TIMESTAMP)
        self.__test_pickle(yaspy.DB_TYPE_TIMESTAMP)

    @unittest.skip
    def test_1521_DB_TYPE_TIMESTAMP_LTZ(self):
        "1521 - test yaspy.DB_TYPE_TIMESTAMP_LTZ comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_TIMESTAMP_LTZ, yaspy.DATETIME)
        self.__test_pickle(yaspy.DB_TYPE_TIMESTAMP_LTZ)

    @unittest.skip
    def test_1522_DB_TYPE_TIMESTAMP_TZ(self):
        "1522 - test yaspy.DB_TYPE_TIMESTAMP_TZ comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_TIMESTAMP_TZ, yaspy.DATETIME)
        self.__test_pickle(yaspy.DB_TYPE_TIMESTAMP_TZ)

    @unittest.skip
    def test_1523_DB_TYPE_VARCHAR(self):
        "1523 - test yaspy.DB_TYPE_VARCHAR comparisons and pickling"
        self.__test_compare(yaspy.DB_TYPE_VARCHAR, yaspy.STRING)
        self.__test_pickle(yaspy.DB_TYPE_VARCHAR)

    @unittest.skip
    def test_1524_NUMBER(self):
        "1524 - test yaspy.NUMBER pickling"
        self.__test_pickle(yaspy.NUMBER)

    @unittest.skip
    def test_1525_STRING(self):
        "1525 - test yaspy.STRING pickling"
        self.__test_pickle(yaspy.STRING)

    @unittest.skip
    def test_1526_DATETIME(self):
        "1526 - test yaspy.DATETIME pickling"
        self.__test_pickle(yaspy.DATETIME)

    @unittest.skip
    def test_1527_BINARY(self):
        "1527 - test yaspy.BINARY pickling"
        self.__test_pickle(yaspy.BINARY)

    @unittest.skip
    def test_1528_ROWID(self):
        "1528 - test yaspy.ROWID pickling"
        self.__test_pickle(yaspy.ROWID)

if __name__ == "__main__":
    test_env.run_test_cases()

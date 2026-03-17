#------------------------------------------------------------------------------
# Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
#
# Portions Copyright 2007-2015, Anthony Tuininga. All rights reserved.
#
# Portions Copyright 2001-2007, Computronix (Canada) Ltd., Edmonton, Alberta,
# Canada. All rights reserved.
#------------------------------------------------------------------------------

"""
1700 - Module for testing error objects
"""

import pickle

import yaspy
import test_env

class TestCase(test_env.BaseTestCase):

    def test_1700_parse_error(self):
        "1700 - test parse error returns offset correctly"
        with self.assertRaises(yaspy.Error) as cm:
            self.cursor.execute("begin t_Missing := 5; end;")
        error_obj, = cm.exception.args
        self.assertEqual(error_obj.code, 4253)
        self.assertEqual(error_obj.line, 0)
        self.assertEqual(error_obj.column, 0)

    def test_1701_pickle_error(self):
        "1701 - test picking/unpickling an error object"
        with self.assertRaises(yaspy.Error) as cm:
            self.cursor.execute("""
                    begin
                        raise_application_error(-20101, 'Test!');
                    end;""")
        error_obj, = cm.exception.args
        self.assertEqual(type(error_obj), yaspy._Error)
        print(error_obj.message)
        self.assertTrue("YAS-" in error_obj.message)
        self.assertEqual(error_obj.code, 6805)
        self.assertEqual(error_obj.line, 3)
        self.assertEqual(error_obj.column, 50)
        new_error_obj = pickle.loads(pickle.dumps(error_obj))
        self.assertEqual(type(new_error_obj), yaspy._Error)
        self.assertTrue(new_error_obj.message == error_obj.message)
        self.assertTrue(new_error_obj.code == error_obj.code)
        self.assertTrue(new_error_obj.line == error_obj.line)
        self.assertTrue(new_error_obj.column == error_obj.column)

if __name__ == "__main__":
    test_env.run_test_cases()

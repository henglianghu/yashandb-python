import yaspy as yashanDB
import test_base

class TestCase(test_base.TestBaseCase):

    def test_insert_returning1(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_dml_rt1")
        cursor.execute("create table test_dml_rt1(c1 int, c2 varchar(100))")

        int_var = cursor.var(yashanDB.INTEGER)
        cursor.execute("insert into test_dml_rt1 values(?, ?) returning c1 into ?", (1, 'abc', int_var))

        data = [1]
        self.assertEqual(data, int_var.values)
        cursor.execute("drop table if exists test_dml_rt1")

    
if __name__ == "__main__":
    test_base.run_test_cases()

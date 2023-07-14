
import test_base

class TestCase(test_base.TestBaseCase):

    def test_error_code(self):
        cursor = self.connection.cursor()
        try:
            cursor.execute("drop table test_error_code")
        except Exception as e:
            error = str(e)
            expectMsg = "YAS-02012 table or view does not exist"
            self.assertTrue(expectMsg in error)

    def test_bind_error_code(self):
        cursor = self.connection.cursor()
        cursor.execute("drop table if exists test_bind_error")
        cursor.execute("create table test_bind_error(c1 int)")
        cursor.execute("insert into test_bind_error values(1)")
        try:
            cursor.execute("select * from test_bind_error where c1 = ?", ('abc', ))
        except Exception as e:
            error = str(e)
            expectMsg = "YAS-00008 type convert error : not a valid number"
            self.assertTrue(expectMsg in error)



if __name__ == "__main__":
    test_base.run_test_cases()

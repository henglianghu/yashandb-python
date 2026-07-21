import os
import subprocess
import sys
import textwrap

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import test_base


class TestCase(test_base.TestBaseCase):
    need_connection = False

    def _connection_info(self):
        dsn = os.environ.get("YASPY_TEST_DSN")
        if dsn is None:
            dsn = os.environ.get(
                "YASPY_TEST_CONNECT_STRING",
                f"{test_base.TestBaseCase.host}:{test_base.TestBaseCase.port}",
            )
        user = os.environ.get(
            "YASPY_TEST_USER",
            os.environ.get("YASPY_TEST_MAIN_USER", test_base.TestBaseCase.user),
        )
        password = os.environ.get(
            "YASPY_TEST_PASSWORD",
            os.environ.get("YASPY_TEST_MAIN_PASSWORD", test_base.TestBaseCase.passwd),
        )
        return dsn, user, password

    def _run_vector_cleanup_case(self, close_connection):
        dsn, user, password = self._connection_info()
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        code = textwrap.dedent(
            f"""
            import yaspy

            conn = yaspy.connect(dsn={dsn!r}, user={user!r}, password={password!r})
            cur = conn.cursor()

            cur.execute("select 1 from dual")
            assert cur.fetchone() == (1,)

            cur.execute("select to_vector('[1,2,3]') from dual")
            row = cur.fetchone()
            assert row is not None
            assert list(row[0]) == [1.0, 2.0, 3.0]

            cur.execute("select to_vector('[1,2,3]') from dual")
            row = cur.fetchone()
            assert row is not None
            assert list(row[0]) == [1.0, 2.0, 3.0]

            if {close_connection!r}:
                cur.close()
                conn.close()
            """
        )
        env = os.environ.copy()
        env["PYTHONPATH"] = (
            project_root
            if not env.get("PYTHONPATH")
            else project_root + os.pathsep + env["PYTHONPATH"]
        )
        proc = subprocess.run(
            [sys.executable, "-c", code],
            cwd=project_root,
            env=env,
            text=True,
            capture_output=True,
        )
        self.assertEqual(
            proc.returncode,
            0,
            "VECTOR cleanup subprocess failed\n"
            f"returncode={proc.returncode}\n"
            f"stdout={proc.stdout}\n"
            f"stderr={proc.stderr}",
        )

    def test_vector_fetch_survives_explicit_close(self):
        self._run_vector_cleanup_case(close_connection=True)

    def test_vector_fetch_survives_process_exit_cleanup(self):
        self._run_vector_cleanup_case(close_connection=False)


if __name__ == "__main__":
    test_base.run_test_cases()

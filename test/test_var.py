import os, sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import datetime
from decimal import Decimal
import yaspy

import test_base


def drop_table(cursor, name: str):
    if not name:
        return
    cursor.execute(f"drop table if exists {name}")


def read(cursor, name: str):
    cursor.execute(f"select * from {name}")
    return cursor.fetchall()

class TestCase(test_base.TestBaseCase):

    def test_num(self, name: str = "test_num"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(
            f"create table {name}(a int, b float, c double, d number, e tinyint)"
        )
        cursor.execute(
            f"insert into {name} values(?, ?, ?, ?, ?)",
            [20, 3.1, 4.14, Decimal("99.0123456789"), False],
        )
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result, [(20, 3.0999999046325684, 4.14, Decimal('99.0123456789'), 0)])


    def test_date(self, name: str = "test_date"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(a date, b timestamp)")
        d1 = datetime.datetime.now()
        d2 = datetime.datetime.now()
        cursor.execute(
            f"insert into {name} values(?, ?)",
            [d1, d2],
        )
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result , [(d1.date(), d2)])


    def test_timedelta(self, name: str = "test_timedelta"):
        cursor = self.connection.cursor()
        sql = "select EXTRACT(DAY FROM (sysdate -startup_time)) *60 * 60 * 24 +"
        sql += "EXTRACT(HOUR FROM(sysdate - startup_time)) * 60 * 60 +"
        sql += "EXTRACT(MINUTE FROM(sysdate - startup_time)) * 60 + "
        sql += "EXTRACT(SECOND FROM(sysdate - startup_time)) AS uptime from v$instance"

        cursor.execute(sql)
        for row in cursor:
            print(row)


    def test_lob(self, name: str = "test_lob"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(a int, b clob, c varchar(64), d blob )")
        cursor.execute(
            f"insert into {name} values(?, ?, ?, ?)", [1, "中文", "ab中文1", b"abc"]
        )

        clob = cursor.var(yaspy.CLOB, size=6)
        clob.setvalue("中文1")
        cursor.execute(
            f"insert into {name} values(?, ?, ?, ?)", [1, clob, "ab中文2", b"abc"]
        )
        clob.free()
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result, [(1, '中文', 'ab中文1', b'abc'), (1, '中文1', 'ab中文2', b'abc')])

        vc = cursor.var(yaspy.VARCHAR, size=7)
        vc.setvalue("ab中文1")
        cursor.execute(f"select * from {name} where c = ?", [vc])
        result = cursor.fetchall()
        self.assertEqual(result, [(1, '中文', 'ab中文1', b'abc')])


    def test_lastid(self, name: str = "test_lastid"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        try:
            cursor.execute(f"drop sequence seq_{name}")
        except:
            pass
        try:
            cursor.execute(f"create sequence seq_{name} start with 1 increment by 1")
        except:
            pass
        cursor.execute(
            f"create table {name} (id number default seq_{name}.nextval, name varchar(256))"
        )
        cursor.execute(f"insert into {name} (name) values(?)", ["abc"])
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result, [(Decimal('1'), 'abc')])

        lastid = cursor.var(yaspy.BIGINT)
        cursor.execute(
            f"insert into {name} (name) values(?) returning id into ?", ["hij", lastid]
        )
        self.assertEqual(lastid.getvalue(), 2)


    def test_value(self, name: str = "test_value"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(
            f"""
            create table {name}
            (
                Id int not null primary key,
                Boolean BOOLEAN,
                Byte smallint,
                SByte tinyint,
                Int16 smallint,
                UInt16 int,
                Int32 int,
                UInt32 bigint,
                Int64 bigint,
                UInt64 number,
                Single float,
                "Double" double,
                "Decimal" number,
                String varchar(8000),
                Guid char(36),
                "Date" date,
                "DateTime" timestamp,
                "Time" TIME
            )
        """
        )
        cursor.execute(
            f"insert into {name} values(0,  null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null)"
        )
        cursor.execute(
            f"insert into {name} values(1,  null, null, null, null, null, null, null, null, null, null, null, null, '', '', null, null, null)"
        )
        cursor.execute(
            f"insert into {name} values(2,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '0', '00000000-0000-0000-0000-000000000000', null, null, '00:00:00')"
        )
        cursor.execute(
            f"insert into {name} values(3,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, '1', '11111111-1111-1111-1111-111111111111', '1111-11-11', '1111-11-11 11:11:11.111', '11:11:11.111')"
        )
        cursor.execute(
            f"insert into {name} values(4,  false, 0, -128, -32768, 0, -2147483648, 0, -9223372036854775808, 0, -1.0000000000000000000000000000000000000018, -2.00000000000000000000000000000000000000000000000000000000000023, 0.000000000000001, null, '33221100-5544-7766-9988-aabbccddeeff', '1000-01-01', '1000-01-01 00:00:00', '00:00:00')"
        )
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(
            result,
            [
                (
                    0,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                ),
                (
                    1,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                ),
                (
                    2,
                    False,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    Decimal("0"),
                    0.0,
                    0.0,
                    Decimal("0"),
                    "0",
                    "00000000-0000-0000-0000-000000000000",
                    None,
                    None,
                    datetime.time(0, 0),
                ),
                (
                    3,
                    True,
                    1,
                    1,
                    1,
                    1,
                    1,
                    1,
                    1,
                    Decimal("1"),
                    1.0,
                    1.0,
                    Decimal("1"),
                    "1",
                    "11111111-1111-1111-1111-111111111111",
                    datetime.date(1111, 11, 11),
                    datetime.datetime(1111, 11, 11, 11, 11, 11, 111000),
                    datetime.time(11, 11, 11, 111000),
                ),
                (
                    4,
                    False,
                    0,
                    -128,
                    -32768,
                    0,
                    -2147483648,
                    0,
                    -9223372036854775808,
                    Decimal("0"),
                    -1.0,
                    -2.0,
                    Decimal("1E-15"),
                    None,
                    "33221100-5544-7766-9988-aabbccddeeff",
                    datetime.date(1000, 1, 1),
                    datetime.datetime(1000, 1, 1, 0, 0),
                    datetime.time(0, 0),
                ),
            ],
        )


    def test_param(self, name: str = "test_param"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name}(a int, b clob, c varchar(64), d varchar(32))")
        cursor.execute(
            f"insert into {name} (a,b,c,d) values(?, ?, ?, ?)",
            [1, "中文2", "中文3", "abc4"],
        )
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result, [(1, '中文2', '中文3', 'abc4')])


    def test_specical_char(self, name: str = "test_specical_char"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"SELECT '😀' from dual")
        result = cursor.fetchall()
        self.assertEqual(result, [('😀',)])


    def test_timespan(self, name: str = "test_timespan"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(
            f"create table {name} (ID number, dayspan  INTERVAL DAY(9) TO SECOND(6), yearspan INTERVAL YEAR(9) TO MONTH)"
        )
        cursor.execute(f"insert into {name} values(1, '-00 23:59:59', '-1-10')")
        cursor.execute(f"insert into {name} values(2, '09 23:59:59', '3-09')")
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(
            result,
            [
                (Decimal("1"), datetime.timedelta(seconds=86399), "-01-10"),
                (Decimal("2"), datetime.timedelta(days=9, seconds=86399), "+03-09"),
            ],
        )


    def test_null(self, name: str = "test_null"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(f"create table {name} (ID int, name varchar(32))")
        cursor.execute(f"insert into {name} values(?, ?)", [None, "aaa"])
        cursor.execute(f"insert into {name} values(?, ?)", [2, None])
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result, [(None, 'aaa'), (2, None)])


    def test_type(self, name: str = "test_type"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)

        # a01 = cursor.var(yaspy.NONE).setvalue(None)
        a02 = cursor.var(yaspy.BOOL).setvalue(True)
        a03 = cursor.var(yaspy.BYTE).setvalue(1)
        a04 = cursor.var(yaspy.SHORT).setvalue(2)
        a05 = cursor.var(yaspy.INTEGER).setvalue(3)
        a06 = cursor.var(yaspy.BIGINT).setvalue(4)
        a07 = cursor.var(yaspy.FLOAT).setvalue(5.5)
        a08 = cursor.var(yaspy.DOUBLE).setvalue(6.6)
        a09 = cursor.var(yaspy.NUMBER).setvalue(Decimal("7.0000000007"))
        a10 = cursor.var(yaspy.DATE).setvalue(datetime.date(year=2006, month=1, day=2))
        a11 = cursor.var(yaspy.TIME).setvalue(datetime.time(hour=15, minute=4, second=5))
        a12 = cursor.var(yaspy.DATETIME).setvalue(
            datetime.datetime(year=2006, month=1, day=2, hour=15, minute=4, second=5)
        )
        a13 = cursor.var(yaspy.YEARDELTA).setvalue("3-09")
        a14 = cursor.var(yaspy.TIMEDELTA).setvalue(
            datetime.timedelta(days=1, seconds=2, microseconds=300)
        )
        a15 = cursor.var(yaspy.CHAR).setvalue("char")
        a16 = cursor.var(yaspy.NCHAR).setvalue("nchar")
        a17 = cursor.var(yaspy.VARCHAR).setvalue("varchar")
        a18 = cursor.var(yaspy.NVARCHAR).setvalue("ncarchar")
        a19 = cursor.var(yaspy.BINARY).setvalue(b"binary")
        a20 = cursor.var(yaspy.CLOB).setvalue("clob")
        a21 = cursor.var(yaspy.BLOB).setvalue(b"blob")
        a22 = cursor.var(yaspy.BIT, size=2).setvalue(1)
        # a23 = cursor.var(yaspy.ROWID).setvalue()
        a24 = cursor.var(yaspy.NCLOB).setvalue("nclob")
        a25 = cursor.var(yaspy.JSON).setvalue(
            {"a": "a", "b": 2, "c": None, "d": False, "e": [1.1, True]}
        )

        b02 = cursor.var(yaspy.BOOL)
        b03 = cursor.var(yaspy.BYTE)
        b04 = cursor.var(yaspy.SHORT)
        b05 = cursor.var(yaspy.INTEGER)
        b06 = cursor.var(yaspy.BIGINT)
        b07 = cursor.var(yaspy.FLOAT)
        b08 = cursor.var(yaspy.DOUBLE)
        b09 = cursor.var(yaspy.NUMBER)
        b10 = cursor.var(yaspy.DATE)
        b11 = cursor.var(yaspy.TIME)
        b12 = cursor.var(yaspy.DATETIME)
        b13 = cursor.var(yaspy.YEARDELTA)
        b14 = cursor.var(yaspy.TIMEDELTA)
        b15 = cursor.var(yaspy.CHAR)
        b16 = cursor.var(yaspy.NCHAR)
        b17 = cursor.var(yaspy.VARCHAR)
        b18 = cursor.var(yaspy.NVARCHAR)
        b19 = cursor.var(yaspy.BINARY)
        b20 = cursor.var(yaspy.CLOB)
        b21 = cursor.var(yaspy.BLOB)
        b22 = cursor.var(yaspy.BIT)
        b23 = cursor.var(yaspy.ROWID)
        b24 = cursor.var(yaspy.NCLOB)
        b25 = cursor.var(yaspy.JSON)

        cursor.execute(
            f"""create table {name} (
            a02 BOOLEAN, a03 TINYINT, a04 SMALLINT, a05 INTEGER, a06 BIGINT, a07 FLOAT, a08 DOUBLE, a09 NUMBER,
            a10 DATE, a11 TIME, a12 TIMESTAMP, a13 INTERVAL YEAR(9) TO MONTH, a14 INTERVAL DAY(9) TO SECOND(6),
            a15 CHAR(64), a16 NCHAR(64), a17 VARCHAR(64), a18 NVARCHAR(64),
            a19 BINARY(64), a20 CLOB, a21 BLOB, a22 BIT(2), a24 NCLOB, a25 JSON    
        )"""
        )

        output_vars = [
            b02,
            b03,
            b04,
            b05,
            b06,
            b07,
            b08,
            b09,
            b10,
            b11,
            b12,
            b13,
            b14,
            b15,
            b16,
            b17,
            b18,
            b19,
            b20,
            b21,
            b22,
            b23,
            b24,
            b25,
        ]
        cursor.execute(
            f"""
            insert into {name} values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
            returning 
            a02, a03, a04, a05, a06, a07, a08,a09, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, rowid, a24, a25
            into
            ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?
            """,
            [
                a02,
                a03,
                a04,
                a05,
                a06,
                a07,
                a08,
                a09,
                a10,
                a11,
                a12,
                a13,
                a14,
                a15,
                a16,
                a17,
                a18,
                a19,
                a20,
                a21,
                a22,
                a24,
                a25,
                *output_vars,
            ],
        )

        a20.free()
        a21.free()
        a24.free()
        print("\noutput:")
        for v in output_vars:
            print("--> ", v.getvalue())
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(
            result,
            [
                (
                    True,
                    1,
                    2,
                    3,
                    4,
                    5.5,
                    6.6,
                    Decimal("7.0000000007"),
                    datetime.date(2006, 1, 2),
                    datetime.time(15, 4, 5),
                    datetime.datetime(2006, 1, 2, 15, 4, 5),
                    "+03-09",
                    datetime.timedelta(days=1, seconds=2, microseconds=300),
                    "char                                                            ",
                    "nchar                                                           ",
                    "varchar",
                    "ncarchar",
                    b"binary",
                    "clob",
                    b"blob",
                    "1",
                    "nclob",
                    {"a": "a", "b": 2, "c": None, "d": False, "e": [1.1, True]},
                )
            ],
        )


    def test_proc(self, name: str = "test_proc"):
        cursor = self.connection.cursor()
        drop_table(cursor, name)
        cursor.execute(
            f"create table {name} (ID number, NAME varchar2(10), SEX varchar2(4), AGE number, ADDRESS varchar2(200))"
        )
        cursor.execute(
            f"create or replace procedure proc1 is begin insert into {name}(ID, NAME, SEX, AGE) values (1, 'moses', 'man', 25); commit; end;"
        )
        cursor.callproc("proc1")

        cursor.execute(
            f"""create or replace procedure proc2(v_id number, v_name varchar2, v_sex varchar2, v_age number)
                is begin insert into {name}(id, name, sex, age) values(v_id, v_name, v_sex, v_age);
                commit; end;"""
        )
        cursor.callproc("proc2", [1, "aa", "man", 20])
        self.connection.commit()
        result = read(cursor, name)
        self.assertEqual(result, [(Decimal('1'), 'moses', 'man', Decimal('25'), None), (Decimal('1'), 'aa', 'man', Decimal('20'), None)])

        cursor.execute(
            f"create or replace procedure proc3 (recount out number) is begin select count(*) into recount from {name}; commit; end;"
        )
        recount = cursor.var(int)
        cursor.callproc("proc3", [recount])
        self.assertEqual(recount.getvalue(), 2)


if __name__ == "__main__":
    test_base.run_test_cases()

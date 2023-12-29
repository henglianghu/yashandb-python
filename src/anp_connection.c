#include "anp_connection.h"
#include "anp_exception.h"
#include "anp_cursor.h"
#include "structmember.h"

static PyObject *anpNewConnection(PyTypeObject *type, PyObject *args,
                                  PyObject *keywordArgs)
{
    return type->tp_alloc(type, 0);
}

static void anpFreeConnection(AnpConnection *conn)
{
    if (conn->hConn != NULL) {
        Py_BEGIN_ALLOW_THREADS
            yapiReleaseConn(conn->hConn);
        Py_END_ALLOW_THREADS
        conn->hConn = NULL;
    }
    Py_CLEAR(conn->username);
    Py_CLEAR(conn->dsn);
    Py_TYPE(conn)->tp_free((PyObject*) conn);
}

int anpGetModule(PyTypeObject *type, PyObject **module,
        PyObject **name)
{
    *module = PyObject_GetAttrString( (PyObject*) type, "__module__");
    if (*module == NULL){
        return -1;
    }
    *name = PyObject_GetAttrString( (PyObject*) type, "__name__");
    if (*name == NULL) {
        Py_DECREF(*module);
        return -1;
    }
    return 0;
}

static PyObject* anpReprConnection(AnpConnection* connection)
{
    PyObject* module;
    PyObject* name;

    if (anpGetModule(Py_TYPE(connection), &module, &name) < 0) {
        return NULL;
    }
    
    PyObject* format = PyUnicode_FromString("%s.%s to %s@%s");
    PyObject* result = PyUnicode_Format(format, PyTuple_Pack(4, module, name, connection->username, connection->dsn));

    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}

static int anpConnectionInit(AnpConnection *conn, PyObject *args,
                             PyObject *keywordArgs)
{
    const char * dsn, *user, *password;
    static char* kwlist[] = {"dsn", "user", "password", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "sss", kwlist, &dsn, &user, &password)) {
        return -1;
    }

    if (yapiAllocEnv(&anpEnv) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }
    
    conn->username = PyUnicode_FromString(user);
    conn->dsn = PyUnicode_FromString(dsn);

    YapiResult res;
    Py_BEGIN_ALLOW_THREADS
        res = yapiConnect(anpEnv, dsn, (int16_t)strlen(dsn), user, (int16_t)strlen(user), password, (int16_t)strlen(password), &conn->hConn);
    Py_END_ALLOW_THREADS

    if (res != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    uint32_t charset = YAPI_CHARSET_UTF8;
    if (yapiSetEnvAttr(anpEnv, YAPI_ATTR_CHARSET_CODE, &charset, sizeof(uint32_t)) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    return 0;
}

static PyObject *anpConnectionClose(AnpConnection *conn, PyObject *args)
{
    if (!anpConnectionIsConnected(conn)) {
        return NULL;
    }

    if (conn->hConn != NULL) {
        Py_BEGIN_ALLOW_THREADS
            yapiDisconnect(conn->hConn);
        Py_END_ALLOW_THREADS
        conn->hConn = NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *anpConnectionCommit(AnpConnection *conn, PyObject *args)
{
    YapiResult ret;
    if (!anpConnectionIsConnected(conn)) {
        return anpRaiseAndReturnNullException();
    }
    if (conn->autocommit) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "Cannot commit when autocommit is enabled.");
    }
    
    Py_BEGIN_ALLOW_THREADS
    ret = yapiCommit(conn->hConn);
    Py_END_ALLOW_THREADS
    if (ret != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    Py_RETURN_NONE;
}

static PyObject *anpConnectionRollback(AnpConnection *conn, PyObject *args)
{
    YapiResult ret;
    if (!anpConnectionIsConnected(conn)) {
        return anpRaiseAndReturnNullException();
    }
    if (conn->autocommit) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "Cannot rollback when autocommit is enabled.");
    }
    Py_BEGIN_ALLOW_THREADS
    ret = yapiRollback(conn->hConn);
    Py_END_ALLOW_THREADS
    if (ret != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    Py_RETURN_NONE;
}

static PyObject *anpConnectionNewCursor(AnpConnection *conn, PyObject *args,
                                        PyObject *keywordArgs)
{
    PyObject *cursorArgs, *callResult, *tempArg;
    Py_ssize_t argsCount = 0, i;

    if (!anpConnectionIsConnected(conn)) {
        return NULL;
    }

    if (args) {
        argsCount = PyTuple_GET_SIZE(args);
    }
    cursorArgs = PyTuple_New(1 + argsCount);
    if (!cursorArgs) {
        return NULL;
    }
    
    Py_INCREF(conn);
    PyTuple_SET_ITEM(cursorArgs, 0, (PyObject*) conn);
    for (i = 0; i < argsCount; i++) {
        tempArg = PyTuple_GET_ITEM(args, i);
        Py_INCREF(tempArg);
        PyTuple_SET_ITEM(cursorArgs, i + 1, tempArg);
    }
    callResult = PyObject_Call( (PyObject*) &anchorPyTypeCursor, cursorArgs,
                            keywordArgs);
    Py_DECREF(cursorArgs);
    return callResult;
}

bool anpConnectionIsConnected(AnpConnection *conn)
{
    if (conn->hConn == NULL) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not connected");
        return YAPI_FALSE;
    }
    return YAPI_TRUE;
}

static PyObject *anpGetAutoCommit(AnpConnection *conn, void *unused)
{
    int32_t len;
    if (yapiGetConnAttr(conn->hConn, YAPI_ATTR_AUTOCOMMIT, &conn->autocommit, sizeof(conn->autocommit), &len) !=
        YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }

    if (conn->autocommit){
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


static int anpSetAutoCommit(AnpConnection *conn, PyObject *value, void *closure)
{
    if (!PyBool_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "The autocommit attribute value must be a bool");
        return -1;
    }
    conn->autocommit =  (value == Py_True);
    Py_INCREF(value);
    int32_t aCommit = conn->autocommit;
    if (yapiSetConnAttr(conn->hConn, YAPI_ATTR_AUTOCOMMIT, &aCommit, sizeof(aCommit)) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    return 0;
}

static PyObject *yaspyConnection_contextManagerEnter(AnpConnection *conn, PyObject* args)
{
    if (!anpConnectionIsConnected(conn)) {
        return NULL;
    }

    Py_INCREF(conn);
    return (PyObject*) conn;
}

static PyObject *yaspyConnection_contextManagerExit(AnpConnection *conn, PyObject* args)
{
    PyObject *excType, *excValue, *excTraceback, *result;

    if (!PyArg_ParseTuple(args, "OOO", &excType, &excValue, &excTraceback)) {
        return NULL;
    }

    result = anpConnectionClose(conn, NULL);
    if (result == NULL) {
        return NULL;
    }

    Py_DECREF(result);
    Py_INCREF(Py_False);
    return Py_False;
}

static PyMethodDef anpMethods[] = {
        { "close",    (PyCFunction) anpConnectionClose,     METH_NOARGS },
        { "commit",   (PyCFunction) anpConnectionCommit,    METH_NOARGS },
        { "rollback", (PyCFunction) anpConnectionRollback,  METH_NOARGS },
        { "cursor",   (PyCFunction) anpConnectionNewCursor, METH_VARARGS | METH_KEYWORDS },
        { "__enter__", (PyCFunction) yaspyConnection_contextManagerEnter,  METH_NOARGS },
        { "__exit__", (PyCFunction) yaspyConnection_contextManagerExit,    METH_VARARGS },
        { NULL }
};

static PyMemberDef anpMembers[] = {
        { "username",         T_OBJECT, offsetof(AnpConnection, username), READONLY },
        { "dsn",              T_OBJECT, offsetof(AnpConnection, dsn),      READONLY },
        { NULL }
};

static PyGetSetDef anpCalcMembers[] = {
    {"autocommit", (getter) anpGetAutoCommit, (setter)anpSetAutoCommit, 0, 0},
    {NULL}
};

PyTypeObject anchorPyTypeConnection = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "yaspy.Connection",
        .tp_basicsize = sizeof(AnpConnection),
        .tp_dealloc = (destructor) anpFreeConnection,
        .tp_repr = (reprfunc) anpReprConnection,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_methods = anpMethods,
        .tp_members = anpMembers,
        .tp_getset = anpCalcMembers,
        .tp_init = (initproc) anpConnectionInit,
        .tp_new = (newfunc) anpNewConnection,
};

YapiResult anpRegistConnection(PyObject* module)
{
    PyType_Ready(&anchorPyTypeConnection);

    Py_INCREF(&anchorPyTypeConnection);
    if (PyModule_AddObject(module, "Connection", (PyObject*) &anchorPyTypeConnection) < 0) {
        return YAPI_ERROR;
    }


    Py_INCREF(&anchorPyTypeConnection);
    if (PyModule_AddObject(module, "connect", (PyObject*) &anchorPyTypeConnection) < 0) {
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

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
    if (conn->hEnv != NULL) {
        Py_BEGIN_ALLOW_THREADS
            yacFreeHandle(YAC_HANDLE_STMT, conn->hStmt);
            yacFreeHandle(YAC_HANDLE_DBC, conn->hConn);
            yacFreeHandle(YAC_HANDLE_ENV, conn->hEnv);
        Py_END_ALLOW_THREADS
        conn->hEnv = NULL;
        conn->hConn = NULL;
        conn->hStmt = NULL;
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

    if (yacAllocHandle(YAC_HANDLE_ENV, NULL, &conn->hEnv) != YAC_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }
    if (yacAllocHandle(YAC_HANDLE_DBC, conn->hEnv, &conn->hConn) != YAC_SUCCESS) {
        yacFreeHandle(YAC_HANDLE_ENV, conn->hEnv);
        conn->hEnv = NULL;
        return anpRaiseAndReturnIntException();
    }

    conn->username = PyUnicode_FromString(user);
    conn->dsn = PyUnicode_FromString(dsn);

    YacResult res;
    Py_BEGIN_ALLOW_THREADS
        res = yacConnect(conn->hConn, dsn, strlen(dsn), user, strlen(user), password, strlen(password));
    Py_END_ALLOW_THREADS

    if (res != YAC_SUCCESS)
    {
        yacFreeHandle(YAC_HANDLE_DBC, conn->hConn);
        conn->hConn = NULL;
        yacFreeHandle(YAC_HANDLE_ENV, conn->hEnv);
        conn->hEnv = NULL;
        return anpRaiseAndReturnIntException();
    }

    if (yacAllocHandle(YAC_HANDLE_STMT, conn->hConn, &conn->hStmt) != YAC_SUCCESS) {
        yacFreeHandle(YAC_HANDLE_DBC, conn->hConn);
        conn->hConn = NULL;
        yacFreeHandle(YAC_HANDLE_ENV, conn->hEnv);
        conn->hEnv = NULL;
        return anpRaiseAndReturnIntException();
    }

    return 0;
}

static PyObject *anpConnectionClose(AnpConnection *conn, PyObject *args)
{
    if (!anpConnectionIsConnected(conn)) {
        return anpRaiseAndReturnNullException();
    }
    if (conn->hStmt != NULL) {
        yacFreeHandle(YAC_HANDLE_STMT, conn->hStmt);
        conn->hStmt = NULL;
    }
    if (conn->hConn != NULL) {
        Py_BEGIN_ALLOW_THREADS
            yacDisconnect(conn->hConn);
        Py_END_ALLOW_THREADS
        yacFreeHandle(YAC_HANDLE_DBC, conn->hConn);
        conn->hConn = NULL;
    }
    if (conn->hEnv != NULL) {
        yacFreeHandle(YAC_HANDLE_ENV, conn->hEnv);
        conn->hEnv = NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *anpConnectionCommit(AnpConnection *conn, PyObject *args)
{
    YacResult ret;
    if (!anpConnectionIsConnected(conn)) {
        return anpRaiseAndReturnNullException();
    }
    if (conn->autocommit) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "Cannot commit when autocommit is enabled.");
    }
    
    Py_BEGIN_ALLOW_THREADS
    ret = yacCommit(conn->hConn);
    Py_END_ALLOW_THREADS
    if (ret != YAC_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    Py_RETURN_NONE;
}

static PyObject *anpConnectionRollback(AnpConnection *conn, PyObject *args)
{
    YacResult ret;
    if (!anpConnectionIsConnected(conn)) {
        return anpRaiseAndReturnNullException();
    }
    if (conn->autocommit) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "Cannot rollback when autocommit is enabled.");
    }
    Py_BEGIN_ALLOW_THREADS
    ret = yacRollback(conn->hConn);
    Py_END_ALLOW_THREADS
    if (ret != YAC_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    Py_RETURN_NONE;
}

static PyObject *anpConnectionNewCursor(AnpConnection *conn, PyObject *args,
                                        PyObject *keywordArgs)
{
    PyObject *createArgs, *result, *arg;
    Py_ssize_t numArgs = 0, i;

    if (!anpConnectionIsConnected(conn)) {
        return NULL;
    }
    if (args) {
        numArgs = PyTuple_GET_SIZE(args);
    }
    createArgs = PyTuple_New(1 + numArgs);
    if (!createArgs) {
        return NULL;
    }
    Py_INCREF(conn);
    PyTuple_SET_ITEM(createArgs, 0, (PyObject*) conn);
    for (i = 0; i < numArgs; i++) {
        arg = PyTuple_GET_ITEM(args, i);
        Py_INCREF(arg);
        PyTuple_SET_ITEM(createArgs, i + 1, arg);
    }
    result = PyObject_Call( (PyObject*) &anchorPyTypeCursor, createArgs,
                            keywordArgs);
    Py_DECREF(createArgs);
    return result;
}

YacBool anpConnectionIsConnected(AnpConnection *conn)
{
    if (conn->hConn == NULL) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not connected");
        return YAC_FALSE;
    }
    return YAC_TRUE;
}

static PyObject *anpGetAutoCommit(AnpConnection *conn, void *unused)
{
    YacInt32 len;
    if(yacGetConnAttr(conn->hConn, YAC_ATTR_AUTOCOMMIT, &conn->autocommit, sizeof(conn->autocommit), &len) != YAC_SUCCESS) {
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
    YacInt32 aCommit = conn->autocommit;
    if (yacSetConnAttr(conn->hConn, YAC_ATTR_AUTOCOMMIT, &aCommit, sizeof(aCommit)) != YAC_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    return 0;
}

static PyMethodDef anpMethods[] = {
        { "close",    (PyCFunction) anpConnectionClose,     METH_NOARGS },
        { "commit",   (PyCFunction) anpConnectionCommit,    METH_NOARGS },
        { "rollback", (PyCFunction) anpConnectionRollback,  METH_NOARGS },
        { "cursor",   (PyCFunction) anpConnectionNewCursor, METH_VARARGS | METH_KEYWORDS },
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

YacResult anpRegistConnection(PyObject* module)
{
    PyType_Ready(&anchorPyTypeConnection);

    Py_INCREF(&anchorPyTypeConnection);
    if (PyModule_AddObject(module, "Connection", (PyObject*) &anchorPyTypeConnection) < 0) {
        return YAC_ERROR;
    }


    Py_INCREF(&anchorPyTypeConnection);
    if (PyModule_AddObject(module, "connect", (PyObject*) &anchorPyTypeConnection) < 0) {
        return YAC_ERROR;
    }
    return YAC_SUCCESS;
}

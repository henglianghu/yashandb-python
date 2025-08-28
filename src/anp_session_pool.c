#include "anp_session_pool.h"
#include "anp_exception.h"
#include "anp_connection.h"

static void anpSessionPoolFree(AnpSessionPool* pool)
{
    if (pool->hConnPool != NULL) {
        Py_BEGIN_ALLOW_THREADS
        yapiConnectionPoolDestroy(pool->hConnPool, pool->mode);
        yapiReleaseConnectionPool(pool->hConnPool);
        Py_END_ALLOW_THREADS
        pool->hConnPool = NULL;
    }
    Py_CLEAR(pool->username);
    Py_CLEAR(pool->password);
    Py_CLEAR(pool->dsn);
    Py_TYPE(pool)->tp_free((PyObject*)pool);
}

static PyObject* anpSessionPoolRepr(AnpSessionPool* pool)
{
    PyObject* module;
    PyObject* name;

    if (anpGetModule(Py_TYPE(pool), &module, &name) < 0) {
        return NULL;
    }
    
    PyObject* format = PyUnicode_FromString("%s.%s to %s@%s");
    PyObject* result = PyUnicode_Format(format, PyTuple_Pack(4, module, name, pool->username, pool->dsn));

    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}

static int anpSessionPoolInit(AnpSessionPool* pool, PyObject* args, PyObject* keywordArgs)
{
    const char * dsn, *user, *password;
    static char* kwlist[] = {"user", "password", "dsn", "min", "max", "increment", "getmode", NULL};
    uint32_t min = 1, max = 4, increment = 1, mode = 1, get_mode = 1;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "sss|iiii", kwlist, &user, &password, &dsn, &min, &max,
                                     &increment, &get_mode)) {
        return -1;
    }

    if (get_mode != 1) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "get mode must be 1");
        return -1;
    }

    if (yapiAllocEnv(&anpEnv) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }
    if (yapiAllocConnectionPool(anpEnv, &pool->hConnPool) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    pool->username = PyUnicode_FromString(user);
    pool->password = PyUnicode_FromString(password);
    pool->dsn = PyUnicode_FromString(dsn);
    pool->minSessions = min;
    pool->maxSessions = max;
    pool->sessionIncrement = increment;
    pool->mode = mode;

    YapiResult res;
    Py_BEGIN_ALLOW_THREADS
    res = yapiConnectionPoolCreate(pool->hConnPool, dsn, (int16_t)strlen(dsn), (uint32_t)pool->minSessions,
                                   (uint32_t)pool->maxSessions, (uint32_t)pool->sessionIncrement, user,
                                   (int16_t)strlen(user), password, (int16_t)strlen(password), (uint32_t)pool->mode);
    Py_END_ALLOW_THREADS

    if (res != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }
    return 0;
}

static PyObject* anpSessionPoolNew(PyTypeObject* type, PyObject* args, PyObject* keywordArgs)
{
    return type->tp_alloc(type, 0);
}

bool anpSessionPoolIsCreated(AnpSessionPool* pool)
{
    if (pool->hConnPool == NULL) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not created");
        return YAPI_FALSE;
    }
    return YAPI_TRUE;
}

static PyObject *anpConnectionGet(AnpSessionPool *pool, PyObject *args)
{
    if (!anpSessionPoolIsCreated(pool)) {
        return NULL;
    }

    AnpConnection* conn = (AnpConnection*)anpNewConnection(&anchorPyTypeConnection, NULL, NULL);

    YapiResult res;
    Py_BEGIN_ALLOW_THREADS
    res = yapiConnectionGet(pool->hConnPool, &conn->hConn);
    Py_END_ALLOW_THREADS

    if (res != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    conn->hConnPool = pool->hConnPool;
    return (PyObject*)conn;
}

static PyObject *anpConnectionRelease(AnpSessionPool *pool, PyObject* args, PyObject* keywordArgs)
{
    if (!anpSessionPoolIsCreated(pool)) {
        return NULL;
    }

    static char *keywordList[] = { "connection", NULL };
    AnpConnection* conn = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O!", keywordList, &anchorPyTypeConnection, &conn)) {
        return NULL;
    }
    if (!anpConnectionIsConnected(conn)) {
        return NULL;
    }

    YapiResult res;
    Py_BEGIN_ALLOW_THREADS
    res = yapiConnectionGiveBack(conn->hConn);
    Py_END_ALLOW_THREADS

    if (res != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    conn->hConn = NULL;
    Py_RETURN_NONE;
}

static PyObject* anpSessionPoolClose(AnpSessionPool* pool)
{
    if (!anpSessionPoolIsCreated(pool)) {
        return NULL;
    }

    if (pool->hConnPool != NULL) {
        YapiResult res;
        Py_BEGIN_ALLOW_THREADS
        res = yapiConnectionPoolDestroy(pool->hConnPool, pool->mode);
        Py_END_ALLOW_THREADS
        if (res != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
    }
    Py_RETURN_NONE;
}

static PyMethodDef anpSessionPoolMethods[] = {
        { "close",    (PyCFunction) anpSessionPoolClose, METH_NOARGS },
        { "release",   (PyCFunction) anpConnectionRelease, METH_VARARGS | METH_KEYWORDS },
        { "acquire", (PyCFunction) anpConnectionGet, METH_NOARGS },
        { NULL }
};

static PyMemberDef anpSessionPoolMembers[] = {
        { "username", T_OBJECT, offsetof(AnpSessionPool, username), READONLY },
        { "password", T_OBJECT, offsetof(AnpSessionPool, password), READONLY },
        { "dsn", T_OBJECT, offsetof(AnpSessionPool, dsn), READONLY },
        { "min", T_INT, offsetof(AnpSessionPool, minSessions), READONLY },
        { "max", T_INT, offsetof(AnpSessionPool, maxSessions), READONLY },
        { "increment", T_INT, offsetof(AnpSessionPool, sessionIncrement), READONLY },
        { NULL }
};

static PyGetSetDef anpSessionPoolCalcMembers[] = {
    {NULL}
};

PyTypeObject anchorPyTypeSessionPool = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "yaspy.SessionPool",
        .tp_basicsize = sizeof(AnpSessionPool),
        .tp_dealloc = (destructor) anpSessionPoolFree,
        .tp_repr = (reprfunc) anpSessionPoolRepr,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_methods = anpSessionPoolMethods,
        .tp_members = anpSessionPoolMembers,
        .tp_getset = anpSessionPoolCalcMembers,
        .tp_init = (initproc) anpSessionPoolInit,
        .tp_new = (newfunc) anpSessionPoolNew,
};

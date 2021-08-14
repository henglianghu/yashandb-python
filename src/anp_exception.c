#include "anp_exception.h"
#include "structmember.h"

PyObject *anpWarningException = NULL;
PyObject *anpErrorException = NULL;
PyObject *anpInterfaceErrorException = NULL;
PyObject *anpDatabaseErrorException = NULL;
PyObject *anpDataErrorException = NULL;
PyObject *anpOperationalErrorException = NULL;
PyObject *anpIntegrityErrorException = NULL;
PyObject *anpInternalErrorException = NULL;
PyObject *anpProgrammingErrorException = NULL;
PyObject *anpNotSupportedException = NULL;

static void anpErrorFree(AnpError *error)
{
    Py_CLEAR(error->message);
    PyObject_Del(error);
}

static PyObject *anpErrorNew(PyTypeObject *type, PyObject *args,
                             PyObject *keywordArgs)
{
    PyObject *message;
    AncUint32 code;
    AnpError *error;
    AncUint32 line;
    AncUint32 column;

    if (!PyArg_ParseTuple(args, "OIII", &message, &code, &line, &column)) {
        return NULL;
    }
    error = (AnpError*) type->tp_alloc(type, 0);
    if (!error)
        return NULL;

    error->code = code;
    error->line = line;
    error->column = column;
    Py_INCREF(message);
    error->message = message;

    return (PyObject*) error;
}

static int anpModuleSetException(PyObject *module, PyObject **exception,
                                 char *name, PyObject *baseException)
{
    char buffer[100];

    sprintf(buffer, "yaspy.%s", name);
    *exception = PyErr_NewException(buffer, baseException, NULL);
    if (*exception == NULL) {
        return -1;
    }
    return PyModule_AddObject(module, name, *exception);
}

static PyObject *anpErrorReduce(AnpError *error)
{
    return Py_BuildValue("(O(OIII))", Py_TYPE(error), error->message,
                         error->code, error->line, error->column);
}

static PyObject *anpErrorStr(AnpError *error)
{
    Py_INCREF(error->message);
    return error->message;
}

static PyMethodDef anpErrorMethods[] = {
        { "__reduce__", (PyCFunction) anpErrorReduce, METH_NOARGS },
        { NULL, NULL }
};

static PyMemberDef anpErrorMembers[] = {
        { "code",     T_UINT,   offsetof(AnpError, code),    READONLY },
        { "line",     T_UINT,   offsetof(AnpError, line),    READONLY },
        { "column",   T_UINT,   offsetof(AnpError, column),  READONLY },
        { "message",  T_OBJECT, offsetof(AnpError, message), READONLY },
        { NULL }
};

PyTypeObject anpPyTypeError = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "yaspy._Error",
        .tp_basicsize = sizeof(AnpError),
        .tp_dealloc = (destructor) anpErrorFree,
        .tp_str = (reprfunc) anpErrorStr,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_methods = anpErrorMethods,
        .tp_members = anpErrorMembers,
        .tp_new = anpErrorNew
};

AncResult anpRegisterException(PyObject *module)
{
    PyType_Ready(&anpPyTypeError);

    // create exception object and add it to the dictionary
    if (anpModuleSetException(module, &anpErrorException,
                              "Error", NULL) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpWarningException,
                              "Warning", NULL) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpInterfaceErrorException,
                              "InterfaceError", anpErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpDatabaseErrorException,
                              "DatabaseError", anpErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpInternalErrorException,
                              "InternalError", anpDatabaseErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpOperationalErrorException,
                              "OperationalError", anpDatabaseErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpProgrammingErrorException,
                              "ProgrammingError", anpDatabaseErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpIntegrityErrorException,
                              "IntegrityError", anpDatabaseErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpDataErrorException,
                              "DataError", anpDatabaseErrorException) < 0) {
        return ANC_ERROR;
    }
    if (anpModuleSetException(module, &anpNotSupportedException,
                              "NotSupportedError", anpDatabaseErrorException) < 0) {
        return ANC_ERROR;
    }

    Py_INCREF(&anpPyTypeError);
    if (PyModule_AddObject(module, "_Error", (PyObject*) &anpPyTypeError) < 0) {
        return ANC_ERROR;
    }

    return ANC_SUCCESS;
}

int anpRaiseExceptionFromInfo(AncUint32 code, const char * message, const char* sqlStat, AncTextPos *pos)
{
    PyObject *exceptionType;
    AnpError *error;

    error = anpExceptionNewFromInfo(code, message, sqlStat, pos);
    if (error == NULL) {
        return -1;
    }
    exceptionType = anpDatabaseErrorException;
    PyErr_SetObject(exceptionType, (PyObject*) error);
    Py_DECREF(error);
    return -1;
}

int anpRaiseAndReturnIntException(void)
{
    AncInt32   code;
    AncTextPos pos;
    AncChar*   message;
    AncChar*   sqlStat;
    ancGetLastError(&code, &message, &sqlStat, &pos);
    return anpRaiseExceptionFromInfo(code, message, sqlStat, &pos);
}

static AnpError *anpErrorNewFromString(const char *message)
{
    AnpError* error;

    error = (AnpError*)anpPyTypeError.tp_alloc(&anpPyTypeError, 0);
    if (!error) {
        return NULL;
    }
    Py_INCREF(Py_None);
    error->message = PyUnicode_DecodeASCII(message, strlen(message), NULL);
    if (!error->message) {
        Py_DECREF(error);
        return NULL;
    }

    return error;
}

PyObject *anpRaiseExceptionFromString(PyObject *exceptionType,
                                  const char *message)
{
    AnpError *error;

    error = anpErrorNewFromString(message);
    if (error == NULL) {
        return NULL;
    }
    PyErr_SetObject(exceptionType, (PyObject*) error);
    Py_DECREF(error);
    return NULL;
}

PyObject *anpRaiseAndReturnNullException(void)
{
    anpRaiseAndReturnIntException();
    return NULL;
}

AnpError *anpExceptionNewFromInfo(AncUint32 code, const char * message, const char* sqlStat, AncTextPos *pos)
{
    AnpError* error;

    // create error object and initialize it
    error = (AnpError*)anpPyTypeError.tp_alloc(&anpPyTypeError, 0);
    if (!error) {
        return NULL;
    }
    error->code = code;
    error->line = pos->line;
    error->column = pos->column;

    // create message
    error->message = PyUnicode_Decode(message, strlen(message), NULL, NULL);
    if (!error->message) {
        Py_DECREF(error);
        return NULL;
    }

    error->sqlStat = PyUnicode_Decode(sqlStat, strlen(sqlStat), NULL, NULL);
    if (!error->sqlStat) {
        Py_DECREF(error->sqlStat);
        return NULL;
    }

    return error;
}



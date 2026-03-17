#include <Python.h>
#include <datetime.h>
#include "anp_module.h"
#include "anp_exception.h"
#include "anp_connection.h"
#include "anp_cursor.h"
#include "anp_var.h"
#include "anp_session_pool.h"
#include "anp_api_type.h"
#include "anp_fetchinfo.h"

YapiEnv* anpEnv = NULL;


#define YASPY_MAKE_TYPE_READY(type) \
    if (PyType_Ready(type) < 0) \
        return NULL;


#define YASPY_ADD_TYPE_OBJECT(name, type) \
    Py_INCREF(type); \
    if (PyModule_AddObject(module, name, (PyObject*) type) < 0) \
        return NULL;


int yaspyModuleAddApiType(PyObject *module, const char *name, YapiType defaultDbType, yaspyApiType **apiType)
{
    yaspyApiType *tempApiType;

    tempApiType = (yaspyApiType*) yasPyTypeApiType.tp_alloc(&yasPyTypeApiType, 0);
    if (tempApiType == NULL) {
        return -1;
    }

    tempApiType->name = name;
    tempApiType->defaultDbType = defaultDbType;
    tempApiType->dbTypes = PyList_New(0);
    if (tempApiType->dbTypes == NULL) {
        Py_DECREF(tempApiType);
        return -1;
    }

    if (PyModule_AddObject(module, name, (PyObject*) tempApiType) < 0) {
        Py_DECREF(tempApiType);
        return -1;
    }
    *apiType = tempApiType;

    return 0;
}

static PyObject* anpModuleTimeFromTicks(PyObject* module, PyObject* args)
{
    PyObject *dateTime = PyDateTime_FromTimestamp(args);
    int hour = PyDateTime_DATE_GET_HOUR(dateTime);
    int minute = PyDateTime_DATE_GET_MINUTE(dateTime);
    int second = PyDateTime_DATE_GET_SECOND(dateTime);
    int usecond = PyDateTime_DATE_GET_MICROSECOND(dateTime);
    Py_XDECREF(dateTime);
    return PyTime_FromTime(hour, minute, second, usecond);
}

static PyObject* anpModuleDateFromTicks(PyObject* module, PyObject* args)
{
    return PyDate_FromTimestamp(args);
}

static PyObject* anpModuleTimestampFromTicks(PyObject* module, PyObject* args)
{
    return PyDateTime_FromTimestamp(args);
}

static PyMethodDef AnchorMethods[] = {
    { "DateFromTicks", (PyCFunction) anpModuleDateFromTicks, METH_VARARGS },
    { "TimeFromTicks", (PyCFunction) anpModuleTimeFromTicks, METH_VARARGS },
    { "TimestampFromTicks", (PyCFunction) anpModuleTimestampFromTicks, METH_VARARGS },
    { NULL }
};

static struct PyModuleDef yaspy_module = {
        PyModuleDef_HEAD_INIT,
        "yaspy",
        NULL, /* module documentation, may be NULL */
        -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
        AnchorMethods,                      // methods
        NULL,                                  // m_reload
        NULL,                                  // traverse
        NULL,                                  // clear
        NULL                                   // free
};

PyMODINIT_FUNC
PyInit_yaspy(void)
{
    PyObject *module;
    PyDateTime_IMPORT;
    
    if (anpInitDecimal() != YAPI_SUCCESS){
        return NULL;
    }

    if (anpInitJson() != YAPI_SUCCESS) {
        return NULL;
    }

    if (anpInitArray() != YAPI_SUCCESS) {
        return NULL;
    }

    YASPY_MAKE_TYPE_READY(&yasPyTypeApiType);
    YASPY_MAKE_TYPE_READY(&anchorPyTypeSessionPool);

    module = PyModule_Create(&yaspy_module);

    YASPY_ADD_TYPE_OBJECT("Date", anpPyTypeDate);
    YASPY_ADD_TYPE_OBJECT("Timestamp", anpPyTypeDateTime);
    YASPY_ADD_TYPE_OBJECT("Time", anpPyTypeTime);
    YASPY_ADD_TYPE_OBJECT("Timedelta", anpPyTypeTimeDelta);

    YASPY_ADD_TYPE_OBJECT("ApiType", &yasPyTypeApiType);
    YASPY_ADD_TYPE_OBJECT("SessionPool", &anchorPyTypeSessionPool);

    if (anpRegisterApiType(module) != YAPI_SUCCESS) {
        return NULL;
    }

    // create constants required by Python DB API 2.0
    if (PyModule_AddStringConstant(module, "apilevel", "2.0") < 0) {
        return NULL;
    }
    if (PyModule_AddIntConstant(module, "threadsafety", 2) < 0) {
        return NULL;
    }
    if (PyModule_AddStringConstant(module, "paramstyle", "named") < 0) {
        return NULL;
    }

    if (anpRegistConnection(module) != YAPI_SUCCESS) {
        return NULL;
    }
    if (anpRegistCursor(module) != YAPI_SUCCESS) {
        return NULL;
    }
    if (anpRegisterException(module) != YAPI_SUCCESS) {
        return NULL;
    }
    if (anpRegisteVarObject(module)!= YAPI_SUCCESS) {
        return NULL;
    }
    if (anpRegisterFetchInfo(module) != YAPI_SUCCESS) {
        return NULL;
    }

    return module;
}

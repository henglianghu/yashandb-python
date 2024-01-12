#include "anp_module.h"
#include <Python.h>
#include "anp_exception.h"
#include "anp_connection.h"
#include "anp_cursor.h"
#include "anp_var.h"

extern PyTypeObject *anpPyTypeDate;
extern PyTypeObject *anpPyTypeDateTime;

YapiEnv* anpEnv = NULL;

yaspyApiType *yaspyApiTypeInteger = NULL;


#define YASPY_MAKE_TYPE_READY(type) \
    if (PyType_Ready(type) < 0) \
        return NULL;


#define YASPY_ADD_TYPE_OBJECT(name, type) \
    Py_INCREF(type); \
    if (PyModule_AddObject(module, name, (PyObject*) type) < 0) \
        return NULL;


static int yaspyModuleAddApiType(PyObject *module, const char *name, YapiType defaultDbType, yaspyApiType **apiType)
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



static PyMethodDef AnchorMethods[] = {
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
    
    if (anpInitDecimal() != YAPI_SUCCESS){
        return NULL;
    }

    YASPY_MAKE_TYPE_READY(&yasPyTypeApiType);

    module = PyModule_Create(&yaspy_module);

    YASPY_ADD_TYPE_OBJECT("Date", anpPyTypeDate);
    YASPY_ADD_TYPE_OBJECT("Timestamp", anpPyTypeDateTime);

    YASPY_ADD_TYPE_OBJECT("ApiType", &yasPyTypeApiType);

    if (yaspyModuleAddApiType(module, "INTEGER", YAPI_TYPE_INTEGER, &yaspyApiTypeInteger) < 0) {
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

    return module;
}

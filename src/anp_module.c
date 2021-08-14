#include "anp_module.h"
#include <Python.h>
#include "datetime.h"
#include "anp_exception.h"
#include "anp_connection.h"
#include "anp_cursor.h"
#include "anp_var.h"

PyTypeObject *anpPyTypeDate;
PyTypeObject *anpPyTypeDateTime;

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
    
    PyDateTime_IMPORT;
    anpPyTypeDate = PyDateTimeAPI->DateType;
    anpPyTypeDateTime = PyDateTimeAPI->DateTimeType;
    if (anpInitDecimal() != ANC_SUCCESS){
        return NULL;
    }

    module = PyModule_Create(&yaspy_module);
    Py_INCREF(anpPyTypeDate);
    if (PyModule_AddObject(module, "Date", (PyObject*) anpPyTypeDate) < 0) {
        return NULL;
    }
    Py_INCREF(anpPyTypeDateTime);
    if (PyModule_AddObject(module, "Timestamp", (PyObject*) anpPyTypeDateTime) < 0) {
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

    if (anpRegistConnection(module) != ANC_SUCCESS) {
        return NULL;
    }
    if (anpRegistCursor(module) != ANC_SUCCESS) {
        return NULL;
    }
    if (anpRegisterException(module) != ANC_SUCCESS) {
        return NULL;
    }
    if (anpRegisteVarObject(module)!= ANC_SUCCESS) {
        return NULL;
    }

    return module;
}

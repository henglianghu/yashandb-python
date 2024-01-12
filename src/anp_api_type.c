#include "anp_api_type.h"


static void yaspyApiTypeFree(yaspyApiType *apiType)
{
    Py_TYPE(apiType)->tp_free((PyObject*) apiType);
}


// dbapi type declaration
PyTypeObject yasPyTypeApiType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "yaspy.ApiType",
    .tp_basicsize = sizeof(yaspyApiType),
    .tp_dealloc = (destructor) yaspyApiTypeFree,
};
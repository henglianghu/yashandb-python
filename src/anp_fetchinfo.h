#ifndef ANCHOR_ANP_FETCHINFO_H
#define ANCHOR_ANP_FETCHINFO_H

#include "Python.h"
#include "anp_cli.h"

// External type object declaration
extern PyTypeObject anchorPyTypeFetchInfo;

// FetchInfo structure definition
typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *type;
    PyObject *display_size;
    PyObject *internal_size;
    PyObject *precision;
    PyObject *scale;
    PyObject *null_ok;
    // VECTOR specific attributes
    PyObject *vector_dimension;
    PyObject *vector_format;
} AnpFetchInfo;

// Function declarations
int anpRegisterFetchInfo(PyObject *module);
PyObject *anpGetVectorFormatObject(YapiVectorFormat format);

#endif // ANCHOR_ANP_FETCHINFO_H
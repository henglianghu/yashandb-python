#ifndef ANCHOR_ANP_VAR_H
#define ANCHOR_ANP_VAR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_cursor.h"

typedef struct StAnpVar {
    PyObject_HEAD
    AnpConnection* connection;

    uint32_t elements;  // the number of allocated elements
    uint32_t size;      // the size of single element
    uint32_t bufferSize;
    bool   isArray;
    bool   isValueSet;
    int32_t* indicator;  // an array to  specify whether the var is NULL
    char*  data;
    YapiType   dbType;
    YapiType   transType;
} AnpVar;

YapiResult anpRegisteVarObject(PyObject* module);
YapiResult anpInitDecimal();

bool   anpCheckVar(PyObject* object);
AnpVar*   anpNewVar(AnpCursor* cursor, Py_ssize_t numElements, YapiType type, Py_ssize_t size, bool isArray);
int       anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos);
PyObject* anpVarGetSingleValue(YapiConnect* hConn, AnpVar* var, uint32_t pos);

int     anpVarSetValue(YapiConnect* hConn, AnpVar* var, uint32_t arrayPos, PyObject* value);
AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements);
void anpAdjustVarTypeSize(PyObject* value, uint32_t* size,YapiType* type);

#endif  // ANCHOR_ANP_VAR_H

#ifndef ANCHOR_ANP_VAR_H
#define ANCHOR_ANP_VAR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_cursor.h"

typedef struct StAnpVar {
    PyObject_HEAD
    AnpConnection* connection;

    YacUint32 elements;  // the number of allocated elements
    YacUint32 size;      // the size of single element
    YacUint32 bufferSize;
    YacBool   isArray;
    YacBool   isValueSet;
    YacInt32* indicator;  // an array to  specify whether the var is NULL
    YacChar*  data;
    YacType   dbType;
    YacType   transType;
} AnpVar;

YacResult anpRegisteVarObject(PyObject* module);
YacResult anpInitDecimal();

YacBool   anpCheckVar(PyObject* object);
AnpVar*   anpNewVar(AnpCursor* cursor, Py_ssize_t numElements, YacType type, Py_ssize_t size, YacBool isArray);
int       anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos);
PyObject* anpVarGetSingleValue(AnpVar* var, YacUint32 pos);

int     anpVarSetValue(AnpVar* var, uint32_t arrayPos, PyObject* value);
AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements);

#endif  // ANCHOR_ANP_VAR_H

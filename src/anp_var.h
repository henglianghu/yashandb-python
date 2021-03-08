#ifndef ANCHOR_ANP_VAR_H
#define ANCHOR_ANP_VAR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_cursor.h"

typedef struct StAnpVar {
    PyObject_HEAD
    AnpConnection* connection;

    AncUint32 elements;  // the number of allocated elements
    AncUint32 size;      // the size of single element
    AncUint32 bufferSize;
    AncBool   isArray;
    AncBool   isValueSet;
    AncInt32* indicator;  // an array to  specify whether the var is NULL
    AncChar*  data;
    AncType   dbType;
    AncType   transType;
} AnpVar;

AncResult anpRegisteVarObject(PyObject* module);
AncResult anpInitDecimal();

AncBool   anpCheckVar(PyObject* object);
AnpVar*   anpNewVar(AnpCursor* cursor, Py_ssize_t numElements, AncType type, Py_ssize_t size, AncBool isArray);
int       anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos);
PyObject* anpVarGetSingleValue(AnpVar* var, AncUint32 pos);

int     anpVarSetValue(AnpVar* var, uint32_t arrayPos, PyObject* value);
AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements);

#endif  // ANCHOR_ANP_VAR_H

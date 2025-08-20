#ifndef ANCHOR_ANP_VAR_H
#define ANCHOR_ANP_VAR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_cursor.h"

typedef struct StAnpVar {
    PyObject_HEAD
    AnpConnection* connection;
    // for bind execute, it is the bind row count
    uint32_t elements;
    uint32_t size;      // the size of single element
    uint32_t bufferSize;
    bool   isArray;
    bool   isValueSet;
    int32_t* indicator;  // an array to  specify whether the var is NULL
    // bind value's offset, for var type
    uint32_t dataOffset;
    char*  data;
    YapiType   dbType;
    YapiType   transType;
    YapiParamDirection bindDir;
    bool isLobTemporary;
} AnpVar;

typedef struct StVarAssist
{
    Py_ssize_t numElements;
    Py_ssize_t size;
    YapiType type;
    bool isArray;
    bool bindIn;
} VarAssist;

YapiResult anpRegisteVarObject(PyObject* module);
YapiResult anpInitDecimal();
YapiResult anpInitJson();

bool   anpCheckVar(PyObject* object);
AnpVar*   anpNewVar(AnpCursor* cursor, VarAssist *assist);
int       anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos);
PyObject* anpVarGetSingleValue(YapiConnect* hConn, AnpVar* var, uint32_t pos);

int     anpVarSetValue(YapiConnect* hConn, AnpVar* var, uint32_t arrayPos, PyObject* value);
AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements, bool bindIn);
void anpAdjustVarTypeSize(PyObject* value, uint32_t* size,YapiType* type);
bool anpVarIsLobType(AnpVar* var);
int anpGetSize(PyObject * value);
YapiType anpGetType(PyObject * value);

#endif  // ANCHOR_ANP_VAR_H

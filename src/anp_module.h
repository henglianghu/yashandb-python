#ifndef ANCHOR_PYTHON_ANCHOR_API_H
#define ANCHOR_PYTHON_ANCHOR_API_H

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <structmember.h>
#include <stdbool.h>
#include "anp_cli.h"

extern YapiEnv* anpEnv;

extern PyTypeObject *anpPyTypeDate;
extern PyTypeObject *anpPyTypeDateTime;
extern PyTypeObject *anpPyTypeTime;
extern PyTypeObject *anpPyTypeTimeDelta;
extern PyTypeObject *anpPyTypeDecimal;

typedef struct yaspyApiType yaspyApiType;

extern yaspyApiType *yaspyApiTypeBool;
extern yaspyApiType *yaspyApiTypeTinyint;
extern yaspyApiType *yaspyApiTypeSmallint;
extern yaspyApiType *yaspyApiTypeInteger;
extern yaspyApiType *yaspyApiTypeBigint;
extern yaspyApiType *yaspyApiTypeFloat;
extern yaspyApiType *yaspyApiTypeDouble;
extern yaspyApiType *yaspyApiTypeNumber;
extern yaspyApiType *yaspyApiTypeDate;
extern yaspyApiType *yaspyApiTypeTime;
extern yaspyApiType *yaspyApiTypeDatetime;
extern yaspyApiType *yaspyApiTypeTimedelta;
extern yaspyApiType *yaspyApiTypeChar;
extern yaspyApiType *yaspyApiTypeVarchar;
extern yaspyApiType *yaspyApiTypeNchar;
extern yaspyApiType *yaspyApiTypeNvarchar;
extern yaspyApiType *yaspyApiTypeBinary;
extern yaspyApiType *yaspyApiTypeBit;
extern yaspyApiType *yaspyApiTypeRowid;
extern yaspyApiType *yaspyApiTypeJson;
extern yaspyApiType *yaspyApiTypeNone;
extern yaspyApiType *yaspyApiTypeYeardelta;
extern yaspyApiType *yaspyApiTypeBlob;
extern yaspyApiType *yaspyApiTypeClob;
extern yaspyApiType *yaspyApiTypeNclob;


struct yaspyApiType
{
    PyObject_HEAD
    const char *name;
    PyObject *dbTypes;
    YapiType defaultDbType;
};

extern PyTypeObject yasPyTypeApiType;

int yaspyModuleAddApiType(PyObject *module, const char *name, YapiType defaultDbType, yaspyApiType **apiType);

#endif //ANCHOR_PYTHON_ANCHOR_API_H

#include "anp_var.h"
#include "datetime.h"
#include "anp_exception.h"
// #include "yacapi.h"

PyTypeObject *anpPyTypeDate;
PyTypeObject *anpPyTypeDateTime;
PyTypeObject *anpPyTypeTime;
PyTypeObject *anpPyTypeTimeDelta;
PyTypeObject *anpPyTypeDecimal;
PyTypeObject *anpPyTypeArray;

bool anpVarIsLobType(AnpVar* var) {
    if (var->transType == YAPI_TYPE_CLOB || var->transType == YAPI_TYPE_BLOB || var->transType == YAPI_TYPE_NCLOB ||
        var->transType == YAPI_TYPE_JSON) {
        return true;
    }

    return false;
}

static void anpVarFree(AnpVar *var)
{
    if (var->data) {
        if (anpVarIsLobType(var)) {
            for (uint32_t i = 0; i < var->elements; i++) {
                if (yapiLobDescFree(*(YapiLobLocator**)(var->data + i * sizeof(YapiLobLocator*)), var->transType) != YAPI_SUCCESS) {
                    (void)anpRaiseAndReturnNullException();
                }
            }
            Py_CLEAR(var->typeData.lobCacheObj);
        } else if (var->dbType == YAPI_TYPE_VECTOR) {
            // Free YapiVector objects for VECTOR type
            for (uint32_t i = 0; i < var->elements; i++) {
                YapiVector** vectorSlot = (YapiVector**)(var->data + i * sizeof(YapiVector*));
                YapiVector* vector = *vectorSlot;
                if (vector != NULL) {
                    yapiDescFree2(anpEnv, vector, YAPI_DESC_VECTOR);
                    *vectorSlot = NULL;
                }
            }
            // Use memset to set all pointers to NULL to prevent double free
            memset(var->data, 0, var->bufferSize);
        }
        PyMem_Free(var->data);
    }
    if (var->indicator) {
        PyMem_Free(var->indicator);
    }
    Py_CLEAR(var->connection);
    Py_TYPE(var)->tp_free((PyObject*) var);
}

static PyObject * anpVarRepr(AnpVar *var)
{
    // temp support
    if (var == NULL) {
        return NULL;
    }

    return PyUnicode_FromFormat("<yaspy.Var type %d value>", var->dbType);
}

static PyObject * anpVarGetType(AnpVar *var, void *unused)
{
    return PyLong_FromLong(var->dbType);
}

static PyObject* yaspyVarGetValues(AnpVar *var, void *unused)
{
    // temporarily only return one value
    PyObject *values = PyList_New(1);
    if (values == NULL) {
        return NULL;
    }

    PyObject *singleValue = anpVarGetSingleValue(var->connection->hConn, var, 0);
    if (singleValue == NULL) {
        Py_DECREF(values);
        return NULL;
    }
    PyList_SET_ITEM(values, 0, singleValue);

    return values;
}

static PyObject* anpVarGetElementCount(AnpVar* var)
{
    return PyLong_FromLong(var->elements);
}

static PyObject* yaspyVarSetValue(AnpVar* var, PyObject *args)
{
    PyObject *value;
    uint32_t pos = 0;

    if (!PyArg_ParseTuple(args, "O|i", &value, &pos)) {
        return NULL;
    }

    if (pos >= var->elements) {
        PyObject* errMsg = PyUnicode_FromString("set value index out of range");
        PyErr_SetObject(PyExc_IndexError, errMsg);
        Py_DECREF(errMsg);
        return NULL;
    }

    if (anpVarSetValue(var->connection->hConn, var, pos, value) < 0) {
        return NULL;
    }

    switch (var->dbType) {
        case YAPI_TYPE_BIGINT: {
            int overflow = 0;
            PyLong_AsLongLongAndOverflow(value, &overflow);
            if (overflow) {
                var->size = anpGetSize(value);
                var->dbType = YAPI_TYPE_VARCHAR;
            }
            break;
        }
        case YAPI_TYPE_NUMBER:
        case YAPI_TYPE_ROWID:
        case YAPI_TYPE_JSON:
        case YAPI_TYPE_YM_INTERVAL:
            var->dbType = YAPI_TYPE_VARCHAR;
            var->size = anpGetSize(value);
            break;
        case YAPI_TYPE_BIT:
            var->dbType = YAPI_TYPE_BIGINT;
            var->size = anpGetSize(value);
            break;
        default:
            break;
    }


    var->bindDir = YAPI_PARAM_INPUT;
    Py_INCREF(var);
    return (PyObject*)var;
}

static PyObject* yaspyVarGetValue(AnpVar* var, PyObject* args, PyObject* keywordArgs)
{
    static char *keywordList[] = { "pos", NULL };
    uint32_t pos = 0;

    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|i", keywordList, &pos)) {
        return NULL;
    }
    if (pos >= var->elements) {
        PyObject* errMsg = PyUnicode_FromString("get value index out of range");
        PyErr_SetObject(PyExc_IndexError, errMsg);
        Py_DECREF(errMsg);
        return NULL;
    }
    return anpVarGetSingleValue(var->connection->hConn, var, pos);
}

static PyObject* yaspyVarFree(AnpVar* var)
{
    if (var->data) {
        if (anpVarIsLobType(var)) {
            for (uint32_t i = 0; i < var->elements; i++) {
                YapiLobLocator** loc = (YapiLobLocator**)(var->data + i * sizeof(YapiLobLocator*));
                if (var->isLobTemporary && loc && yapiLobFreeTemporary(var->connection->hConn, *loc) != YAPI_SUCCESS) {
                    (void)anpRaiseAndReturnIntException();
                }
                if (yapiLobDescFree(*loc, var->transType) != YAPI_SUCCESS) {
                    (void)anpRaiseAndReturnNullException();
                }
            }
        } else if (var->dbType == YAPI_TYPE_VECTOR) {
            // Free YapiVector objects for VECTOR type
            for (uint32_t i = 0; i < var->elements; i++) {
                YapiVector** vectorSlot = (YapiVector**)(var->data + i * sizeof(YapiVector*));
                YapiVector* vector = *vectorSlot;
                if (vector != NULL) {
                    yapiDescFree2(anpEnv, vector, YAPI_DESC_VECTOR);
                    *vectorSlot = NULL;
                }
            }
            // Use memset to set all pointers to NULL to prevent double free
            memset(var->data, 0, var->bufferSize);
        }
        PyMem_Free(var->data);
        var->data = NULL;
    }
    Py_RETURN_NONE;
}

static PyGetSetDef anpCalcMembers[] = {
    { "actual_elements", (getter) anpVarGetElementCount, 0, 0, 0 },
    { "actualElements", (getter) anpVarGetElementCount, 0, 0, 0 },
    { "type",           (getter)anpVarGetType,                   0, 0, 0 },
    { "values",         (getter)yaspyVarGetValues,               0, 0, 0 },
    { NULL }
};

static PyMethodDef anpVarMethods[] = {
    { "setvalue" , (PyCFunction) yaspyVarSetValue, METH_VARARGS },
    { "getvalue" , (PyCFunction) yaspyVarGetValue, METH_VARARGS | METH_KEYWORDS },
    { "free" , (PyCFunction) yaspyVarFree, METH_NOARGS },
    { NULL }
};

PyTypeObject anchorPyTypeVar = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "yaspy.Var",
    .tp_basicsize = sizeof(AnpVar),
    .tp_dealloc = (destructor)anpVarFree,
    .tp_repr = (reprfunc)anpVarRepr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = anpVarMethods,
    .tp_getset = anpCalcMembers
};


static PyTypeObject * anpPyTypeJsonDumps;
static PyTypeObject * anpPyTypeJsonLoads;

YapiResult anpInitJson()
{
    PyObject *module;

    module = PyImport_ImportModule("json");
    if (module == NULL) {
        return YAPI_ERROR;
    }
    anpPyTypeJsonDumps = (PyTypeObject*) PyObject_GetAttrString(module, "dumps");
    if (anpPyTypeJsonDumps == NULL) {
        Py_DECREF(module);
        return YAPI_ERROR;
    }
    anpPyTypeJsonLoads = (PyTypeObject*) PyObject_GetAttrString(module, "loads");
    if (anpPyTypeJsonLoads == NULL) {
        Py_DECREF(module);
        return YAPI_ERROR;
    }
    Py_DECREF(module);
    return YAPI_SUCCESS;
}

YapiResult anpInitDecimal()
{
    PyObject *module;

    PyDateTime_IMPORT;
    if (PyErr_Occurred()) {
        return YAPI_ERROR;
    }
    anpPyTypeDate = PyDateTimeAPI->DateType;
    anpPyTypeDateTime = PyDateTimeAPI->DateTimeType;
    anpPyTypeTime = PyDateTimeAPI->TimeType;
    anpPyTypeTimeDelta = PyDateTimeAPI->DeltaType;

    module = PyImport_ImportModule("decimal");
    if (module == NULL) {
        return YAPI_ERROR;
    }
    anpPyTypeDecimal = (PyTypeObject*) PyObject_GetAttrString(module, "Decimal");
    Py_DECREF(module);
    if (anpPyTypeDecimal == NULL) {
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

YapiResult anpInitArray()
{
    PyObject *module;

    module = PyImport_ImportModule("array");
    if (module == NULL) {
        return YAPI_ERROR;
    }

    anpPyTypeArray = (PyTypeObject*) PyObject_GetAttrString(module, "array");
    Py_DECREF(module);

    if (anpPyTypeArray == NULL) {
        return YAPI_ERROR;
    }

    return YAPI_SUCCESS;
}

YapiResult anpRegisteVarObject(PyObject* module)
{
    PyType_Ready(&anchorPyTypeVar);

    Py_INCREF(&anchorPyTypeVar);
    if (PyModule_AddObject(module, "Var", (PyObject*) &anchorPyTypeVar) < 0) {
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

AnpVar* anpNewVar(AnpCursor* cursor, VarAssist *assist)
{
    AnpVar* var = (AnpVar*) anchorPyTypeVar.tp_alloc(&anchorPyTypeVar, 0);
    if (var == NULL) {
        return NULL;
    }

    Py_INCREF(cursor->connection);
    var->connection = cursor->connection;
    var->bindDir = 0;
    if (assist->bindIn) {
        var->bindDir = YAPI_PARAM_INPUT;
    }

    YapiType type = assist->type;
    Py_ssize_t size = assist->size;

    // for executemany, char/varchar allocated size is max
    if ((assist->numElements > 1) && assist->bindIn && (size <= CONVERT_TO_LOB_SIZE)) {
        // for lob type，use buffer stream upload if the size not exceeds the limit
        if (type == YAPI_TYPE_BLOB || type == YAPI_TYPE_CLOB || type == YAPI_TYPE_NCLOB || type == YAPI_TYPE_JSON) {
            type = YAPI_TYPE_VARCHAR;
        }
        if (((type >= YAPI_TYPE_CHAR) && (type <= YAPI_TYPE_NVARCHAR)) || (type == YAPI_TYPE_BINARY)) {
            size = (Py_ssize_t) CONVERT_TO_LOB_SIZE;
        }
    }
    var->dataOffset = 0;

    var->size = size;
    var->elements = (uint32_t)assist->numElements;
    var->isArray = assist->isArray;
    var->bufferSize = var->size * var->elements;
    var->dbType = type;
    var->isLobTemporary = false;
    var->typeData.lobCacheObj = NULL;
    if(type == YAPI_TYPE_NUMBER || type == YAPI_TYPE_NUMBER_FLOAT || type == YAPI_TYPE_BIT || type == YAPI_TYPE_ROWID || 
        type == YAPI_TYPE_YM_INTERVAL || type == YAPI_TYPE_JSON) {
        var->transType = YAPI_TYPE_VARCHAR;
    } else {
        var->transType = type;
    }

    if (assist->bindIn && (size > CONVERT_TO_LOB_SIZE)) {
        if (type == YAPI_TYPE_CHAR || type == YAPI_TYPE_VARCHAR) {
            var->transType = YAPI_TYPE_CLOB;
            var->dbType = YAPI_TYPE_CLOB;
        } else if(type == YAPI_TYPE_BINARY) {
            var->transType = YAPI_TYPE_BLOB;
            var->dbType = YAPI_TYPE_BLOB;
        }
    }
    
    if (anpVarIsLobType(var)) {
        var->size = -1;
        var->bufferSize = sizeof(YapiLobLocator*) * var->elements;
        var->data = PyMem_Malloc(var->bufferSize);
        if (var->data == NULL) {
            Py_DECREF(var);
            return (AnpVar*)PyErr_NoMemory();
        }

        memset(var->data, 0, var->bufferSize);

        YapiLobLocator* loc;
        for (uint32_t i = 0; i < var->elements; i++) {
            if (yapiLobDescAlloc((YapiConnect*)var->connection->hConn, var->transType, (void**)&loc) != YAPI_SUCCESS) {
                return (AnpVar*)anpRaiseAndReturnNullException();
            }
            if (!cursor->fetchVariables) {
                if (yapiLobCreateTemporary((YapiConnect*)var->connection->hConn, loc) != YAPI_SUCCESS) {
                    return (AnpVar*)anpRaiseAndReturnNullException();
                }
                var->isLobTemporary = true;
            }
            memcpy(var->data + i * sizeof(YapiLobLocator*), &loc, sizeof(YapiLobLocator*));
        }
    } else if (var->dbType == YAPI_TYPE_VECTOR) {
        // For VECTOR type, we store pointers to YapiVector objects
        var->size = sizeof(YapiVector*);
        var->bufferSize = var->size * var->elements;
        var->data = PyMem_Malloc(var->bufferSize);
        if (var->data == NULL) {
            Py_DECREF(var);
            return (AnpVar*)PyErr_NoMemory();
        }

        memset(var->data, 0, var->bufferSize);
        // Initialize vector format to a default value
        var->typeData.vectorFormat = YAPI_VECTOR_FORMAT_FLEX;

        // Allocate YapiVector objects for each element
        for (uint32_t i = 0; i < var->elements; i++) {
            YapiVector* vector = NULL;
            if (yapiDescAlloc2(anpEnv, (void**)&vector, YAPI_DESC_VECTOR) != YAPI_SUCCESS) {
                Py_DECREF(var);
                return (AnpVar*)anpRaiseAndReturnNullException();
            }
            // Store the vector pointer in data
            *((YapiVector**)(var->data + i * sizeof(YapiVector*))) = vector;
        }
    } else {
        var->data = PyMem_Malloc(var->bufferSize);
        if (var->data == NULL) {
            Py_DECREF(var);
            return (AnpVar*)PyErr_NoMemory();
        }
    }

    var->indicator = PyMem_Malloc(var->elements * sizeof(uint32_t));
    if (var->indicator == NULL) {
        Py_DECREF(var);
        return (AnpVar*)PyErr_NoMemory();
    }

    return var;
}

bool anpCheckVar(PyObject* object)
{
    return (Py_TYPE(object) == &anchorPyTypeVar);
}

static PyObject* anpGetLobData(YapiConnect* hConn, YapiType type, char* data)
{
    YapiLobLocator* loc = (YapiLobLocator*)data;
    char readBuf[LOB_BUFFER_SIZE];
    uint64_t length = LOB_BUFFER_SIZE;
    PyObject* byteArray = PyByteArray_FromStringAndSize(NULL, 0);
    if (!byteArray) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create bytearray");
        return NULL;
    }

    while (length > 0) {
        if (yapiLobRead(hConn, loc, &length, (uint8_t*)readBuf, LOB_BUFFER_SIZE) != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
        uint64_t origSize = PyByteArray_Size(byteArray);
        uint64_t newSize = origSize + length;

        if (PyByteArray_Resize(byteArray, newSize) < 0) {
            Py_DECREF(byteArray);
            PyErr_SetString(PyExc_RuntimeError, "Failed to resize bytearray");
            return NULL;
        }

        char* byteArrayBuf = PyByteArray_AsString(byteArray);
        if (!byteArrayBuf) {
            Py_DECREF(byteArray);
            PyErr_SetString(PyExc_RuntimeError, "Failed to get bytearray buffer");
            return NULL;
        }

        memcpy(byteArrayBuf + origSize, readBuf, length);
    }

    PyObject* var = NULL;
    char* dataBuf = PyByteArray_AsString(byteArray);
    size_t size = PyByteArray_Size(byteArray);
    if (type == YAPI_TYPE_BLOB) {
        var = PyBytes_FromStringAndSize(dataBuf, size);
        Py_DECREF(byteArray);
    } else {
        var = PyUnicode_FromString(dataBuf);
        Py_DECREF(byteArray);
        if (!var) {
            PyErr_Clear();
            PyObject* bytesObj = PyBytes_FromStringAndSize(data, size);
            PyObject* reprStr = PyObject_Repr(bytesObj);
            Py_DECREF(bytesObj);
            return reprStr;
        }
    }

    return var;
}

static PyObject* anpVarToPyDelta(char* data)
{
    int dsDays;
    int dsHours;
    int dsMinutes;
    int dsSeconds;
    int dsMicroSecs;

    YapiDSInterval dsInterval = *(YapiDSInterval*)data;
    if (yapiDSIntervalGetDaySecond(dsInterval, &dsDays, &dsHours, &dsMinutes, &dsSeconds, &dsMicroSecs) != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }

    dsSeconds = dsHours * 60 * 60 + dsMinutes * 60 + dsSeconds;
    return PyDelta_FromDSU(dsDays, dsSeconds, dsMicroSecs);
}

static PyObject* anpVarToPyVector(AnpVar* var, YapiVector* vector)
{
    // Get vector info - use size directly from YapiVector
    uint16_t dim = 0;
    uint32_t arrayLen = vector->size;
    
    // Check if vector format is valid, if not, get it from the vector
    if (var->typeData.vectorFormat != YAPI_VECTOR_FORMAT_FLOAT32 &&
        var->typeData.vectorFormat != YAPI_VECTOR_FORMAT_FLOAT64) {
        // Invalid format, get it from the vector
        if (yapiVectorGetFormat(vector, &var->typeData.vectorFormat) != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
    }
    
    // Allocate buffer for array data
    uint8_t* arrayBuffer = (uint8_t*)PyMem_Malloc(arrayLen);
    if (!arrayBuffer) {
        return PyErr_NoMemory();
    }
    
    // Convert vector to array using the format stored in var
    if (yapiVectorToArray(vector, var->typeData.vectorFormat, &dim, arrayBuffer, &arrayLen, 0) != YAPI_SUCCESS) {
        PyMem_Free(arrayBuffer);
        return anpRaiseAndReturnNullException();
    }
    
    // Create Python array based on format stored in var
    const char* typecode;
    switch (var->typeData.vectorFormat) {
        case YAPI_VECTOR_FORMAT_FLOAT32:
            typecode = "f";  // float32
            break;
        case YAPI_VECTOR_FORMAT_FLOAT64:
            typecode = "d";  // float64
            break;
        default:
            PyMem_Free(arrayBuffer);
            return anpRaiseExceptionFromString(anpNotSupportedException, "Unsupported vector format");
    }
    
    // Create array constructor args
    PyObject* arrayArgs = Py_BuildValue("(s)", typecode);
    if (!arrayArgs) {
        PyMem_Free(arrayBuffer);
        return NULL;
    }
    
    // Create array object
    PyObject* arrayObj = PyObject_CallObject((PyObject*)anpPyTypeArray, arrayArgs);
    Py_DECREF(arrayArgs);
    
    if (!arrayObj) {
        PyMem_Free(arrayBuffer);
        return NULL;
    }
    
    // Set array data from buffer
    PyObject* fromBytesFunc = PyObject_GetAttrString(arrayObj, "frombytes");
    if (!fromBytesFunc) {
        Py_DECREF(arrayObj);
        PyMem_Free(arrayBuffer);
        return NULL;
    }
    
    PyObject* bufferBytes = PyBytes_FromStringAndSize((char*)arrayBuffer, arrayLen);
    if (!bufferBytes) {
        Py_DECREF(fromBytesFunc);
        Py_DECREF(arrayObj);
        PyMem_Free(arrayBuffer);
        return NULL;
    }
    
    PyObject* callResult = PyObject_CallFunctionObjArgs(fromBytesFunc, bufferBytes, NULL);
    Py_DECREF(bufferBytes);
    Py_DECREF(fromBytesFunc);
    
    if (!callResult) {
        Py_DECREF(arrayObj);
        PyMem_Free(arrayBuffer);
        return NULL;
    }
    
    Py_DECREF(callResult);
    PyMem_Free(arrayBuffer);
    return arrayObj;
}

static PyObject *anpVarToPython(YapiConnect* hConn, AnpVar* var, uint32_t pos)
{
    char message[120];
    PyObject* result;
    YapiDate *date;
    YapiShortTime* time;
    YapiTimestamp *timestamp;
    YapiDateStruct ds;
    
    YapiType type = var->dbType;
    char* data =  var->data + pos * var->size;

    switch (type) {
        case YAPI_TYPE_BOOL:
            result = PyBool_FromLong(*(int8_t*)data);
            break;
        case YAPI_TYPE_TINYINT:
            result = PyLong_FromLong(*(int8_t*)data);
            break;
        case YAPI_TYPE_SMALLINT:
            result = PyLong_FromLong(*(int16_t*)data);
            break;
        case YAPI_TYPE_INTEGER:
            result =  PyLong_FromLong(*(int32_t*)data);
            break;
        case YAPI_TYPE_BIGINT:
            result =  PyLong_FromLongLong(*(int64_t*)data);
            break;
        case YAPI_TYPE_FLOAT:
            result =  PyFloat_FromDouble(*(float *)data);
            break;
        case YAPI_TYPE_DOUBLE:
            result =  PyFloat_FromDouble(*(double *)data);
            break;
        case YAPI_TYPE_NUMBER:
        case YAPI_TYPE_NUMBER_FLOAT: {
            PyObject* stringObj = PyUnicode_Decode(data, strlen(data), NULL, NULL);
            result = PyObject_CallFunctionObjArgs((PyObject*)anpPyTypeDecimal, stringObj, NULL);
            Py_DECREF(stringObj);
            break;
        }
        case YAPI_TYPE_DATE:
            date = (YapiDate *)data;
            yapiGetDateStruct(*date, &ds);
            result =  PyDate_FromDate(ds.year, ds.month, ds.day);
            break;
        case YAPI_TYPE_SHORTTIME:
            time = (YapiShortTime *)data;
            yapiGetDateStruct(*time, &ds);
            result = PyTime_FromTime(ds.hour, ds.minute, ds.second, ds.fraction);
            break;
        case YAPI_TYPE_TIMESTAMP:
        case YAPI_TYPE_TIMESTAMP_TZ:
        case YAPI_TYPE_TIMESTAMP_LTZ:
            timestamp = (YapiTimestamp*)data;
            yapiGetDateStruct(timestamp->stamp, &ds);
            result =  PyDateTime_FromDateAndTime(ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second, ds.fraction);
            break;
 
        case YAPI_TYPE_CHAR:
        case YAPI_TYPE_VARCHAR:
        case YAPI_TYPE_ROWID:
        case YAPI_TYPE_BIT:
        case YAPI_TYPE_YM_INTERVAL:
            result = PyUnicode_FromString(data);
            break;

        case YAPI_TYPE_NCHAR:
        case YAPI_TYPE_NVARCHAR:
            result = PyUnicode_Decode(data, (Py_ssize_t)var->indicator[pos], "utf-16", "strict");
            break;

        case YAPI_TYPE_DS_INTERVAL:
            result = anpVarToPyDelta(data);
            break;

        case YAPI_TYPE_BINARY:
            result = PyBytes_FromStringAndSize(data, (Py_ssize_t)var->indicator[pos]);
            break;

        case YAPI_TYPE_CLOB:
        case YAPI_TYPE_BLOB:
        case YAPI_TYPE_NCLOB:
            if (var->typeData.lobCacheObj != NULL) {
                Py_INCREF(var->typeData.lobCacheObj);
                result = var->typeData.lobCacheObj;
                break;
            }
            var->typeData.lobCacheObj = anpGetLobData(hConn, type, (char*)*((YapiLobLocator**)data));
            Py_INCREF(var->typeData.lobCacheObj);
            result = var->typeData.lobCacheObj;
            break;
        case YAPI_TYPE_JSON: {
            PyObject* jsonStr = PyUnicode_FromString(data);
            PyObject* args = Py_BuildValue("(O)", jsonStr);
            if (!args) {
                Py_DECREF(jsonStr);
                result = anpRaiseExceptionFromString(anpProgrammingErrorException, "prepare json loads args failed");
                break;
            }
            result = PyObject_CallObject((PyObject*)anpPyTypeJsonLoads, args);
            if (!result) {
                Py_DECREF(jsonStr);
                Py_DECREF(args);
                PyErr_Print();
                result = anpRaiseExceptionFromString(anpProgrammingErrorException, "json loads failed");
                break;
            }
            Py_DECREF(jsonStr);
            Py_DECREF(args);
            break;
        }
        case YAPI_TYPE_VECTOR: {
            YapiVector* vector = *((YapiVector**)data);
            result = anpVarToPyVector(var, vector);
            break;
        }
        default:
            snprintf(message, 120, "not support type %d", type);
            result = anpRaiseExceptionFromString(anpNotSupportedException, message);
            break;
    }
    return result;
}

PyObject* anpVarGetSingleValue(YapiConnect* hConn, AnpVar* var, uint32_t pos)
{
    if (pos > 1) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "AnpVar not support multi value");
    }
    if (var->indicator[pos] == YAPI_NULL_DATA) {
        Py_RETURN_NONE;
    }
    return anpVarToPython(hConn, var, pos);
}

int anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos)
{
    const char *nameStr;
    if (name) {
        if (!PyUnicode_Check(name)) {
            anpRaiseExceptionFromString(anpInterfaceErrorException, "the name of bind paramter is not string");
            return YAPI_ERROR;
        }
        nameStr = PyUnicode_AsUTF8(name);
        if (nameStr == NULL) {
            anpRaiseExceptionFromString(anpInterfaceErrorException, "the name of bind paramter should not null");
            return YAPI_ERROR;
        }
    }

    uint32_t bindSize = var->size;
    if ((var->elements > 1) && (var->dataOffset > 0)) {
        bindSize = -2;
    }

    if (name) {
        if (yapiBindParameterByName(cursor->hStmt, (char*)nameStr, var->bindDir, var->dbType, var->data, bindSize,
                                    var->bufferSize, var->indicator) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
    } else {
        if (yapiBindParameter(cursor->hStmt, pos, var->bindDir, var->dbType, var->data, bindSize, var->bufferSize,
                              var->indicator) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
    }
    return 0;
}

int adjustAnpVarBuffSize(AnpVar* var, uint32_t elementSize)
{
    if (var->size >= elementSize) {
        return 0;
    }

    size_t relocSize = (size_t)elementSize*var->elements;
    if (relocSize < var->bufferSize*2) {
        relocSize = (size_t)(var->bufferSize*2);
    }

    var->data = PyMem_Realloc(var->data, relocSize);
    if (var->data == NULL) {
        return -1;
    }

    var->bufferSize = relocSize;
    var->size = elementSize;
    return 0;
}

static int anpVarSetDecimal(AnpVar* var, uint32_t arrayPos, PyObject* value) 
{
    PyObject *strValue = PyObject_Str(value);
    if (!strValue) {
        anpRaiseExceptionFromString(anpProgrammingErrorException, "get decimal string failed");
        return -1;
    }

    Py_ssize_t enCodeStrSize = 0;
    const char* bindStr = PyUnicode_AsUTF8AndSize(strValue, &enCodeStrSize);
    if (bindStr == NULL) {
        Py_DECREF(strValue);
        return -1;
    }

    uint32_t costSize = (uint32_t)(enCodeStrSize + 1);
    if (costSize > var->size) {
        if (adjustAnpVarBuffSize(var, costSize) < 0) {
            return -1;
        }
    }

    strcpy(var->data + var->dataOffset, bindStr);
    var->indicator[arrayPos] = (int32_t)enCodeStrSize;
    var->dataOffset += costSize;
    var->data[var->dataOffset - 1] = '\0';

    Py_DECREF(strValue);
    return 0;
}

static int anpVarSetPyDelta(AnpVar* var, uint32_t arrayPos, PyObject* value)
{
    YapiDSInterval *dsInterval = (YapiDSInterval*)var->data;
    *(dsInterval + arrayPos) = 0;
    int deltaSeconds = PyDateTime_DELTA_GET_SECONDS(value);
    int hour = deltaSeconds / 3600;
    int seconds = deltaSeconds % 3600;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    int microSeconds = PyDateTime_DELTA_GET_MICROSECONDS(value);
    int days = PyDateTime_DELTA_GET_DAYS(value);
    if (yapiDSIntervalSetDaySecond(dsInterval + arrayPos, days, hour, minutes, seconds, microSeconds) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }
    var->indicator[arrayPos] = (int32_t)sizeof(YapiDSInterval);
    return 0;
}

static const char* anpVarGetJsonStringAndSize(PyObject* value, Py_ssize_t* size)
{
    PyObject* args = Py_BuildValue("(O)", value);
    if (!args) {
        anpRaiseExceptionFromString(anpProgrammingErrorException, "prepare json dumps args failed");
        return NULL;
    }
    // this ref count should decreased by caller
    PyObject* jsonStr = PyObject_CallObject((PyObject*)anpPyTypeJsonDumps, args);
    if (!jsonStr) {
        Py_DECREF(args);
        anpRaiseExceptionFromString(anpProgrammingErrorException, "json dumps failed");
        return NULL;
    }
    Py_ssize_t  enCodeStrSize = 0;
    const char* bindStr = PyUnicode_AsUTF8AndSize(jsonStr, &enCodeStrSize);
    if (bindStr == NULL) {
        Py_DECREF(args);
        Py_DECREF(jsonStr);
        return NULL;
    }
    *size = enCodeStrSize;
    Py_DECREF(args);
    return bindStr;
}

int anpVarSetValue(YapiConnect* hConn, AnpVar* var, uint32_t arrayPos, PyObject* value)
{
    if (value == Py_None){
        var->indicator[arrayPos] = YAPI_NULL_DATA;
        if (var->dbType == YAPI_TYPE_UNKNOWN) {
            var->dbType = YAPI_TYPE_VARCHAR;
        }
        return 0;
    }

    if (PyBool_Check(value)) {
        bool* b = (bool *)var->data;
        b[arrayPos] = PyObject_IsTrue(value);
        var->indicator[arrayPos] = (int32_t)sizeof(bool);
        return 0;
    }

    if (PyUnicode_Check(value)) {
        Py_ssize_t enCodeStrSize = 0;
        const char* bindStr = PyUnicode_AsUTF8AndSize(value, &enCodeStrSize);
        if (bindStr == NULL) {
            return -1;
        }

        if (anpVarIsLobType(var)) {
            if (yapiLobWrite(hConn, *(YapiLobLocator**)(var->data + var->dataOffset), NULL, (uint8_t*)bindStr,
                             (uint64_t)enCodeStrSize) != YAPI_SUCCESS) {
                return -1;
            }
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
            var->dataOffset += sizeof(YapiLobLocator*);
        } else {
            if (var->dbType == YAPI_TYPE_BOOL) {
                bool* b = (bool*)var->data;
                b[arrayPos] = PyObject_IsTrue(value);
                var->indicator[arrayPos] = (int32_t)sizeof(bool);
                return 0;
            }
            uint32_t costSize = (uint32_t)(enCodeStrSize + 1);
            if (costSize > var->size) {
                if (adjustAnpVarBuffSize(var, costSize) < 0) {
                    return -1;
                }
            }

            memcpy(var->data + var->dataOffset, bindStr, enCodeStrSize);
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
            var->dataOffset += costSize;
            var->data[var->dataOffset - 1] = '\0';
        }
        return 0;
    }

    if (PyBytes_Check(value)) {
        if (anpVarIsLobType(var)) {
             if (yapiLobWrite(hConn, *(YapiLobLocator**)(var->data + var->dataOffset), NULL, 
                (uint8_t*)PyBytes_AS_STRING(value), (int)PyBytes_GET_SIZE(value)) != YAPI_SUCCESS) {
                return -1;
             }
             var->indicator[arrayPos] = (int32_t)PyBytes_GET_SIZE(value);
             var->dataOffset += sizeof(YapiLobLocator*);
             return 0;
        }
        if (var->dbType == YAPI_TYPE_BOOL) {
            bool* b = (bool*)var->data;
            b[arrayPos] = PyObject_IsTrue(value);
            var->indicator[arrayPos] = (int32_t)sizeof(bool);
            return 0;
        }

        Py_ssize_t byteSize = PyBytes_GET_SIZE(value);
        if (byteSize == 0) {
            var->indicator[arrayPos] = YAPI_NULL_DATA;
            return 0;
        }

        uint32_t costSize = (uint32_t)byteSize;
        if (costSize > var->size) {
            if (adjustAnpVarBuffSize(var, costSize) < 0) {
                return -1;
            }
        }

        memcpy(var->data + var->dataOffset, PyBytes_AS_STRING(value), byteSize);
        var->indicator[arrayPos] = (int32_t)costSize;
        var->dataOffset += costSize;
        return 0;
    }
    
    if (PyLong_Check(value)) {
        if (var->dbType == YAPI_TYPE_VARCHAR) {
            Py_ssize_t  enCodeStrSize = 0;
            PyObject*   numberStr = PyObject_Str(value);
            const char* bindStr = PyUnicode_AsUTF8AndSize(numberStr, &enCodeStrSize);
            if (bindStr == NULL) {
                return -1;
            }
            uint32_t costSize = (uint32_t)(enCodeStrSize + 1);
            if (adjustAnpVarBuffSize(var, NUMBER_STRING_BUFFER_SIZE) < 0) {
                return -1;
            }

            strcpy(var->data + var->dataOffset, bindStr);
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
            var->dataOffset += costSize;
            var->data[var->dataOffset - 1] = '\0';
            return 0;
        } else if (var->dbType == YAPI_TYPE_NUMBER) {
            Py_ssize_t  enCodeStrSize = 0;
            PyObject*   numberStr = PyObject_Str(value);
            const char* bindStr = PyUnicode_AsUTF8AndSize(numberStr, &enCodeStrSize);
            if (bindStr == NULL) {
                return -1;
            }
            if (yapiNumberFromText(bindStr, strlen(bindStr), NUMBER_FROM_INTEGER_STRING_FMT,
                                   NUMBER_FROM_INTEGER_STRING_FMT_STR_LEN, NULL, 0,
                                   (YapiNumber*)(var->data + var->dataOffset)) != YAPI_SUCCESS) {
                return anpRaiseAndReturnIntException();
            }

            var->indicator[arrayPos] = var->size;
            var->dataOffset += var->size;
            return 0;
        } else if (var->dbType == YAPI_TYPE_BOOL) {
            bool* b = (bool *)var->data;
            b[arrayPos] = PyObject_IsTrue(value);
            var->indicator[arrayPos] = (int32_t)sizeof(bool);
            return 0;
        }

        int64_t* iv = (int64_t*)var->data;
        int64_t  bindValue = PyLong_AsLongLong(value);
        PyObject* pyError = PyErr_Occurred();
        if ((bindValue == -1L) && (pyError != NULL)) {
            PyErr_SetString(pyError, "fail to get long long value from PyObject");
            return -1;
        }

        iv[arrayPos] = bindValue;
        var->indicator[arrayPos] = (int32_t)sizeof(int64_t);
        return 0;
    }

    if (PyFloat_Check(value)) {
        if (var->dbType == YAPI_TYPE_BOOL) {
            bool* b = (bool *)var->data;
            b[arrayPos] = PyObject_IsTrue(value);
            var->indicator[arrayPos] = (int32_t)sizeof(bool);
            return 0;
        }
        if (var->dbType == YAPI_TYPE_VARCHAR) {
            Py_ssize_t  enCodeStrSize = 0;
            PyObject*   numberStr = PyObject_Str(value);
            const char* bindStr = PyUnicode_AsUTF8AndSize(numberStr, &enCodeStrSize);
            if (bindStr == NULL) {
                return -1;
            }
            uint32_t costSize = (uint32_t)(enCodeStrSize + 1);
            if (adjustAnpVarBuffSize(var, NUMBER_STRING_BUFFER_SIZE) < 0) {
                return -1;
            }

            strcpy(var->data + var->dataOffset, bindStr);
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
            var->dataOffset += costSize;
            var->data[var->dataOffset - 1] = '\0';
            return 0;
        }
        if (var->dbType == YAPI_TYPE_DOUBLE) {
          double* dv = (double*)var->data;
          dv[arrayPos] = PyFloat_AsDouble(value);
          var->indicator[arrayPos] = (int32_t)sizeof(double);
          return 0;
        }
        float* fv = (float*)var->data;
        fv[arrayPos] = (float)PyFloat_AsDouble(value);
        var->indicator[arrayPos] = (int32_t)sizeof(float);
        return 0;
    }

    if (PyDateTime_Check(value)) {
        YapiTimestamp *timeStamp = (YapiTimestamp*)var->data;
        int year = PyDateTime_GET_YEAR(value);
        int month = PyDateTime_GET_MONTH(value);
        int day = PyDateTime_GET_DAY(value);
        int hour = PyDateTime_DATE_GET_HOUR(value);
        int minute = PyDateTime_DATE_GET_MINUTE(value);
        int second = PyDateTime_DATE_GET_SECOND(value);
        int microSec = PyDateTime_DATE_GET_MICROSECOND(value);
        if (yapiTimestampSetTimestamp(timeStamp + arrayPos, (int16_t)year, (uint8_t)month, (uint8_t)day, (uint8_t)hour,
            (uint8_t)minute, (uint8_t)second, (uint32_t)microSec) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
        var->indicator[arrayPos] = (int32_t)sizeof(YapiTimestamp);
        
        return 0;
    }

    if (PyDate_Check(value)) {
        YapiDate *date = (YapiDate*)var->data;
        int year = PyDateTime_GET_YEAR(value);
        int month = PyDateTime_GET_MONTH(value);
        int day = PyDateTime_GET_DAY(value);
        if (yapiDateSetDate(date + arrayPos, (int16_t)year, (uint8_t)month, (uint8_t)day) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
        var->indicator[arrayPos] = (int32_t)sizeof(YapiDate);

        return 0;
    }

    if (PyTime_Check(value)) {
        YapiShortTime *shortTime = (YapiShortTime*)var->data;
        int hour = PyDateTime_TIME_GET_HOUR(value);
        int minute = PyDateTime_TIME_GET_MINUTE(value);
        int second = PyDateTime_TIME_GET_SECOND(value);
        int microSec = PyDateTime_TIME_GET_MICROSECOND(value);
        if (yapiShortTimeSetShortTime(shortTime + arrayPos, (uint8_t)hour, (uint8_t)minute,
            (uint8_t)second, (uint32_t)microSec) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
        var->indicator[arrayPos] = (int32_t)sizeof(YapiShortTime);

        return 0;
    }

    if (PyObject_TypeCheck(value, anpPyTypeDecimal)) {
        return anpVarSetDecimal(var, arrayPos, value);
    }

    if (PyDelta_Check(value)) {
        return anpVarSetPyDelta(var, arrayPos, value);
    }

    if (PyList_Check(value) || PyDict_Check(value)) {
        Py_ssize_t enCodeStrSize = 0;
        const char* bindStr = anpVarGetJsonStringAndSize(value, &enCodeStrSize);
        if (bindStr == NULL) {
            return -1;
        }

        if (anpVarIsLobType(var)) {
            if (yapiLobWrite(hConn, *(YapiLobLocator**)(var->data + var->dataOffset), NULL, (uint8_t*)bindStr,
                             (uint64_t)enCodeStrSize) != YAPI_SUCCESS) {
                return -1;
            }
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
            var->dataOffset += sizeof(YapiLobLocator*);
        } else {
            uint32_t costSize = (uint32_t)(enCodeStrSize + 1);
            if (costSize > var->size) {
                if (adjustAnpVarBuffSize(var, costSize) < 0) {
                    return -1;
                }
            }

            strcpy(var->data + var->dataOffset, bindStr);
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
            var->dataOffset += costSize;
            var->data[var->dataOffset - 1] = '\0';
        }
        return 0;
    }

    if (PyObject_TypeCheck(value, anpPyTypeArray)) {
        // For VECTOR type, we need to convert array to YapiVector
        // Reuse existing vector if it exists
        YapiVector* vector = *((YapiVector**)(var->data + arrayPos * sizeof(YapiVector*)));
        if (vector == NULL) {
            // Allocate new vector if it doesn't exist
            if (yapiDescAlloc2(anpEnv, (void**)&vector, YAPI_DESC_VECTOR) != YAPI_SUCCESS) {
                return anpRaiseAndReturnIntException();
            }
            // Store the vector pointer in data
            *((YapiVector**)(var->data + arrayPos * sizeof(YapiVector*))) = vector;
        }
        
        // Get array type code and buffer info
        PyObject* typecodeObj = PyObject_GetAttrString(value, "typecode");
        if (!typecodeObj) {
            return -1;
        }
        
        const char* typecode = PyUnicode_AsUTF8(typecodeObj);
        Py_DECREF(typecodeObj);
        
        if (!typecode) {
            return -1;
        }
        
        // Determine YapiVectorFormat based on array typecode
        YapiVectorFormat format;
        uint16_t dim = 0;
        
        if (strcmp(typecode, "f") == 0) {  // float
            format = YAPI_VECTOR_FORMAT_FLOAT32;
            dim = (uint16_t)PyObject_Length(value);
        } else if (strcmp(typecode, "d") == 0) {  // double
            format = YAPI_VECTOR_FORMAT_FLOAT64;
            dim = (uint16_t)PyObject_Length(value);
        } else {
            anpRaiseExceptionFromString(anpNotSupportedException, "Unsupported array type for VECTOR");
            return -1;
        }
        
        // Store the format in var for later use in anpVarToPython
        var->typeData.vectorFormat = format;
        
        // Get array buffer
        PyObject* bufferObj = PyObject_CallMethod(value, "tobytes", NULL);
        if (!bufferObj) {
            return -1;
        }
        
        if (!PyBytes_Check(bufferObj)) {
            Py_DECREF(bufferObj);
            anpRaiseExceptionFromString(anpProgrammingErrorException, "Failed to get bytes from array");
            return -1;
        }
        
        uint8_t* arrayBuffer = (uint8_t*)PyBytes_AS_STRING(bufferObj);
        uint32_t bufferLen = (uint32_t)PyBytes_GET_SIZE(bufferObj);
        
        // Convert array to YapiVector
        if (yapiVectorFromArray(vector, format, dim, arrayBuffer, bufferLen, 0) != YAPI_SUCCESS) {
            Py_DECREF(bufferObj);
            return anpRaiseAndReturnIntException();
        }
        
        Py_DECREF(bufferObj);

        // Store the vector pointer in data
        *((YapiVector**)(var->data + arrayPos * sizeof(YapiVector*))) = vector;
        var->indicator[arrayPos] = (int32_t)sizeof(YapiVector*);
        return 0;
    }

    anpRaiseExceptionFromString(anpNotSupportedException, "not support type");
    return -1;
}

int anpGetSize(PyObject * value)
{
    if (value == Py_None) {
        return 1;
    }

    if (PyBool_Check(value)) {
        return 1;
    }

    if (PyUnicode_Check(value)) {
        Py_ssize_t size = 0;
        PyUnicode_AsUTF8AndSize(value, &size);
        return (int)(size + 1);
    }

    if (PyBytes_Check(value)) {
        return (int)PyBytes_GET_SIZE(value) + 1;
    }

    if (PyLong_Check(value)) {
        int overflow = 0;
        PyLong_AsLongLongAndOverflow(value, &overflow);
        if (overflow) {
            return sizeof(YapiNumber);
        }
        return 8;
    }

    if (PyFloat_Check(value)) {
        return 8;
    }

    if (PyDateTime_Check(value)) {
        return sizeof(YapiTimestamp);
    }

    if (PyDate_Check(value)) {
        return sizeof(YapiDate);
    }

    if (PyTime_Check(value)) {
        return sizeof(YapiShortTime);
    }

    if (PyObject_TypeCheck(value, anpPyTypeDecimal)) {
        PyObject *strValue = PyObject_Str(value);
        if (!strValue) {
            anpRaiseExceptionFromString(anpProgrammingErrorException, "get decimal string failed");
            return -1;
        }

        Py_ssize_t enCodeStrSize = 0;
        const char* bindStr = PyUnicode_AsUTF8AndSize(strValue, &enCodeStrSize);
        if (bindStr == NULL) {
            Py_DECREF(strValue);
            return -1;
        }

        Py_DECREF(strValue);
        return (int)(enCodeStrSize + 1);
    }

    if (PyDelta_Check(value)) {
        return sizeof(YapiDSInterval);
    }

    if (PyList_Check(value) || PyDict_Check(value)) {
        Py_ssize_t enCodeStrSize = 0;
        const char* result = anpVarGetJsonStringAndSize(value, &enCodeStrSize);
        if (!result) {
            return -1;
        }
        return (int)(enCodeStrSize + 1);
    }

    if (PyObject_TypeCheck(value, anpPyTypeArray)) {
        // For VECTOR type, we store pointers to YapiVector objects, not the struct itself
        return sizeof(YapiVector*);
    }

    return 0;
}

YapiType anpGetType(PyObject * value)
{
    if (value == Py_None) {
        return 0;
    }

    if (PyBool_Check(value)) {
        return YAPI_TYPE_BOOL;
    }

    if (PyUnicode_Check(value)) {
        return YAPI_TYPE_VARCHAR;
    }

    if (PyBytes_Check(value)) {
        return YAPI_TYPE_BINARY;
    }

    if (PyLong_Check(value)) {
        int overflow = 0;
        PyLong_AsLongLongAndOverflow(value, &overflow);
        if (overflow) {
            return YAPI_TYPE_NUMBER;
        }
        return YAPI_TYPE_BIGINT;
    }

    if (PyFloat_Check(value)) {
        return YAPI_TYPE_DOUBLE;
    }

    if (PyDateTime_Check(value)) {
        return YAPI_TYPE_TIMESTAMP;
    }

    if (PyDate_Check(value)) {
        return YAPI_TYPE_DATE;
    }

    if (PyTime_Check(value)) {
        return YAPI_TYPE_SHORTTIME;
    }

    if (PyObject_TypeCheck(value, anpPyTypeDecimal)) {
        return YAPI_TYPE_VARCHAR;
    }

    if (PyDelta_Check(value)) {
        return YAPI_TYPE_DS_INTERVAL;
    }

    if (PyList_Check(value) || PyDict_Check(value)) {
        return YAPI_TYPE_VARCHAR;
    }

    if (PyObject_TypeCheck(value, anpPyTypeArray)) {
        return YAPI_TYPE_VECTOR;
    }

    return 0;
}

void anpAdjustVarTypeSize(PyObject* value, uint32_t* size,YapiType* type)
{
    *type = anpGetType(value);
    *size = (Py_ssize_t)anpGetSize(value);
}

AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements, bool bindIn)
{
    int isArray = 0;
    Py_ssize_t size = 0;
    Py_ssize_t i = 0;
    Py_ssize_t tempSize = 0;
    YapiType type = 0;
    char message[250];

    if (PyList_Check(value)) {
        isArray = 1;
        for (i = 0; i < PyList_GET_SIZE(value); i++) {
            PyObject * obj = PyList_GET_ITEM(value, i);

            YapiType tmpType = anpGetType(obj);
            if (type == 0) {
                type = tmpType;
            } else if (type != tmpType) {
                snprintf(message, sizeof(message),
                         "element %u value is not the same type as previous "
                         "elements", (unsigned) i);
                return (AnpVar *)anpRaiseExceptionFromString(anpNotSupportedException, message);
            }
            tempSize = anpGetSize(obj);
            if (tempSize > size) {
                size = tempSize;
            }
        }
    } else {
        size = anpGetSize(value);
        if (size < 0) {
            return NULL;
        }
        type = anpGetType(value);
    }

    VarAssist assist = {.isArray = isArray, .numElements = numElements,
        .size = size, .type = type, .bindIn = bindIn};
    return anpNewVar(cursor, &assist);
}

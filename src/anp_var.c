#include "anp_var.h"
#include "datetime.h"
#include "anp_exception.h"

PyTypeObject *anpPyTypeDate;
PyTypeObject *anpPyTypeDateTime;

static void anpVarFree(AnpVar *var)
{
    if(var->data) {
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
    return NULL;
}

static PyObject * anpVarGetType(AnpVar *var, void *unused)
{
    return PyLong_FromLong(var->dbType);
}

static PyGetSetDef anpCalcMembers[] = {
    { "type",           (getter)anpVarGetType,                   0, 0, 0 },
    { NULL }
};

PyTypeObject anchorPyTypeVar = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "yaspy.Var",
    .tp_basicsize = sizeof(AnpVar),
    .tp_dealloc = (destructor)anpVarFree,
    .tp_repr = (reprfunc)anpVarRepr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = anpCalcMembers};

static PyTypeObject * anpPyTypeDecimal;

YapiResult anpInitDecimal()
{
    PyObject *module;

    PyDateTime_IMPORT;
    if (PyErr_Occurred()) {
        return YAPI_ERROR;
    }
    anpPyTypeDate = PyDateTimeAPI->DateType;
    anpPyTypeDateTime = PyDateTimeAPI->DateTimeType;

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

YapiResult anpRegisteVarObject(PyObject* module)
{
    PyType_Ready(&anchorPyTypeVar);

    Py_INCREF(&anchorPyTypeVar);
    if (PyModule_AddObject(module, "Var", (PyObject*) &anchorPyTypeVar) < 0) {
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

AnpVar* anpNewVar(AnpCursor* cursor, Py_ssize_t numElements, YapiType type, Py_ssize_t size, bool isArray)
{
    AnpVar* var = (AnpVar*) anchorPyTypeVar.tp_alloc(&anchorPyTypeVar, 0);
    if (var == NULL) {
        return NULL;
    }

    Py_INCREF(cursor->connection);
    var->connection = cursor->connection;

    var->size = (uint32_t)size;
    var->elements = (uint32_t)numElements;
    var->isArray = isArray;
    var->bufferSize = var->size * var->elements;
    var->dbType = type;
    if(type == YAPI_TYPE_NUMBER || type == YAPI_TYPE_ROWID){
        var->transType = YAPI_TYPE_VARCHAR;
    } else {
        var->transType = type;
    }

    if(type != YAPI_TYPE_CLOB && type != YAPI_TYPE_BLOB && size > 8001)
    {
        var->transType = type == YAPI_TYPE_BINARY ? YAPI_TYPE_BLOB : YAPI_TYPE_CLOB;
        var->dbType = var->transType;
    }
    
    if (var->transType == YAPI_TYPE_CLOB || var->transType == YAPI_TYPE_BLOB)
    {
        var->size = -1;
        YapiLobLocator* loc;
        if (yapiLobDescAlloc((YapiConnect*)var->connection->hConn, var->transType, (void**)&loc) != YAPI_SUCCESS)
        {
            return (AnpVar*)anpRaiseAndReturnNullException();
        }
        if (!cursor->fetchVariables)
        {
            if (yapiLobCreateTemporary((YapiConnect*)var->connection->hConn, loc) != YAPI_SUCCESS)
            {
                return (AnpVar*)anpRaiseAndReturnNullException();
            }
        }
        var->data = (char*)loc;
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

static YapiResult anpLobBytes2Str(uint8_t* buf, uint64_t len)
{
    len *= 2;
    char* strBuf = PyMem_Malloc(len);
    if(strBuf == NULL)
    {
        return YAPI_ERROR;
    }
    uint32_t pos = 0;
    uint32_t index;
    uint8_t  value;
    for (uint32_t i = 0; i < len; i++) {
        index = i / 2;
        value = pos % 8 ? buf[index] & 0xF : (buf[index] & 0xF0) >> 4;
        strBuf[i] = value >= 10 ? value - 10 + 'A' : value + '0';
        pos += 4;
    }

    memcpy(buf, strBuf, len);
    buf[len] = '\0';
    PyMem_Free(strBuf);
    return YAPI_SUCCESS;
}

static PyObject* anpGetLobData(YapiConnect* hConn, YapiType type, char* data)
{
    YapiLobLocator* loc = (YapiLobLocator*)data;
    uint64_t length;
    yapiLobGetLength(hConn, loc, &length);
    if (type == YAPI_TYPE_BLOB) {
        length *= 2;
    }
    
    char* readBuf = PyMem_Malloc(length + 1);
    if (readBuf == NULL) {
        return (PyObject*)PyErr_NoMemory();
    }
    readBuf[length] = '\0';
    
    if (yapiLobRead(hConn, loc, &length, (uint8_t*)readBuf, length) != YAPI_SUCCESS) {
        PyMem_Free(readBuf);
        readBuf = NULL;
        return anpRaiseAndReturnNullException();
    }
    
    if (type == YAPI_TYPE_BLOB) {
        if (anpLobBytes2Str((uint8_t*)readBuf, length) != YAPI_SUCCESS) {
            PyMem_Free(readBuf);
            readBuf = NULL;
            return anpRaiseAndReturnNullException();
        }
    }
    PyObject* var =  PyUnicode_FromString((char*)readBuf);
    PyMem_Free(readBuf);
    readBuf = NULL;
    return var;
}

static PyObject *anpVarToPython(YapiConnect* hConn, YapiType type, char* data)
{
    char message[120];
    PyObject* result;
    YapiDate *date;
    YapiShortTime* time;
    YapiTimestamp *timestamp;
    YapiDateStruct ds;
    
    switch (type) {
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
        case YAPI_TYPE_NUMBER: {
            PyObject* stringObj = PyUnicode_Decode(data, strlen(data), NULL, NULL);
            result = PyObject_CallFunctionObjArgs((PyObject*)anpPyTypeDecimal, stringObj, NULL);
            Py_DECREF(stringObj);
            break;
        }
        case YAPI_TYPE_DATE:
            date = (YapiDate *)data;
            yapiGetDateStruct(*date, &ds);
            result =  PyDateTime_FromDateAndTime(ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second, ds.fraction/1000);
            break;
        case YAPI_TYPE_SHORTTIME:
            time = (YapiShortTime *)data;
            yapiGetDateStruct(*time, &ds);
            result = PyTime_FromTime(ds.hour, ds.minute, ds.second, ds.fraction/1000);
            break;
        case YAPI_TYPE_TIMESTAMP:
        case YAPI_TYPE_TIMESTAMP_TZ:
        case YAPI_TYPE_TIMESTAMP_LTZ:
            timestamp = (YapiTimestamp*)data;
            yapiGetDateStruct(timestamp->stamp, &ds);
            result =  PyDateTime_FromDateAndTime(ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second, ds.fraction/1000);
            break;
 
        case YAPI_TYPE_CHAR:
        case YAPI_TYPE_NCHAR:
        case YAPI_TYPE_VARCHAR:
        case YAPI_TYPE_NVARCHAR:
        case YAPI_TYPE_ROWID:
            result = PyUnicode_FromString(data);
            break;
        case YAPI_TYPE_CLOB:
        case YAPI_TYPE_BLOB:
            result = anpGetLobData(hConn, type, data);
            break;
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
    return anpVarToPython(hConn, var->dbType, var->data + pos * var->size);
}

int anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos)
{
    if (name) {
        anpRaiseExceptionFromString(anpNotSupportedException, "not support bind by name");
        return -1;
    }

    if (var->dbType == YAPI_TYPE_BLOB || var->dbType == YAPI_TYPE_CLOB) {
        if (yapiBindParameter(cursor->hStmt, pos, YAPI_PARAM_INPUT, var->dbType, &var->data, var->size, var->bufferSize, var->indicator) !=
        YAPI_SUCCESS) {
            return -1;
        }
        return 0;
    }

    if (yapiBindParameter(cursor->hStmt, pos, YAPI_PARAM_INPUT, var->dbType, var->data, var->size, var->bufferSize, var->indicator) !=
        YAPI_SUCCESS) {
        return -1;
    }
    return 0;
}

int anpVarSetValue(YapiConnect* hConn, AnpVar* var, uint32_t arrayPos, PyObject* value)
{
    if (value == Py_None){
        var->indicator[arrayPos] = YAPI_NULL_DATA;
    }
    if (PyBool_Check(value)) {
        bool* b = (bool *)var->data;
        b[arrayPos] = PyObject_IsTrue(value);
        var->indicator[arrayPos] = (int32_t)sizeof(bool);
        return 0;
    }
    if (PyUnicode_Check(value)) {
        PyObject* tmp = PyUnicode_AsEncodedString(value, "utf8", NULL);
        if (tmp == NULL) {
            return -1;
        }

        if (var->transType == YAPI_TYPE_CLOB || var->transType == YAPI_TYPE_BLOB)
        {
             if (yapiLobWrite(hConn, (YapiLobLocator*)var->data, NULL, 
                        (uint8_t*)PyBytes_AS_STRING(tmp), (int)PyUnicode_GET_LENGTH(value)) != YAPI_SUCCESS)
             {
                return -1;
             }
             return 0;
        } else {
            strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(tmp));
            var->indicator[arrayPos] = (int)PyUnicode_GET_LENGTH(value);
        }
        return 0;
    }
    if (PyBytes_Check(value)) {
        if (var->transType == YAPI_TYPE_BLOB || var->transType == YAPI_TYPE_CLOB)
        {
             if (yapiLobWrite(hConn, (YapiLobLocator*)var->data, NULL, 
                        (uint8_t*)value, (int)PyBytes_GET_SIZE(value)) != YAPI_SUCCESS)
             {
                return -1;
             }
             var->indicator[arrayPos] = NULL;
             return 0;
        } else {
            strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(value));
            var->indicator[arrayPos] = (int)PyBytes_GET_SIZE(value);
        }
        return 0;
    }
    if (PyLong_Check(value)) {
        int32_t *iv = (int32_t *)var->data;
        iv[arrayPos] = PyLong_AsLong(value);
        var->indicator[arrayPos] = (int32_t)sizeof(int64_t);
        return 0;
    }
    if (PyFloat_Check(value)) {
        double *dv = (double *)var->data;
        dv[arrayPos] = PyFloat_AsDouble(value);
        var->indicator[arrayPos] = (int32_t)sizeof(double);
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
        return PyUnicode_GET_LENGTH(value) + 1;
    }
    if (PyBytes_Check(value)) {
        return (int)PyBytes_GET_SIZE(value) + 1;
    }
    if (PyLong_Check(value)) {
        return 4;
    }
    if (PyFloat_Check(value)) {
        return 8;
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
        return YAPI_TYPE_INTEGER;
    }
    if (PyFloat_Check(value)) {
        return YAPI_TYPE_DOUBLE;
    }

    return 0;
}

void anpAdjustVarTypeSize(PyObject* value, uint32_t* size,YapiType* type)
{
    *type = anpGetType(value);
    *size = (Py_ssize_t)anpGetSize(value);
}

AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements)
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
        type = anpGetType(value);
    }
    return anpNewVar(cursor, numElements, type, size, isArray);
}

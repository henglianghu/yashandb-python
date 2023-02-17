#include "anp_var.h"
#include "datetime.h"
#include "anp_exception.h"

PyTypeObject *anpPyTypeDate;
PyTypeObject *anpPyTypeDateTime;

static void anpVarFree(AnpVar *var)
{
    if(var->data) {
        if (var->transType == YAPI_TYPE_CLOB || var->transType == YAPI_TYPE_BLOB) {
            if (yapiLobDescFree(var->data, var->transType) != YAPI_SUCCESS) {
                (void)anpRaiseAndReturnNullException();
            }
        } else {
            PyMem_Free(var->data);
        }
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

AnpVar* anpNewVar(AnpCursor* cursor, VarAssist *assist, bool bindIn)
{
    AnpVar* var = (AnpVar*) anchorPyTypeVar.tp_alloc(&anchorPyTypeVar, 0);
    if (var == NULL) {
        return NULL;
    }

    Py_INCREF(cursor->connection);
    var->connection = cursor->connection;

    YapiType type = assist->type;
    Py_ssize_t size = assist->size;

    var->size = (uint32_t)size;
    var->elements = (uint32_t)assist->numElements;
    var->isArray = assist->isArray;
    var->bufferSize = var->size * var->elements;
    var->dbType = type;
    if(type == YAPI_TYPE_NUMBER || type == YAPI_TYPE_BIT || type == YAPI_TYPE_ROWID || 
        type == YAPI_TYPE_YM_INTERVAL) {
        var->transType = YAPI_TYPE_VARCHAR;
    } else {
        var->transType = type;
    }

    if (bindIn && (size > CONVERT_TO_LOB_SIZE)) {
        if ((type >= YAPI_TYPE_CHAR) && (type <= YAPI_TYPE_NVARCHAR)) {
            var->transType = YAPI_TYPE_CLOB;
            var->dbType = YAPI_TYPE_CLOB;
        } else if(type == YAPI_TYPE_BINARY) {
            var->transType = YAPI_TYPE_BLOB;
            var->dbType = YAPI_TYPE_BLOB;
        }
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

static PyObject* anpGetLobData(YapiConnect* hConn, YapiType type, char* data)
{
    YapiLobLocator* loc = (YapiLobLocator*)data;
    uint64_t length;
    yapiLobGetLength(hConn, loc, &length);
    if (length == 0) {
        Py_RETURN_NONE;
    }
    
    if (type == YAPI_TYPE_CLOB) {
        length = length * 4;
    }

    char* readBuf = PyMem_Malloc(length + 1);
    if (readBuf == NULL) {
        return (PyObject*)PyErr_NoMemory();
    }
    
    if (yapiLobRead(hConn, loc, &length, (uint8_t*)readBuf, length) != YAPI_SUCCESS) {
        PyMem_Free(readBuf);
        readBuf = NULL;
        return anpRaiseAndReturnNullException();
    }
    
    PyObject* var = NULL;
    if (type == YAPI_TYPE_BLOB) {
        var = PyBytes_FromStringAndSize(readBuf, (Py_ssize_t)length);
    } else {
        readBuf[length] = '\0';
        var =  PyUnicode_FromString((char*)readBuf);
    }

    PyMem_Free(readBuf);
    readBuf = NULL;
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
        case YAPI_TYPE_NUMBER: {
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
        case YAPI_TYPE_NCHAR:
        case YAPI_TYPE_VARCHAR:
        case YAPI_TYPE_NVARCHAR:
        case YAPI_TYPE_ROWID:
        case YAPI_TYPE_BIT:
        case YAPI_TYPE_YM_INTERVAL:
            result = PyUnicode_FromString(data);
            break;

        case YAPI_TYPE_DS_INTERVAL:
            result = anpVarToPyDelta(data);
            break;

        case YAPI_TYPE_BINARY:
            result = PyBytes_FromStringAndSize(data, (Py_ssize_t)var->indicator[pos]);
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
    return anpVarToPython(hConn, var, pos);
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
            return anpRaiseAndReturnIntException();
        }
        return 0;
    }

    if (yapiBindParameter(cursor->hStmt, pos, YAPI_PARAM_INPUT, var->dbType, var->data, var->size, var->bufferSize, var->indicator) !=
        YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }
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

    strcpy(var->data + var->size*arrayPos, bindStr);
    var->indicator[arrayPos] = (int32_t)enCodeStrSize;
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

        if (var->transType == YAPI_TYPE_CLOB || var->transType == YAPI_TYPE_BLOB) {
             if (yapiLobWrite(hConn, (YapiLobLocator*)var->data, NULL, 
                 (uint8_t*)bindStr, (uint64_t)enCodeStrSize) != YAPI_SUCCESS) {
                return -1;
             }
             return 0;
        } else {
            strcpy(var->data + var->size*arrayPos, bindStr);
            var->indicator[arrayPos] = (int32_t)enCodeStrSize;
        }
        return 0;
    }

    if (PyBytes_Check(value)) {
        if (var->transType == YAPI_TYPE_BLOB || var->transType == YAPI_TYPE_CLOB) {
             if (yapiLobWrite(hConn, (YapiLobLocator*)var->data, NULL, 
                (uint8_t*)value, (int)PyBytes_GET_SIZE(value)) != YAPI_SUCCESS) {
                return -1;
             }
             var->indicator[arrayPos] = 0;
             return 0;
        }

        Py_ssize_t byteSize = PyBytes_GET_SIZE(value);
        if (byteSize == 0) {
            var->indicator[arrayPos] = YAPI_NULL_DATA;
            return 0;   
        }

        strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(value));
        var->indicator[arrayPos] = (int)byteSize;
        return 0;
    }
    
    if (PyLong_Check(value)) {
        int64_t *iv = (int64_t *)var->data;
        int64_t bindValue = PyLong_AsLongLong(value);
        PyObject *pyError = PyErr_Occurred();
        if ((bindValue == -1L) && (pyError != NULL)) {
            PyErr_SetString(pyError, "fail to get long long value from PyObject");
            return -1;
        }
        
        iv[arrayPos] = bindValue;
        var->indicator[arrayPos] = (int32_t)sizeof(int64_t);
        return 0;
    }

    if (PyFloat_Check(value)) {
        double *dv = (double *)var->data;
        dv[arrayPos] = PyFloat_AsDouble(value);
        var->indicator[arrayPos] = (int32_t)sizeof(double);
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
        .size = size, .type = type};
    return anpNewVar(cursor, &assist, bindIn);
}

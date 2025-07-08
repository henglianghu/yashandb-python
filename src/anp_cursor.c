#include "anp_cursor.h"
#include "structmember.h"
#include "anp_exception.h"
#include "anp_var.h"
#include "anp_api_type.h"


static int anpCursorInit(AnpCursor* cursor, PyObject* arguments, PyObject* keywordArgs)
{
    static char*   keywords[] = {"connection", "scrollable", NULL};
    PyObject*      scrollObject;
    AnpConnection* connection;

    scrollObject = NULL;
    if (!PyArg_ParseTupleAndKeywords(arguments, keywordArgs, "O!|O", keywords, &anchorPyTypeConnection, &connection,
                                     &scrollObject)) {
        return -1;
    }

    Py_INCREF(connection);
    cursor->arraySize = 100;
    cursor->connection = connection;
    cursor->isOpen = 1;
    cursor->isFail = 0;

    return 0;
}

static PyObject* anpCursorNew(PyTypeObject* type, PyObject* args, PyObject* keywordArgs)
{
    return type->tp_alloc(type, 0);
}

static bool anpCursorIsOpen(AnpCursor* cursor)
{
    if (!cursor->isOpen) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not open");
        return YAPI_FALSE;
    }
    return anpConnectionIsConnected(cursor->connection);
}

uint32_t anpGetDisplaySize(YapiColumnDesc* desc)
{
    uint32_t displaySize;
    switch (desc->type) {
        case YAPI_TYPE_CHAR:
        case YAPI_TYPE_VARCHAR:
        case YAPI_TYPE_NCHAR:
        case YAPI_TYPE_NVARCHAR:
        case YAPI_TYPE_CLOB:
            displaySize = codSizeAlign4(desc->size) + 1;
            break;

        case YAPI_TYPE_BINARY:
        case YAPI_TYPE_BLOB:
            displaySize = codSizeAlign4(desc->size * 2);
            break;

        case YAPI_TYPE_DATE:
            displaySize = 32;
            break;

        case YAPI_TYPE_TIMESTAMP:
        case YAPI_TYPE_TIMESTAMP_TZ:
        case YAPI_TYPE_TIMESTAMP_LTZ:
            displaySize = 64;
            break;

        case YAPI_TYPE_TINYINT:
            displaySize = 5;
            break;

        case YAPI_TYPE_SMALLINT:
            displaySize = 8;
            break;

        case YAPI_TYPE_INTEGER:
            displaySize = 12;
            break;

        case YAPI_TYPE_BIGINT:
        case YAPI_TYPE_FLOAT:
        case YAPI_TYPE_DOUBLE:
            displaySize = 21;
            break;

        case YAPI_TYPE_NUMBER:
        case YAPI_TYPE_NUMBER_FLOAT:
            displaySize = codSizeAlign4(desc->precision + 8);
            break;

        case YAPI_TYPE_BIT:
            displaySize = desc->size + 1;
            break;

        case YAPI_TYPE_ROWID:
            displaySize = 24;
            break;

        default:
            displaySize = 0;
            break;
    }
    return displaySize;
}

static PyObject* anpCursorItemDescription(AnpCursor* cursor, uint32_t pos)
{
    YapiColumnDesc desc;
    if (yapiDescribeCol2(cursor->hStmt, pos, &desc) != YAPI_SUCCESS) {
        return NULL;
    }

    PyObject* tuple = PyTuple_New(7);
    if (tuple == NULL) {
        return NULL;
    }
    int itemDisplaySize = anpGetDisplaySize(&desc);

    PyTuple_SET_ITEM(tuple, 0, PyUnicode_Decode(desc.name, strlen(desc.name), NULL, NULL));
    PyTuple_SET_ITEM(tuple, 1, PyLong_FromLong(desc.type));
    if (itemDisplaySize == 0) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 2, Py_None);
    } else {
        PyTuple_SET_ITEM(tuple, 2, PyLong_FromLong(itemDisplaySize));
    }

    if (desc.size == 0) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 3, Py_None);
    } else {
        PyTuple_SET_ITEM(tuple, 3, PyLong_FromLong(desc.size));
    }

    if (desc.precision != 255) {
        PyTuple_SET_ITEM(tuple, 4, PyLong_FromLong(desc.precision));
    } else {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 4, Py_None);
    }

    if (desc.scale != -128) {
        PyTuple_SET_ITEM(tuple, 5, PyLong_FromLong(desc.scale));
    } else {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 5, Py_None);
    }

    PyTuple_SET_ITEM(tuple, 6, PyLong_FromLong(desc.nullable));

    return tuple;
}

static PyObject* anpCursorGetDescription(AnpCursor* cursor, void* unused)
{
    int16_t queryColumns;

    if (!anpCursorIsOpen(cursor)) {
        return NULL;
    }

    if (cursor->hStmt == NULL) {
        Py_RETURN_NONE;
    }
    if (yapiNumResultCols(cursor->hStmt, &queryColumns) != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    if (queryColumns == 0) {
        Py_RETURN_NONE;
    }

    PyObject* results = PyList_New(queryColumns);
    if (results == NULL) {
        return NULL;
    }

    for (int16_t i = 0; i < queryColumns; i++) {
        PyObject* tuple = anpCursorItemDescription(cursor, i);
        if (tuple != NULL) {
            PyList_SET_ITEM(results, i, tuple);
        } else {
            Py_DECREF(results);
            return NULL;
        }
    }

    return results;
}

static YapiResult anpCursorSetBindVariableHelper(AnpCursor* cursor, unsigned numElements, unsigned arrayPos,
    PyObject* value, AnpVar* oldVar, AnpVar** newVar)
{
    AnpVar* varToSet;
    *newVar = NULL;

    bool isValueVar = anpCheckVar(value);

    if (oldVar != NULL) {
        if (isValueVar) {
            if ((PyObject*)oldVar != value) {
                Py_INCREF(value);
                *newVar = (AnpVar*)value;
            }
            return YAPI_SUCCESS;
        }

        uint32_t bindCostSize;
        YapiType bindType;
        bool needCreateVar = false;
        anpAdjustVarTypeSize(value, &bindCostSize, &bindType);
        if (bindCostSize > CONVERT_TO_LOB_SIZE) {
            needCreateVar = true;
        }

        if ((numElements == 1) && (bindType != oldVar->dbType)) {
            needCreateVar = true;
        }

        varToSet = oldVar;
        if ((numElements > oldVar->elements) || needCreateVar) {
            oldVar->size = bindCostSize;
            oldVar->dbType = bindType;
            VarAssist assist = { .isArray = oldVar->isArray, .numElements = numElements,
                .size = oldVar->size, .type = oldVar->dbType, .bindIn = true};
            *newVar = anpNewVar(cursor, &assist);
            if (!*newVar) {
                return YAPI_ERROR;
            }
            varToSet = *newVar;
        }

        if (anpVarSetValue(cursor->connection->hConn, varToSet, arrayPos, value) < 0) {
            return YAPI_ERROR;
        }

        return YAPI_SUCCESS;
    }

    if (isValueVar) {
        Py_INCREF(value);
        *newVar = (AnpVar*)value;
    } else {
        *newVar = anpVarNewByValue(cursor, value, numElements, true);
        if (*newVar == NULL) {
            return YAPI_ERROR;
        }

        if (anpVarSetValue(cursor->connection->hConn, *newVar, arrayPos, value) < 0) {
            Py_CLEAR(*newVar);
            return YAPI_ERROR;
        }
    }

    return YAPI_SUCCESS;
}

YapiResult anpCursorSetBindByPos(AnpCursor* cursor, PyObject* parameters, unsigned numElements, unsigned arrayPos)
{
    Py_ssize_t temp = PySequence_Size(parameters);
    if (temp < 0) {
        return YAPI_ERROR;
    }

    uint32_t numParams = (uint32_t)temp;
    uint32_t origNumParams = 0;
    if (cursor->bindVariables) {
        uint32_t origBoundByPos = PyList_Check(cursor->bindVariables);
        if (!origBoundByPos) {
            anpRaiseExceptionFromString(anpProgrammingErrorException,
                                        "positional and named binds cannot be intermixed");
            return YAPI_ERROR;
        }
        origNumParams = (uint32_t)PyList_GET_SIZE(cursor->bindVariables);
    } else {
        cursor->bindVariables = PyList_New(numParams);
        if (!cursor->bindVariables) {
            return YAPI_ERROR;
        }
    }

    AnpVar* newVar = NULL;
    for (uint32_t i = 0; i < numParams; i++) {
        PyObject* origVar;
        PyObject* paramValue = PySequence_GetItem(parameters, i);
        if (paramValue == NULL) {
            return YAPI_ERROR;
        }
        Py_DECREF(paramValue);
        if (i < origNumParams) {
            origVar = PyList_GET_ITEM(cursor->bindVariables, i);
            if (origVar == Py_None) {
                origVar = NULL;
            }
        } else {
            origVar = NULL;
        }

        if (anpCursorSetBindVariableHelper(cursor, numElements, arrayPos, paramValue, (AnpVar*)origVar, &newVar) < 0) {
            return YAPI_ERROR;
        }
        if (newVar != NULL) {
            if (i < (uint32_t)PyList_GET_SIZE(cursor->bindVariables)) {
                if (PyList_SetItem(cursor->bindVariables, i, (PyObject*)newVar) < 0) {
                    Py_DECREF(newVar);
                    return YAPI_ERROR;
                }
            } else {
                if (PyList_Append(cursor->bindVariables, (PyObject*)newVar) < 0) {
                    Py_DECREF(newVar);
                    return YAPI_ERROR;
                }
                Py_DECREF(newVar);
            }
        }
    }
    return YAPI_SUCCESS;
}

YapiResult anpCursorSetBindByName(AnpCursor* cursor, PyObject* parameters, unsigned numElements, unsigned arrayPos)
{
    if (cursor->bindVariables) {
        uint32_t origBoundByPos = PyList_Check(cursor->bindVariables);
        if (origBoundByPos) {
            anpRaiseExceptionFromString(anpProgrammingErrorException, "positional and named binds cannot be intermixed");
            return YAPI_ERROR;
        }
    } else {
        cursor->bindVariables = PyDict_New();
        if (!cursor->bindVariables) {
            return YAPI_ERROR;
        }
    }

    AnpVar* newVar;
    Py_ssize_t  pos = 0;
    PyObject*   key;
    PyObject*   value;

    while (PyDict_Next(parameters, &pos, &key, &value)) {
        PyObject* oldVar = PyDict_GetItem(cursor->bindVariables, key);
        if (anpCursorSetBindVariableHelper(cursor, numElements, arrayPos, value, (AnpVar*)oldVar, &newVar) < 0) {
            return YAPI_ERROR;
        }
        if (newVar != NULL) {
            if (PyDict_SetItem(cursor->bindVariables, key, (PyObject*)newVar) < 0) {
                Py_DECREF(newVar);
                return YAPI_ERROR;
            }
            Py_DECREF(newVar);
        }
    }
    return 0;
}

YapiResult anpCursorSetBindVariables(AnpCursor* cursor, PyObject* parameters, uint32_t numElements, unsigned arrayPos)
{
    if (PySequence_Check(parameters)) {
        return anpCursorSetBindByPos(cursor, parameters, numElements, arrayPos);
    } else {
        return anpCursorSetBindByName(cursor, parameters, numElements, arrayPos);
    }
}

void anpGetColumnSize(YapiColumnDesc* desc, uint32_t* bindSize, uint32_t maxCharsetRatio)
{
    switch (desc->type) {
        case YAPI_TYPE_CHAR:
        case YAPI_TYPE_VARCHAR:
            *bindSize = codSizeAlign4(desc->size) * maxCharsetRatio + 1;
            break;
        case YAPI_TYPE_NCHAR:
        case YAPI_TYPE_NVARCHAR:
            *bindSize = codSizeAlign4(desc->size) + 1;
            break;

        case YAPI_TYPE_BINARY:
            *bindSize = codSizeAlign4(desc->size);
            break;

        case YAPI_TYPE_DATE:
            *bindSize = 32;
            break;

        case YAPI_TYPE_TIMESTAMP:
        case YAPI_TYPE_TIMESTAMP_TZ:
        case YAPI_TYPE_TIMESTAMP_LTZ:
            *bindSize = 64;
            break;
        
        case YAPI_TYPE_YM_INTERVAL:
            *bindSize = 15;
            break;

        case YAPI_TYPE_DS_INTERVAL:
            *bindSize = 32;
            break;

        case YAPI_TYPE_TINYINT:
            *bindSize = 5;
            break;

        case YAPI_TYPE_SMALLINT:
            *bindSize = 8;
            break;

        case YAPI_TYPE_INTEGER:
            *bindSize = 12;
            break;

        case YAPI_TYPE_BIGINT:
        case YAPI_TYPE_FLOAT:
        case YAPI_TYPE_DOUBLE:
            *bindSize = 21;
            break;

        case YAPI_TYPE_NUMBER:
        case YAPI_TYPE_NUMBER_FLOAT:
            *bindSize = codSizeAlign4(desc->precision + 8);;
            break;

        case YAPI_TYPE_BIT:
            *bindSize = desc->size + 1;
            break;

        case YAPI_TYPE_ROWID:
            *bindSize = 44;
            break;

        case YAPI_TYPE_BLOB:
        case YAPI_TYPE_CLOB:
        case YAPI_TYPE_NCLOB:
            *bindSize = -1;
            break;
        default:
            *bindSize = 20;
            break;
    }
}

YapiResult anpCursorPerformBind(AnpCursor* cursor)
{
    PyObject * key, *var;
    Py_ssize_t pos;

    cursor->setInputSizes = 0;

    if (!cursor->bindVariables) {
        return YAPI_SUCCESS;
    }

    if (PyDict_Check(cursor->bindVariables)) {
        pos = 0;
        while (PyDict_Next(cursor->bindVariables, &pos, &key, &var)) {
            if (anpBindVar((AnpVar*)var, cursor, key, 1) < 0) {
                return YAPI_ERROR;
            }
        }
        return YAPI_SUCCESS;
    }

    for (int i = 0; i < PyList_GET_SIZE(cursor->bindVariables); i++) {
        var = PyList_GET_ITEM(cursor->bindVariables, i);
        if (var != Py_None) {
            if (anpBindVar((AnpVar*)var, cursor, NULL, i + 1) < 0) {
                return YAPI_ERROR;
            }
        }
    }
    return YAPI_SUCCESS;
}

static int anpCursorPerformDefine(AnpCursor* cursor, uint32_t numQueryColumns)
{
    if (cursor->fetchVariables != NULL) {
        return 0;
    }

    cursor->fetchVariables = PyList_New(numQueryColumns);
    if (cursor->fetchVariables == NULL) {
        return -1;
    }

    uint32_t pos;
    YapiColumnDesc queryInfo;
    cursor->fetchArraySize = cursor->arraySize;
    for (pos = 0; pos < numQueryColumns; pos++) {
        if (yapiDescribeCol2(cursor->hStmt, pos, &queryInfo) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }

        if ((queryInfo.type > YAPI_TYPE_CURSOR) && (queryInfo.type != YAPI_TYPE_NUMBER_FLOAT)) {
            anpRaiseExceptionFromString(anpNotSupportedException, "unsupported binding type");
            return -1;
        }

        uint32_t maxCharsetRatio = 1;
        yapiGetConnAttr(cursor->connection->hConn, YAPI_ATTR_MAX_CHARSET_RATIO, &maxCharsetRatio, sizeof(uint32_t), NULL);

        uint32_t size;
        anpGetColumnSize(&queryInfo, &size, maxCharsetRatio);

        VarAssist assist = {.numElements = cursor->fetchArraySize, .type = queryInfo.type, 
            .size = size, .isArray = false, .bindIn = false};
        AnpVar* var = anpNewVar(cursor, &assist);
        if (var == NULL) {
            return -1;
        }

        PyList_SET_ITEM(cursor->fetchVariables, pos, (PyObject*)var);

        // if (anpVarIsLobType(var)) {
        //     if (yapiBindColumn(cursor->hStmt, pos, var->transType, var->data, -1, var->indicator) != YAPI_SUCCESS) {
        //         return anpRaiseAndReturnIntException();
        //     }
        //     continue;
        // }

        if (yapiBindColumn(cursor->hStmt, pos, var->transType, var->data, size, var->indicator) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
    }
    return 0;
}

static int anpTryReleaseLobLoc(AnpCursor* cursor)
{
    if (!cursor->bindVariables) {
        return YAPI_SUCCESS;
    }

    AnpVar*  bindVar;
    if (PyList_Check(cursor->bindVariables)) {
        uint32_t bindNum = (uint32_t)PyList_GET_SIZE(cursor->bindVariables);
        for (uint32_t i = 0; i < bindNum; i++) {
            bindVar = (AnpVar*)PyList_GET_ITEM(cursor->bindVariables, i);
            if (!anpVarIsLobType(bindVar)) {
                continue;
            }
            for (uint32_t i = 0; i < bindVar->elements; i++) {
                if (yapiLobFreeTemporary(cursor->connection->hConn,
                                         (YapiLobLocator*)(bindVar->data + i * sizeof(YapiLobLocator*))) !=
                    YAPI_SUCCESS) {
                    return anpRaiseAndReturnIntException();
                }
            }
        }
    } else if (PyDict_Check(cursor->bindVariables)) {
        PyObject * key, *var;
        Py_ssize_t pos = 0;
        while (PyDict_Next(cursor->bindVariables, &pos, &key, &var)) {
            bindVar = (AnpVar*)var;
            if (!anpVarIsLobType(bindVar)) {
                continue;
            }
            for (uint32_t i = 0; i < bindVar->elements; i++) {
                if (yapiLobFreeTemporary(cursor->connection->hConn,
                                         (YapiLobLocator*)(bindVar->data + i * sizeof(YapiLobLocator*))) !=
                    YAPI_SUCCESS) {
                    return anpRaiseAndReturnIntException();
                }
            }
        }
    }
    return 0;
}

int yaspyGetDbTypeFromPyType(PyObject *type, YapiType *dbType) 
{
    int status = PyObject_IsInstance(type, (PyObject*)&yasPyTypeApiType);
    if (status < 0) {
        return -1;
    }

    if (status == 1) {
        yaspyApiType *apiType = (yaspyApiType*)type;
        *dbType = apiType->defaultDbType;
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "expecting dbapi type");
    return -1;
}

static int getDefaultTypeSize(YapiType type)
{
    int typeSize = 0;
    switch (type)
    {
        case YAPI_TYPE_TINYINT:
        case YAPI_TYPE_SMALLINT:
        case YAPI_TYPE_INTEGER:
        case YAPI_TYPE_BIGINT:
            typeSize = 8;
            break;
        default:
            typeSize = 20;
            break;
    }

    return typeSize;
}

// create an anpVar for binding out parameter
static PyObject* yaspyCursorVar(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    //add a arg for input/output ?
    static char *keywordList[] = { "typ", "size", "arraysize", NULL};
    PyObject *type;

    int size = 0;
    int arraySize = 1;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O|ii", keywordList, &type, &size, &arraySize)) {
        return NULL;
    }

    YapiType dbType = YAPI_TYPE_UNKNOWN;
    if (yaspyGetDbTypeFromPyType(type, &dbType) < 0) {
        return NULL;
    }

    if (size == 0) {
        size = getDefaultTypeSize(dbType);
    }
    VarAssist assist = {.numElements = arraySize, .type = dbType, .size = size, .isArray = false, .bindIn = false};
    AnpVar *var = anpNewVar(cursor, &assist);
    var->bindDir = YAPI_PARAM_OUTPUT;

    return (PyObject*)var;
}

static void resetBindAnpVars(AnpCursor* cursor)
{
    if (cursor->bindVariables == NULL) {
        return;
    }

    bool isList = PyList_Check(cursor->bindVariables);
    if (isList) {
        uint32_t bindVarsCnt = (uint32_t)PyList_GET_SIZE(cursor->bindVariables);
        for (uint32_t i = 0; i < bindVarsCnt; i++) {
            PyObject* origVar = PyList_GET_ITEM(cursor->bindVariables, i);
            if ((origVar == NULL) || (origVar == Py_None)) {
                continue;
            }

            ((AnpVar*)origVar)->dataOffset = 0;
        }
    }

    bool isDict = PyDict_Check(cursor->bindVariables);
    if (isDict) {
        Py_ssize_t pos = 0;
        PyObject*  key;
        PyObject*  value;

        while (PyDict_Next(cursor->bindVariables, &pos, &key, &value)) {
            PyObject* origVar = PyDict_GetItem(cursor->bindVariables, key);
            if ((origVar == NULL) || (origVar == Py_None)) {
                continue;
            }
            ((AnpVar*)origVar)->dataOffset = 0;
        }
    }

}

static PyObject* anpCursorExecute(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    PyObject *statement, *execArgs;
    int16_t  numQueryColumns;
    int       status;

    execArgs = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &statement, &execArgs)) {
        return NULL;
    }

    if (execArgs && keywordArgs) {
        if (PyDict_Size(keywordArgs) == 0) {
            keywordArgs = NULL;
        } else {
            return anpRaiseExceptionFromString(anpInterfaceErrorException,
                                               "expecting argument or keyword arguments, not both");
        }
    }
    if (keywordArgs) {
        execArgs = keywordArgs;
    }
    if (execArgs) {
        if (!PyDict_Check(execArgs) && !PySequence_Check(execArgs)) {
            PyErr_SetString(PyExc_TypeError, "expecting a dictionary, sequence or keyword args");
            return NULL;
        }
    }

    if (!anpCursorIsOpen(cursor)) {
        return NULL;
    }

    if (statement == cursor->statment && !cursor->isFail) {
        statement = cursor->statment;
    } else {
        Py_CLEAR(cursor->statment);
        Py_XINCREF(statement);
        cursor->statment = statement;
        Py_CLEAR(cursor->fetchVariables);
        Py_CLEAR(cursor->bindVariables);

        char* sql = PyBytes_AsString(PyUnicode_AsUTF8String(statement));
        Py_BEGIN_ALLOW_THREADS
            if (cursor->hStmt != NULL) {
                yapiReleaseStmt(cursor->hStmt);
                cursor->hStmt = NULL;
            }
            status = yapiPrepare(cursor->connection->hConn, sql, (int32_t)strlen(sql), &cursor->hStmt);
        Py_END_ALLOW_THREADS
        if (status != YAPI_SUCCESS) {
            cursor->isFail = 1;
            return anpRaiseAndReturnNullException();
        }
        int32_t len;
        if (yapiGetStmtAttr(cursor->hStmt, YAPI_ATTR_SQLTYPE, &cursor->sqlType, sizeof(cursor->sqlType), &len) != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
    }
    if (yapiNumResultCols(cursor->hStmt, &numQueryColumns) != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }

    resetBindAnpVars(cursor);
    if (execArgs && anpCursorSetBindVariables(cursor, execArgs, 1, 0) < 0) {
        return NULL;
    }

    if (anpCursorPerformBind(cursor) != YAPI_SUCCESS) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
        status = yapiExecute(cursor->hStmt);
    Py_END_ALLOW_THREADS
    if (status != YAPI_SUCCESS) {
        cursor->isFail = 1;
        return anpRaiseAndReturnNullException();
    }

    cursor->isFail = 0;
    if (anpTryReleaseLobLoc(cursor) < 0)
    {
        return anpRaiseAndReturnNullException();
    }

    if (numQueryColumns == 0) {
        if (cursor->sqlType >= YAPI_SQLTYPE_CREATE_DATABASE) {
            cursor->rowCount = 0;
            Py_RETURN_NONE;
        }
        int32_t len;
        if (yapiGetStmtAttr(cursor->hStmt, YAPI_ATTR_ROWS_AFFECTED, &cursor->rowCount, sizeof(uint64_t), &len)
            != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
        Py_RETURN_NONE;
    } else {
        cursor->rowCount = 0;
    }

    if (numQueryColumns <= 0) {
        Py_RETURN_NONE;
    }

    if (anpCursorPerformDefine(cursor, numQueryColumns) >= 0) {
        Py_INCREF(cursor);
        return (PyObject*)cursor;
    }
    return NULL;
}

static PyObject* anpCursorClose(AnpCursor* cursor, PyObject* args)
{
    if (!anpCursorIsOpen(cursor)) {
        return NULL;
    }

    if (cursor->hStmt != NULL) {
        yapiReleaseStmt(cursor->hStmt);
        cursor->hStmt = NULL;
    }
    cursor->isOpen = false;
    Py_RETURN_NONE;
}

static PyObject* anpCursorCreateRow(AnpCursor* cursor, uint32_t pos)
{
    PyObject * tuple, *item;
    Py_ssize_t i;

    cursor->rowCount++;

    Py_ssize_t numItems = PyList_GET_SIZE(cursor->fetchVariables);
    tuple = PyTuple_New(numItems);
    if (!tuple) {
        return NULL;
    }

    for (i = 0; i < numItems; i++) {
        AnpVar* var = (AnpVar*)PyList_GET_ITEM(cursor->fetchVariables, i);
        item = anpVarGetSingleValue(var->connection->hConn, var, pos);
        if (item == NULL) {
            Py_DECREF(tuple);
            return NULL;
        }

        PyTuple_SET_ITEM(tuple, i, item);
    }

    return tuple;
}

static YapiResult anpCursorCheck(AnpCursor* cursor)
{
    if (!anpCursorIsOpen(cursor)) {
        return YAPI_ERROR;
    }
    if (cursor->fetchVariables == NULL) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not a query");
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

static PyObject* anpCursorFetchOne(AnpCursor* cursor, PyObject* args)
{
    YapiResult ret;
    if (anpCursorCheck(cursor) != YAPI_SUCCESS) {
        return NULL;
    }
    
    uint32_t rows;
    Py_BEGIN_ALLOW_THREADS
    ret = yapiFetch(cursor->hStmt, &rows);
    Py_END_ALLOW_THREADS
    if (ret != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    if (rows > 0) {
        return anpCursorCreateRow(cursor, 0);
    }

    Py_RETURN_NONE;
}

static PyObject* anpCursorCallProc(AnpCursor* cursor, PyObject* args)
{
    const char* procName;
    PyObject*   params = NULL;

    if (!PyArg_ParseTuple(args, "s|O", &procName, &params)) {
        return NULL;
    }

    uint32_t paramCnt = 0;
    if (params) {
        if (!PySequence_Check(params)) {
            return NULL;
        }
        paramCnt = PySequence_Size(params);
    }

    const char* sql_format = "begin %s(%s); end;";
    char paramListStr[PROCEDURE_PARAM_LIST_BUFFER_SIZE] = "";
    for (uint32_t i = 0; i < paramCnt; i++) {
        if (i == 0) {
            strcat(paramListStr, "?");
            continue;
        }
        strcat(paramListStr, ",?");
    }

    char sql[PROCEDURE_SQL_BUFFER_SIZE] = "";
    sprintf(sql, sql_format, procName, paramListStr);

    PyObject* argsTuple = PyTuple_New(2);
    if (!argsTuple) {
        return anpRaiseAndReturnNullException();
    }
    PyTuple_SetItem(argsTuple, 0, PyUnicode_FromString(sql));
    PyTuple_SetItem(argsTuple, 1, params);
    if (!anpCursorExecute(cursor, argsTuple, NULL)) {
        Py_DECREF(argsTuple);
        return NULL;
    }
    Py_DECREF(argsTuple);

    PyObject* result = PyList_New(0);
    if (!result) {
        return NULL;
    }
    if (!cursor->bindVariables) {
        return result;
    }
    Py_ssize_t size = PyList_GET_SIZE(cursor->bindVariables);
    AnpVar* bindVar = NULL;
    PyObject* value = NULL;
    for (uint32_t i = 0; i < size; i++) {
        bindVar = (AnpVar*)PyList_GET_ITEM(cursor->bindVariables, i);
        value = anpVarGetSingleValue(bindVar->connection->hConn, bindVar, 0);
        if (PyList_Append(result, value)) {
            Py_DECREF(value);
            return NULL;
        }
        Py_DECREF(value);
    }
    return result;
}

static int anpInternalPrepare(AnpCursor* cursor, PyObject *sqlStr)
{
    if (sqlStr == cursor->statment && !cursor->isFail) {
        sqlStr = cursor->statment;
        return 0;
    }

    Py_CLEAR(cursor->statment);
    Py_XINCREF(sqlStr);
    cursor->statment = sqlStr;
    Py_CLEAR(cursor->fetchVariables);
    Py_CLEAR(cursor->bindVariables);

    int status;
    char* sql = PyBytes_AsString(PyUnicode_AsUTF8String(sqlStr));
    Py_BEGIN_ALLOW_THREADS
        if (cursor->hStmt != NULL) {
            yapiReleaseStmt(cursor->hStmt);
            cursor->hStmt = NULL;
        }
        status = yapiPrepare(cursor->connection->hConn, sql, (int32_t)strlen(sql), &cursor->hStmt);
    Py_END_ALLOW_THREADS
    if (status != YAPI_SUCCESS) {
        cursor->isFail = 1;
        return anpRaiseAndReturnIntException();
    }

    int32_t len;
    if (yapiGetStmtAttr(cursor->hStmt, YAPI_ATTR_SQLTYPE, &cursor->sqlType, sizeof(cursor->sqlType), &len) != YAPI_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    return 0;
}

static PyObject* anpCursorExecuteMany(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    PyObject *sqlStr;
    PyObject *execArgs;
    int batchErrorsEnabled = 0;
    int arrayDMLRowCountsEnabled = 0;
    static char *keywordList[] = { "statement", "parameters", "batcherrors",
            "arraydmlrowcounts", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "OO|ii", keywordList, 
    &sqlStr, &execArgs, &batchErrorsEnabled, &arrayDMLRowCountsEnabled)) {
        PyErr_SetString(PyExc_TypeError, "parameters should be a list of sequences/dictionaries "
                "or an integer specifying the number of times to execute "
                "the statement");
        return NULL;
    }

    if (!anpCursorIsOpen(cursor)) {
        return NULL;
    }

    int16_t  numQueryColumns;
    int       status;

    // do prepare, todo: extract a func() for both here and the execute()'s code
    if (anpInternalPrepare(cursor, sqlStr) < 0) {
        return NULL;
    }

    if (yapiNumResultCols(cursor->hStmt, &numQueryColumns) != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }

    resetBindAnpVars(cursor);
    PyObject *rowParameter;
    uint32_t paramRowCnt = (uint32_t) PyList_GET_SIZE(execArgs);
    for (uint32_t i = 0; i < paramRowCnt; i++) {
        rowParameter = PyList_GET_ITEM(execArgs, i);
        if (!PyDict_Check(rowParameter) && !PySequence_Check(rowParameter)) {
            return anpRaiseExceptionFromString(anpInterfaceErrorException, "expecting a list of dictionaries or sequences");
        }
        
        if (anpCursorSetBindVariables(cursor, rowParameter, paramRowCnt, i) < 0) {
            return NULL;
        }
    }

    if (yapiSetStmtAttr(cursor->hStmt, YAPI_ATTR_PARAMSET_SIZE, &paramRowCnt, sizeof(uint32_t)) != YAPI_SUCCESS) {
        return NULL;
    }

    if (anpCursorPerformBind(cursor) != YAPI_SUCCESS) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
        status = yapiExecute(cursor->hStmt);
    Py_END_ALLOW_THREADS
    if (status != YAPI_SUCCESS) {
        cursor->isFail = 1;
        return anpRaiseAndReturnNullException();
    }

    cursor->isFail = 0;
    if (anpTryReleaseLobLoc(cursor) < 0)
    {
        return anpRaiseAndReturnNullException();
    }

    if (numQueryColumns == 0) {
        if (cursor->sqlType >= YAPI_SQLTYPE_CREATE_DATABASE) {
            cursor->rowCount = 0;
            Py_RETURN_NONE;
        }
        int32_t len;
        if (yapiGetStmtAttr(cursor->hStmt, YAPI_ATTR_ROWS_AFFECTED, &cursor->rowCount, sizeof(uint64_t), &len)
            != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
        Py_RETURN_NONE;
    } else {
        cursor->rowCount = 0;
    }

    Py_RETURN_NONE;
}

static PyObject* anpCursorFetch(AnpCursor* cursor, uint32_t fetchRows)
{
    PyObject *list = NULL;
    PyObject *row = NULL;
    YapiResult ret;
    uint32_t rowCount, rows;

    if (anpCursorCheck(cursor) != YAPI_SUCCESS) {
        return NULL;
    }

    list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }

    for(rowCount = 0; fetchRows == 0 || rowCount < fetchRows; rowCount++) {
        rows = 0;
        Py_BEGIN_ALLOW_THREADS
        ret = yapiFetch(cursor->hStmt, &rows);
        Py_END_ALLOW_THREADS

        if (ret != YAPI_SUCCESS) {
            Py_DECREF(list);
            return anpRaiseAndReturnNullException();
        }

        if (rows <= 0) {
            break;
        }

        row = anpCursorCreateRow(cursor, 0);
        if ( row == NULL) {
            Py_DECREF(list);
            return NULL;
        }

        if (PyList_Append(list, row) < 0) {
            Py_DECREF(row);
            Py_DECREF(list);
            return NULL;
        }

        Py_DECREF(row);
    }

    return list;
}

static PyObject* anpCursorFetchMany(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    int32_t fetchRows = cursor->arraySize;
    static char*   keywordList[] = {"size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|i", keywordList, &fetchRows)) {
        return NULL;
    }

    if (fetchRows <= 0) {
        return anpRaiseExceptionFromString(anpInterfaceErrorException,
               "The fetch size should be a number and greater than zero.");
    }

    return anpCursorFetch(cursor,fetchRows);
}

static PyObject* anpCursorFetchAll(AnpCursor* cursor, PyObject* args)
{
    return anpCursorFetch(cursor, 0);
}

static PyObject* anpCursorNextSet(AnpCursor* cursor, PyObject* args)
{
    return anpRaiseExceptionFromString(anpNotSupportedException, "nextset() not support");
}

static PyObject* anpCursorSetInputSizes(AnpCursor* cursor, PyObject* args)
{
    return anpRaiseExceptionFromString(anpNotSupportedException, "setinputsizes() not implement");
}

static PyObject* anpCursorSetOutputSize(AnpCursor* cursor, PyObject* args)
{
    return anpRaiseExceptionFromString(anpNotSupportedException, "setoutputsize() not implement");
}

static PyObject* anpCursorIter(AnpCursor* cursor)
{
    if (anpCursorCheck(cursor) != YAPI_SUCCESS) {
        return NULL;
    }

    Py_INCREF(cursor);
    return (PyObject*)cursor;
}

static PyObject* anpCursorNext(AnpCursor * cursor)
{
    YapiResult ret;
    if (anpCursorCheck(cursor) != YAPI_SUCCESS) {
        return NULL;
    }

    uint32_t rows;
    Py_BEGIN_ALLOW_THREADS
    ret = yapiFetch(cursor->hStmt, &rows);
    Py_END_ALLOW_THREADS
    if (ret != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    
    if (rows > 0) {
        return anpCursorCreateRow(cursor, 0);
    }

    return NULL;
}

static void anpCursorFree(AnpCursor* cursor)
{
    Py_CLEAR(cursor->fetchVariables);
    Py_CLEAR(cursor->bindVariables);
    if (cursor->hStmt != NULL) {
        yapiReleaseStmt(cursor->hStmt);
        cursor->hStmt = NULL;
    }

    Py_CLEAR(cursor->connection);
    Py_TYPE(cursor)->tp_free((PyObject*)cursor);
}

static PyObject *yaspyCursor_contextManagerEnter(AnpCursor *cursor,
        PyObject* args)
{
    Py_INCREF(cursor);
    return (PyObject*) cursor;
}

static PyObject *yaspyCursor_contextManagerExit(AnpCursor *cursor,
        PyObject* args)
{
    PyObject *excType, *excValue, *excTraceback, *result;
    if (!PyArg_ParseTuple(args, "OOO", &excType, &excValue, &excTraceback)) {
        return NULL;
    }

    result = anpCursorClose(cursor, NULL);
    if (result == NULL) {
        return NULL;
    }

    Py_DECREF(result);
    Py_INCREF(Py_False);
    return Py_False;
}

static PyMethodDef anpMethods[] = {
        {"callproc",     (PyCFunction) anpCursorCallProc,    METH_VARARGS  | METH_KEYWORDS },
        {"close",        (PyCFunction) anpCursorClose,       METH_NOARGS},
        {"execute",      (PyCFunction) anpCursorExecute,     METH_VARARGS | METH_KEYWORDS},
        {"executemany",  (PyCFunction) anpCursorExecuteMany, METH_VARARGS | METH_KEYWORDS },
        {"fetchone",     (PyCFunction) anpCursorFetchOne,    METH_NOARGS},
        {"fetchmany",    (PyCFunction) anpCursorFetchMany,   METH_VARARGS | METH_KEYWORDS },
        {"fetchall",     (PyCFunction) anpCursorFetchAll,    METH_NOARGS },
        {"nextset",      (PyCFunction) anpCursorNextSet,     METH_NOARGS},
        {"setinputsizes", (PyCFunction) anpCursorSetInputSizes, METH_VARARGS | METH_KEYWORDS },
        {"setoutputsize", (PyCFunction) anpCursorSetOutputSize, METH_VARARGS },
        {"var",          (PyCFunction)yaspyCursorVar, METH_VARARGS | METH_KEYWORDS},
        { "__enter__",   (PyCFunction) yaspyCursor_contextManagerEnter, METH_NOARGS },
        { "__exit__",    (PyCFunction) yaspyCursor_contextManagerExit, METH_VARARGS },
        {NULL, NULL}
};

static PyMemberDef anpMembers[] = {
        {"arraysize", T_UINT,      offsetof(AnpCursor, arraySize), 0},
        {"rowcount",  T_ULONGLONG, offsetof(AnpCursor, rowCount), READONLY},
        {NULL}
};

static PyGetSetDef anpCalcMembers[] = {
        {"description", (getter) anpCursorGetDescription, 0, 0, 0},
        {NULL}
};

PyTypeObject anchorPyTypeCursor = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "yaspy.Cursor",
        .tp_basicsize = sizeof(AnpCursor),
        .tp_dealloc = (destructor) anpCursorFree,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_iter = (getiterfunc)anpCursorIter,
        .tp_iternext = (iternextfunc)anpCursorNext,
        .tp_methods = anpMethods,
        .tp_members = anpMembers,
        .tp_getset = anpCalcMembers,
        .tp_init = (initproc) anpCursorInit,
        .tp_new = anpCursorNew
};

YapiResult anpRegistCursor(PyObject *module)
{
    PyType_Ready(&anchorPyTypeCursor);

    Py_INCREF(&anchorPyTypeCursor);
    if (PyModule_AddObject(module, "Cursor", (PyObject *) &anchorPyTypeCursor) < 0) {
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

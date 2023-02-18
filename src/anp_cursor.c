#include "anp_cursor.h"
#include "structmember.h"
#include "anp_exception.h"
#include "anp_var.h"


static PyObject* anpCursorNew(PyTypeObject* type, PyObject* args, PyObject* keywordArgs)
{
    return type->tp_alloc(type, 0);
}

static int anpCursorInit(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    static char*   keywordList[] = {"connection", "scrollable", NULL};
    AnpConnection* connection;
    PyObject*      scrollableObj;

    // parse arguments
    scrollableObj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O!|O", keywordList, &anchorPyTypeConnection, &connection,
                                     &scrollableObj)) {
        return -1;
    }

    // initialize members
    Py_INCREF(connection);
    cursor->connection = connection;
    cursor->arraySize = 100;
    cursor->isOpen = 1;
    cursor->isFail = 0;

    return 0;
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
    int displaySize = anpGetDisplaySize(&desc);

    PyTuple_SET_ITEM(tuple, 0, PyUnicode_Decode(desc.name, strlen(desc.name), NULL, NULL));
    PyTuple_SET_ITEM(tuple, 1, PyLong_FromLong(desc.type));
    if (displaySize != 0) {
        PyTuple_SET_ITEM(tuple, 2, PyLong_FromLong(displaySize));
    } else {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 2, Py_None);
    }

    if (desc.size != 0) {
        PyTuple_SET_ITEM(tuple, 3, PyLong_FromLong(desc.size));
    } else {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 3, Py_None);
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

    // make sure the cursor is open
    if (!anpCursorIsOpen(cursor)) {
        return NULL;
    }

    // determine the number of query columns; if not a query return None
    if (cursor->hStmt == NULL) {
        Py_RETURN_NONE;
    }
    if (yapiNumResultCols(cursor->hStmt, &queryColumns) != YAPI_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    if (queryColumns == 0) {
        Py_RETURN_NONE;
    }

    // create a list of the required length
    PyObject* results = PyList_New(queryColumns);
    if (results == NULL) {
        return NULL;
    }

    // create tuples corresponding to the select-items
    for (int16_t i = 0; i < queryColumns; i++) {
        PyObject* tuple = anpCursorItemDescription(cursor, i);
        if (tuple == NULL) {
            Py_DECREF(results);
            return NULL;
        }
        PyList_SET_ITEM(results, i, tuple);
    }

    return results;
}

static YapiResult anpCursorSetBindVariableHelper(AnpCursor* cursor, unsigned numElements, unsigned arrayPos,
    PyObject* value, AnpVar* origVar, AnpVar** newVar, int deferTypeAssignment)
{
    AnpVar* varToSet;
    int     isValueVar;

    *newVar = NULL;
    isValueVar = anpCheckVar(value);

    // handle case where variable is already bound, either from a prior
    // execution or a call to setinputsizes()
    if (origVar != NULL) {
        // if the value is a variable object, rebind it if necessary
        if (isValueVar) {
            if ((PyObject*)origVar != value) {
                Py_INCREF(value);
                *newVar = (AnpVar*)value;
            }
            return YAPI_SUCCESS;
        }

        varToSet = origVar;
        if (numElements >= origVar->elements) {
            anpAdjustVarTypeSize(value, &origVar->size, &origVar->dbType);

            VarAssist assist = { .isArray = origVar->isArray, .numElements = numElements, 
                .size = origVar->size, .type = origVar->dbType};
            *newVar = anpNewVar(cursor, &assist, true);
            if (!*newVar) {
                return YAPI_ERROR;
            }
            varToSet = *newVar;
        }

        // attempt to set the value
        if (varToSet && anpVarSetValue(cursor->connection->hConn, varToSet, arrayPos, value) < 0) {
            // executemany() should simply fail after the first element
            if (arrayPos > 0) {
                return YAPI_ERROR;
            }

            // clear the exception and try to create a new variable
            PyErr_Clear();
            Py_CLEAR(*newVar);
            origVar = NULL;
        }
    }

    // if no original variable used, create a new one
    if (origVar == NULL) {
        // if the value is a variable object, bind it directly
        if (isValueVar) {
            Py_INCREF(value);
            *newVar = (AnpVar*)value;

            // otherwise, create a new variable, unless the value is None and
            // we wish to defer type assignment
        } else if (value != Py_None || !deferTypeAssignment) {
            *newVar = anpVarNewByValue(cursor, value, numElements, true);
            if (*newVar == NULL) {
                return YAPI_ERROR;
            }
            if (anpVarSetValue(cursor->connection->hConn, *newVar, arrayPos, value) < 0) {
                Py_CLEAR(*newVar);
                return YAPI_ERROR;
            }
        }
    }

    return YAPI_SUCCESS;
}

YapiResult anpCursorSetBindByPos(AnpCursor* cursor, PyObject* parameters, unsigned numElements, unsigned arrayPos,
                                int deferTypeAssignment)
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
    AnpVar* newVar;

    for (uint32_t i = 0; i < numParams; i++) {
        PyObject* origVar;
        PyObject* value = PySequence_GetItem(parameters, i);
        if (value == NULL) {
            return YAPI_ERROR;
        }
        Py_DECREF(value);
        if (i < origNumParams) {
            origVar = PyList_GET_ITEM(cursor->bindVariables, i);
            if (origVar == Py_None) {
                origVar = NULL;
            }
        } else {
            origVar = NULL;
        }

        if (anpCursorSetBindVariableHelper(cursor, numElements, arrayPos, value, (AnpVar*)origVar, &newVar,
                                           deferTypeAssignment) < 0) {
            return YAPI_ERROR;
        }
        if (newVar) {
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

YapiResult anpCursorSetBindByName(AnpCursor* cursor, PyObject* parameters, unsigned numElements, unsigned arrayPos,
                                 int deferTypeAssignment)
{
#if 0
    if (cursor->bindVariables) {
        YacUint32 origBoundByPos = PyList_Check(cursor->bindVariables);
        if (origBoundByPos) {
            anpErrorRaiseFromString(anpProgrammingErrorException, "positional and named binds cannot be intermixed");
            return -1;
        }
    } else {
        cursor->bindVariables = PyDict_New();
        if (!cursor->bindVariables) {
            return -1;
        }
    }

    AnpVar* newVar;
    Py_ssize_t  pos = 0;
    PyObject*   key;
    PyObject*   value;

    while (PyDict_Next(parameters, &pos, &key, &value)) {
        PyObject* origVar = PyDict_GetItem(cursor->bindVariables, key);
        if (anpCursorSetBindVariableHelper(cursor, numElements, arrayPos, value, (AnpVar*)origVar, &newVar,
                                           deferTypeAssignment) < 0) {
            return -1;
        }
        if (newVar) {
            if (PyDict_SetItem(cursor->bindVariables, key, (PyObject*)newVar) < 0) {
                Py_DECREF(newVar);
                return -1;
            }
            Py_DECREF(newVar);
        }
    }
#endif
    return 0;
}

YapiResult anpCursorSetBindVariables(AnpCursor* cursor, PyObject* parameters, uint32_t numElements, unsigned arrayPos,
                                    int deferTypeAssignment)
{
    if (PySequence_Check(parameters)) {
        return anpCursorSetBindByPos(cursor, parameters, numElements, arrayPos, deferTypeAssignment);
    } else {
        return anpCursorSetBindByName(cursor, parameters, numElements, arrayPos, deferTypeAssignment);
    }
}

void anpGetColumnSize(YapiColumnDesc* desc, uint32_t* bindSize)
{
    switch (desc->type) {
        case YAPI_TYPE_CHAR:
        case YAPI_TYPE_VARCHAR:
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
    int        i;

    // ensure that input sizes are reset
    // this is done before binding is attempted so that if binding fails and
    // a new statement is prepared, the bind variables will be reset and
    // spurious errors will not occur
    cursor->setInputSizes = 0;

    // set values and perform binds for all bind variables
    if (cursor->bindVariables) {
        if (PyDict_Check(cursor->bindVariables)) {
            pos = 0;
            while (PyDict_Next(cursor->bindVariables, &pos, &key, &var)) {
                if (anpBindVar((AnpVar*)var, cursor, key, 1) < 0) {
                    return YAPI_ERROR;
                }
            }
        } else {
            for (i = 0; i < PyList_GET_SIZE(cursor->bindVariables); i++) {
                var = PyList_GET_ITEM(cursor->bindVariables, i);
                if (var != Py_None) {
                    if (anpBindVar((AnpVar*)var, cursor, NULL, i + 1) < 0) {
                        return YAPI_ERROR;
                    }
                }
            }
        }
    }
    return YAPI_SUCCESS;
}

static int anpCursorPerformDefine(AnpCursor* cursor, uint32_t numQueryColumns)
{
    // if fetch variables already exist, nothing more to do (we are executing
    // the same statement and therefore all defines have already been
    // performed)
    if (cursor->fetchVariables) {
        return 0;
    }

    // create a list corresponding to the number of items
    cursor->fetchVariables = PyList_New(numQueryColumns);
    if (!cursor->fetchVariables) {
        return -1;
    }

    uint32_t     pos;
    YapiColumnDesc queryInfo;
    // create a variable for each of the query columns
    cursor->fetchArraySize = cursor->arraySize;
    for (pos = 0; pos < numQueryColumns; pos++) {
        // get query information for the column position
        if (yapiDescribeCol2(cursor->hStmt, pos, &queryInfo) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }

        uint32_t size;
        anpGetColumnSize(&queryInfo, &size);

        VarAssist assist = {.numElements = cursor->fetchArraySize, .type = queryInfo.type, 
            .size = size, .isArray = false};
        AnpVar* var = anpNewVar(cursor, &assist, false);
        if (var == NULL) {
            return -1;
        }

        PyList_SET_ITEM(cursor->fetchVariables, pos, (PyObject*)var);

        if( var->transType == YAPI_TYPE_CLOB || var->transType == YAPI_TYPE_BLOB)
        {
            if (yapiBindColumn(cursor->hStmt, pos, var->transType, &var->data, -1, NULL) != YAPI_SUCCESS) {
                return anpRaiseAndReturnIntException();
            }
            continue;
        }

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

    uint32_t bindNum = (uint32_t)PyList_GET_SIZE(cursor->bindVariables);
    AnpVar* bindVar;
    for (uint32_t i = 0; i < bindNum; i++) {
        bindVar = (AnpVar*)PyList_GET_ITEM(cursor->bindVariables, i);
        if (bindVar->transType != YAPI_TYPE_CLOB && bindVar->transType != YAPI_TYPE_BLOB) {
            continue;
        }

        if (yapiLobFreeTemporary(cursor->connection->hConn, (YapiLobLocator*)bindVar->data) != YAPI_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
    }
    return 0;
}

static PyObject* anpCursorExecute(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    PyObject *statement, *executeArgs;
    int16_t  numQueryColumns;
    int       status;

    executeArgs = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &statement, &executeArgs)) {
        return NULL;
    }

    if (executeArgs && keywordArgs) {
        if (PyDict_Size(keywordArgs) == 0) {
            keywordArgs = NULL;
        } else {
            return anpRaiseExceptionFromString(anpInterfaceErrorException,
                                               "expecting argument or keyword arguments, not both");
        }
    }
    if (keywordArgs) {
        executeArgs = keywordArgs;
    }
    if (executeArgs) {
        if (!PyDict_Check(executeArgs) && !PySequence_Check(executeArgs)) {
            PyErr_SetString(PyExc_TypeError, "expecting a dictionary, sequence or keyword args");
            return NULL;
        }
    }

    // make sure the cursor is open
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

        // prepare the statement, if applicable
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

    // perform binds
    if (executeArgs && anpCursorSetBindVariables(cursor, executeArgs, 1, 0, 0) < 0) {
        return NULL;
    }

    if (anpCursorPerformBind(cursor) != YAPI_SUCCESS) {
        return NULL;
    }

    // execute the statement
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
        // get the count of the rows affected
        int32_t len;
        if (yapiGetStmtAttr(cursor->hStmt, YAPI_ATTR_ROWS_AFFECTED, &cursor->rowCount, sizeof(uint64_t), &len)
            != YAPI_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
        Py_RETURN_NONE;
    } else {
        cursor->rowCount = 0;
    }

    // for queries, return the cursor for convenience
    if (numQueryColumns > 0) {
        if (anpCursorPerformDefine(cursor, numQueryColumns) < 0) {
            return NULL;
        }
        Py_INCREF(cursor);
        return (PyObject*)cursor;
    }

    // for statements other than queries, simply return None
    Py_RETURN_NONE;
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

    // bump row count as a new row has been found
    cursor->rowCount++;

    Py_ssize_t numItems = PyList_GET_SIZE(cursor->fetchVariables);
    tuple = PyTuple_New(numItems);
    if (!tuple) {
        return NULL;
    }

    // acquire the value for each item
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
    return anpRaiseExceptionFromString(anpNotSupportedException, "callproc() not implement");
}

static PyObject* anpCursorExecuteMany(AnpCursor* cursor, PyObject* args)
{
    return anpRaiseExceptionFromString(anpNotSupportedException, "executemany() not implement");
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

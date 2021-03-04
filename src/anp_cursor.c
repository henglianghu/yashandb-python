#include "anp_cursor.h"
#include "structmember.h"
#include "cod_inc.h"
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

    if (ancAllocHandle(ANC_HANDLE_STMT, connection->hConn, &cursor->hStmt) != ANC_SUCCESS) {
        return anpRaiseAndReturnIntException();
    }

    // initialize members
    Py_INCREF(connection);
    cursor->connection = connection;
    cursor->arraySize = 100;
    cursor->isOpen = 1;

    return 0;
}

static void anpCursorFree(AnpCursor* cursor)
{
    Py_CLEAR(cursor->fetchVariables);
    Py_CLEAR(cursor->bindVariables);
    if (cursor->isOpen && cursor->connection->hConn != NULL) {
        ancFreeHandle(ANC_HANDLE_STMT, cursor->hStmt);
        cursor->hStmt = NULL;
        cursor->isOpen = COD_FALSE;
    }

    Py_CLEAR(cursor->connection);
    Py_TYPE(cursor)->tp_free((PyObject*)cursor);
}

static AncBool anpCursorIsOpen(AnpCursor* cursor)
{
    if (!cursor->isOpen) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not open");
        return COD_FALSE;
    }
    return anpConnectionIsConnected(cursor->connection);
}

CodUint32 anpGetDisplaySize(AncColumnDesc* desc)
{
    CodUint32 displaySize;
    switch (desc->type) {
        case ANC_TYPE_CHAR:
        case ANC_TYPE_VARCHAR:
        case ANC_TYPE_NCHAR:
        case ANC_TYPE_NVARCHAR:
        case ANC_TYPE_CLOB:
            displaySize = codSizeAlign4(desc->size) + 1;
            break;

        case ANC_TYPE_BINARY:
        case ANC_TYPE_BLOB:
            displaySize = codSizeAlign4(desc->size * 2);
            break;

        case ANC_TYPE_DATE:
            displaySize = 32;
            break;

        case ANC_TYPE_TIMESTAMP:
        case ANC_TYPE_TIMESTAMP_TZ:
        case ANC_TYPE_TIMESTAMP_LTZ:
            displaySize = 64;
            break;

        case ANC_TYPE_TINYINT:
            displaySize = 5;
            break;

        case ANC_TYPE_SMALLINT:
            displaySize = 8;
            break;

        case ANC_TYPE_INTEGER:
            displaySize = 12;
            break;

        case ANC_TYPE_BIGINT:
        case ANC_TYPE_FLOAT:
        case ANC_TYPE_DOUBLE:
            displaySize = 21;
            break;

        case ANC_TYPE_NUMBER:
            displaySize = codSizeAlign4(desc->precision + 8);
            break;

        case ANC_TYPE_BIT:
            displaySize = desc->size + 1;
            break;

        case ANC_TYPE_ROWID:
            displaySize = 24;
            break;

        default:
            displaySize = 0;
            break;
    }
    return displaySize;
}

static PyObject* anpCursorItemDescription(AnpCursor* cursor, AncUint32 pos)
{
    AncColumnDesc desc;
    if (ancDescribeCol2(cursor->hStmt, pos, &desc) != ANC_SUCCESS) {
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

    if (desc.precision != 0 || desc.scale != 0) {
        PyTuple_SET_ITEM(tuple, 4, PyLong_FromLong(desc.precision));
        PyTuple_SET_ITEM(tuple, 5, PyLong_FromLong(desc.scale));
    } else {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 4, Py_None);
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(tuple, 5, Py_None);
    }

    PyTuple_SET_ITEM(tuple, 6, PyLong_FromLong(desc.nullable));

    return tuple;
}

static PyObject* anpCursorGetDescription(AnpCursor* cursor, void* unused)
{
    AncInt16 queryColumns;

    // make sure the cursor is open
    if (!anpCursorIsOpen(cursor)) {
        return NULL;
    }

    // determine the number of query columns; if not a query return None
    if (cursor->hStmt == NULL) {
        Py_RETURN_NONE;
    }
    if (ancNumResultCols(cursor->hStmt, &queryColumns) != ANC_SUCCESS) {
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
    for (AncInt16 i = 0; i < queryColumns; i++) {
        PyObject* tuple = anpCursorItemDescription(cursor, i);
        if (tuple == NULL) {
            Py_DECREF(results);
            return NULL;
        }
        PyList_SET_ITEM(results, i, tuple);
    }

    return results;
}

static AncResult anpCursorSetBindVariableHelper(AnpCursor* cursor, unsigned numElements, unsigned arrayPos,
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
            return ANC_SUCCESS;
        }

        varToSet = origVar;
        if (numElements > origVar->elements) {
            *newVar = anpNewVar(cursor, numElements, origVar->dbType, origVar->size, origVar->isArray);
            if (!*newVar) {
                return ANC_ERROR;
            }
            varToSet = *newVar;
        }

        // attempt to set the value
        if (varToSet && anpVarSetValue(varToSet, arrayPos, value) < 0) {
            // executemany() should simply fail after the first element
            if (arrayPos > 0) {
                return ANC_ERROR;
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
            *newVar = anpVarNewByValue(cursor, value, numElements);
            if (*newVar == NULL) {
                return ANC_ERROR;
            }
            if (anpVarSetValue(*newVar, arrayPos, value) < 0) {
                Py_CLEAR(*newVar);
                return ANC_ERROR;
            }
        }
    }

    return ANC_SUCCESS;
}

AncResult anpCursorSetBindByPos(AnpCursor* cursor, PyObject* parameters, unsigned numElements, unsigned arrayPos,
                                int deferTypeAssignment)
{
    Py_ssize_t temp = PySequence_Size(parameters);
    if (temp < 0) {
        return ANC_ERROR;
    }

    AncUint32 numParams = (AncUint32)temp;
    AncUint32 origNumParams = 0;
    if (cursor->bindVariables) {
        AncUint32 origBoundByPos = PyList_Check(cursor->bindVariables);
        if (!origBoundByPos) {
            anpRaiseExceptionFromString(anpProgrammingErrorException,
                                        "positional and named binds cannot be intermixed");
            return ANC_ERROR;
        }
        origNumParams = (AncUint32)PyList_GET_SIZE(cursor->bindVariables);
    } else {
        cursor->bindVariables = PyList_New(numParams);
        if (!cursor->bindVariables) {
            return ANC_ERROR;
        }
    }
    AnpVar* newVar;

    for (AncUint32 i = 0; i < numParams; i++) {
        PyObject* origVar;
        PyObject* value = PySequence_GetItem(parameters, i);
        if (value == NULL) {
            return ANC_ERROR;
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
            return ANC_ERROR;
        }
        if (newVar) {
            if (i < (uint32_t)PyList_GET_SIZE(cursor->bindVariables)) {
                if (PyList_SetItem(cursor->bindVariables, i, (PyObject*)newVar) < 0) {
                    Py_DECREF(newVar);
                    return ANC_ERROR;
                }
            } else {
                if (PyList_Append(cursor->bindVariables, (PyObject*)newVar) < 0) {
                    Py_DECREF(newVar);
                    return ANC_ERROR;
                }
                Py_DECREF(newVar);
            }
        }
    }
    return ANC_SUCCESS;
}

AncResult anpCursorSetBindByName(AnpCursor* cursor, PyObject* parameters, unsigned numElements, unsigned arrayPos,
                                 int deferTypeAssignment)
{
#if 0
    if (cursor->bindVariables) {
        AncUint32 origBoundByPos = PyList_Check(cursor->bindVariables);
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

AncResult anpCursorSetBindVariables(AnpCursor* cursor, PyObject* parameters, AncUint32 numElements, unsigned arrayPos,
                                    int deferTypeAssignment)
{
    if (PySequence_Check(parameters)) {
        return anpCursorSetBindByPos(cursor, parameters, numElements, arrayPos, deferTypeAssignment);
    } else {
        return anpCursorSetBindByName(cursor, parameters, numElements, arrayPos, deferTypeAssignment);
    }
}

void anpGetColumnSize(AncColumnDesc* desc, CodUint32* bindSize)
{
    switch (desc->type) {
        case ANC_TYPE_CHAR:
        case ANC_TYPE_VARCHAR:
        case ANC_TYPE_NCHAR:
        case ANC_TYPE_NVARCHAR:
        case ANC_TYPE_CLOB:
            *bindSize = codSizeAlign4(desc->size) + 1;
            break;

        case ANC_TYPE_BINARY:
        case ANC_TYPE_BLOB:
            *bindSize = codSizeAlign4(desc->size * 2);
            break;

        case ANC_TYPE_DATE:
            *bindSize = 32;
            break;

        case ANC_TYPE_TIMESTAMP:
        case ANC_TYPE_TIMESTAMP_TZ:
        case ANC_TYPE_TIMESTAMP_LTZ:
            *bindSize = 64;
            break;

        case ANC_TYPE_TINYINT:
            *bindSize = 5;
            break;

        case ANC_TYPE_SMALLINT:
            *bindSize = 8;
            break;

        case ANC_TYPE_INTEGER:
            *bindSize = 12;
            break;

        case ANC_TYPE_BIGINT:
        case ANC_TYPE_FLOAT:
        case ANC_TYPE_DOUBLE:
            *bindSize = 21;
            break;

        case ANC_TYPE_NUMBER:
            *bindSize = 1000;
            break;

        case ANC_TYPE_BIT:
            *bindSize = desc->size + 1;
            break;

        case ANC_TYPE_ROWID:
            *bindSize = 24;
            break;

        default:
            *bindSize = 20;
            break;
    }
}

AncResult anpCursorPerformBind(AnpCursor* cursor)
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
                if (anpBindVar((AnpVar*)var, cursor, key, 0) < 0) {
                    return ANC_ERROR;
                }
            }
        } else {
            for (i = 0; i < PyList_GET_SIZE(cursor->bindVariables); i++) {
                var = PyList_GET_ITEM(cursor->bindVariables, i);
                if (var != Py_None) {
                    if (anpBindVar((AnpVar*)var, cursor, NULL, i) < 0) {
                        return ANC_ERROR;
                    }
                }
            }
        }
    }
    return ANC_SUCCESS;
}

static int anpCursorPerformDefine(AnpCursor* cursor, AncUint32 numQueryColumns)
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

    AncUint32     pos;
    AncColumnDesc queryInfo;
    // create a variable for each of the query columns
    cursor->fetchArraySize = cursor->arraySize;
    for (pos = 0; pos < numQueryColumns; pos++) {
        // get query information for the column position
        if (ancDescribeCol2(cursor->hStmt, pos, &queryInfo) != ANC_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }

        AncUint32 size;
        anpGetColumnSize(&queryInfo, &size);

        AnpVar* var = anpNewVar(cursor, cursor->fetchArraySize, queryInfo.type, size, 0);
        if (var == NULL) {
            return -1;
        }

        PyList_SET_ITEM(cursor->fetchVariables, pos, (PyObject*)var);
        if (ancBindColumn(cursor->hStmt, pos, var->transType, var->data, size, var->indicator) != ANC_SUCCESS) {
            return anpRaiseAndReturnIntException();
        }
    }

    return 0;
}

static PyObject* anpCursorExecute(AnpCursor* cursor, PyObject* args, PyObject* keywordArgs)
{
    PyObject *statement, *executeArgs;
    AncInt16  numQueryColumns;
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

    if (statement == cursor->statment) {
        statement = cursor->statment;
    } else {
        Py_CLEAR(cursor->statment);
        Py_XINCREF(statement);
        cursor->statment = statement;
        Py_CLEAR(cursor->fetchVariables);

        // prepare the statement, if applicable
        if (ancPrepare(cursor->hStmt, PyBytes_AsString(PyUnicode_AsUTF8String(statement))) != ANC_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
        if (ancGetStmtAttr(cursor->hStmt, ANC_ATTR_SQLTYPE, &cursor->sqlType, sizeof(cursor->sqlType)) != ANC_SUCCESS) {
            return anpRaiseAndReturnNullException();
        }
    }
    if (ancNumResultCols(cursor->hStmt, &numQueryColumns) != ANC_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }

    // perform binds
    if (executeArgs && anpCursorSetBindVariables(cursor, executeArgs, 1, 0, 0) < 0) {
        return NULL;
    }
    if (anpCursorPerformBind(cursor) < 0) {
        return NULL;
    }

    // execute the statement
    Py_BEGIN_ALLOW_THREADS
     status = ancExecute(cursor->hStmt);
    Py_END_ALLOW_THREADS
    if (status != ANC_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }

    if (numQueryColumns == 0) {
        if (cursor->sqlType >= ANC_SQLTYPE_CREATE_DATABASE) {
            cursor->rowCount = 1;
            Py_RETURN_NONE;
        }
        // get the count of the rows affected
        if (ancGetStmtAttr(cursor->hStmt, ANC_ATTR_ROWS_AFFECTED, &cursor->rowCount, sizeof(AncUint64))
            != ANC_SUCCESS) {
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
    if (anpCursorIsOpen(cursor)) {
        ancFreeHandle(ANC_HANDLE_STMT, cursor->hStmt);
    }
    cursor->hStmt = NULL;
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
        item = anpVarGetSingleValue(var, pos);
        if (item == NULL) {
            Py_DECREF(tuple);
            return NULL;
        }

        PyTuple_SET_ITEM(tuple, i, item);
    }

    return tuple;
}

static AncResult anpCursorCheck(AnpCursor* cursor)
{
    if (!anpCursorIsOpen(cursor)) {
        return ANC_ERROR;
    }
    if (cursor->fetchVariables == NULL) {
        anpRaiseExceptionFromString(anpInterfaceErrorException, "not a query");
        return ANC_ERROR;
    }
    return ANC_SUCCESS;
}

static PyObject* anpCursorFetchOne(AnpCursor* cursor, PyObject* args)
{
    if (anpCursorCheck(cursor) != ANC_SUCCESS) {
        return NULL;
    }
    
    AncUint32 rows;
    if (ancFetch(cursor->hStmt, &rows) != ANC_SUCCESS) {
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

static PyObject* anpCursorFetchMany(AnpCursor* cursor, PyObject* args)
{
    return anpRaiseExceptionFromString(anpNotSupportedException, "fetchmany() not implement");
}

static PyObject* anpCursorFetchAll(AnpCursor* cursor, PyObject* args)
{
    return anpRaiseExceptionFromString(anpNotSupportedException, "fetchall() not implement");
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
    if (anpCursorCheck(cursor) != ANC_SUCCESS) {
        return NULL;
    }

    Py_INCREF(cursor);
    return (PyObject*)cursor;
}

static PyObject* anpCursorNext(AnpCursor * cursor)
{
    if (anpCursorCheck(cursor) != ANC_SUCCESS) {
        return NULL;
    }

    AncUint32 rows;
    if (ancFetch(cursor->hStmt, &rows) != ANC_SUCCESS) {
        return anpRaiseAndReturnNullException();
    }
    printf("anpCursorNext:%d\n", rows);
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
        .tp_name = "anchor_python.Cursor",
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

AncResult anpRegistCursor(PyObject *module)
{
    PyType_Ready(&anchorPyTypeCursor);

    Py_INCREF(&anchorPyTypeCursor);
    if (PyModule_AddObject(module, "Cursor", (PyObject *) &anchorPyTypeCursor) < 0) {
        return ANC_ERROR;
    }
    return ANC_SUCCESS;
}

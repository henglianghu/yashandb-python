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

YacResult anpInitDecimal()
{
    PyObject *module;

    PyDateTime_IMPORT;
    if (PyErr_Occurred()) {
        return YAC_ERROR;
    }
    anpPyTypeDate = PyDateTimeAPI->DateType;
    anpPyTypeDateTime = PyDateTimeAPI->DateTimeType;

    module = PyImport_ImportModule("decimal");
    if (module == NULL) {
        return YAC_ERROR;
    }
    anpPyTypeDecimal = (PyTypeObject*) PyObject_GetAttrString(module, "Decimal");
    Py_DECREF(module);
    if (anpPyTypeDecimal == NULL) {
        return YAC_ERROR;
    }
    return YAC_SUCCESS;
}

YacResult anpRegisteVarObject(PyObject* module)
{
    PyType_Ready(&anchorPyTypeVar);

    Py_INCREF(&anchorPyTypeVar);
    if (PyModule_AddObject(module, "Var", (PyObject*) &anchorPyTypeVar) < 0) {
        return YAC_ERROR;
    }
    return YAC_SUCCESS;
}

AnpVar* anpNewVar(AnpCursor* cursor, Py_ssize_t numElements, YacType type, Py_ssize_t size, YacBool isArray)
{
    AnpVar* var = (AnpVar*) anchorPyTypeVar.tp_alloc(&anchorPyTypeVar, 0);
    if (var == NULL) {
        return NULL;
    }

    Py_INCREF(cursor->connection);
    var->connection = cursor->connection;

    var->size = (YacUint32)size;
    var->elements = (YacUint32)numElements;
    var->isArray = isArray;
    var->bufferSize = var->size * var->elements;
    var->dbType = type;
    if(type == YAC_TYPE_NUMBER){
        var->transType = YAC_TYPE_VARCHAR;
    } else {
        var->transType = type;
    }
    var->data = PyMem_Malloc(var->bufferSize);
    if (var->data == NULL) {
        Py_DECREF(var);
        return (AnpVar*)PyErr_NoMemory();
    }
    var->indicator = PyMem_Malloc(var->elements * sizeof(YacInt32));
    if (var->indicator == NULL) {
        Py_DECREF(var);
        return (AnpVar*)PyErr_NoMemory();
    }

    return var;
}

YacBool anpCheckVar(PyObject* object)
{
    return (Py_TYPE(object) == &anchorPyTypeVar);
}

static PyObject *anpVarToPython(YacType type, YacChar* data)
{
    YacChar message[120];
    PyObject* result;
    YacDate *date;
    YacTimestamp *timestamp;
    YacDateStruct ds;

    switch (type) {
        case YAC_TYPE_TINYINT:
            result = PyLong_FromLong(*(YacInt8*)data);
            break;
        case YAC_TYPE_SMALLINT:
            result = PyLong_FromLong(*(YacInt16*)data);
            break;
        case YAC_TYPE_INTEGER:
            result =  PyLong_FromLong(*(YacInt32*)data);
            break;
        case YAC_TYPE_BIGINT:
            result =  PyLong_FromLongLong(*(YacInt64*)data);
            break;
        case YAC_TYPE_FLOAT:
            result =  PyFloat_FromDouble(*(float *)data);
            break;
        case YAC_TYPE_DOUBLE:
            result =  PyFloat_FromDouble(*(double *)data);
            break;
        case YAC_TYPE_NUMBER: {
            PyObject* stringObj = PyUnicode_Decode(data, strlen(data), NULL, NULL);
            result = PyObject_CallFunctionObjArgs((PyObject*)anpPyTypeDecimal, stringObj, NULL);
            Py_DECREF(stringObj);
            break;
        }
        case YAC_TYPE_DATE:
            date = (YacDate *)data;
            yacGetDateStruct(*date, &ds);
            result =  PyDateTime_FromDateAndTime(ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second, ds.fraction/1000);
            break;
        case YAC_TYPE_TIMESTAMP:
        case YAC_TYPE_TIMESTAMP_TZ:
        case YAC_TYPE_TIMESTAMP_LTZ:
            timestamp = (YacTimestamp*)data;
            yacGetDateStruct(timestamp->stamp, &ds);
            result =  PyDateTime_FromDateAndTime(ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second, ds.fraction/1000);
            break;
 
        case YAC_TYPE_CHAR:
        case YAC_TYPE_NCHAR:
        case YAC_TYPE_VARCHAR:
        case YAC_TYPE_NVARCHAR:
            result = PyUnicode_FromString(data);
            break;
        default:
            snprintf(message, 120, "not support type %d", type);
            result = anpRaiseExceptionFromString(anpNotSupportedException, message);
            break;
    }
    return result;
}

PyObject* anpVarGetSingleValue(AnpVar* var, YacUint32 pos)
{
    if (pos > 1) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "AnpVar not support multi value");
    }
    if (var->indicator[pos] == YAC_NULL_DATA) {
        Py_RETURN_NONE;
    }
    return anpVarToPython(var->dbType, var->data + pos * var->size);
}

int anpBindVar(AnpVar* var, AnpCursor* cursor, PyObject* name, uint32_t pos)
{
    if (name) {
        anpRaiseExceptionFromString(anpNotSupportedException, "not support bind by name");
        return -1;
    }
    if (yacBindParameter(cursor->hStmt, pos, YAC_PARAM_INPUT, var->dbType, var->data, var->size, var->bufferSize, var->indicator) !=
        YAC_SUCCESS) {
        return -1;
    }
    return 0;
}

int anpVarSetValue(AnpVar* var, uint32_t arrayPos, PyObject* value)
{
    if (value == Py_None){
        var->indicator[arrayPos] = YAC_NULL_DATA;
    }
    if (PyBool_Check(value)) {
        YacBool* b = (YacBool *)var->data;
        b[arrayPos] = PyObject_IsTrue(value);
        var->indicator[arrayPos] = (YacInt32)sizeof(YacBool);
        return 0;
    }
    if (PyUnicode_Check(value)) {
        PyObject* tmp = PyUnicode_AsEncodedString(value, "utf8", NULL);
        if (tmp == NULL) {
            return -1;
        }
        strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(tmp));
        var->indicator[arrayPos] = (int)PyUnicode_GET_LENGTH(value);
        return 0;
    }
    if (PyBytes_Check(value)) {
        strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(value));
        var->indicator[arrayPos] = (int)PyBytes_GET_SIZE(value);
        return 0;
    }
    if (PyLong_Check(value)) {
        YacInt32 *iv = (YacInt32 *)var->data;
        iv[arrayPos] = PyLong_AsLong(value);
        var->indicator[arrayPos] = (YacInt32)sizeof(YacInt64);
        return 0;
    }
    if (PyFloat_Check(value)) {
        double *dv = (double *)var->data;
        dv[arrayPos] = PyFloat_AsDouble(value);
        var->indicator[arrayPos] = (YacInt32)sizeof(double);
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
        return (int)PyUnicode_GET_LENGTH(value) + 1;
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

YacType anpGetType(PyObject * value)
{
    if (value == Py_None) {
        return 0;
    }
    if (PyBool_Check(value)) {
        return YAC_TYPE_BOOL;
    }
    if (PyUnicode_Check(value)) {
        return YAC_TYPE_VARCHAR;
    }
    if (PyBytes_Check(value)) {
        return YAC_TYPE_VARCHAR;
    }
    if (PyLong_Check(value)) {
        return YAC_TYPE_INTEGER;
    }
    if (PyFloat_Check(value)) {
        return YAC_TYPE_DOUBLE;
    }

    return 0;
}

AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements)
{
    int isArray = 0;
    Py_ssize_t size = 0;
    Py_ssize_t i = 0;
    Py_ssize_t tempSize = 0;
    YacType type = 0;
    char message[250];

    if (PyList_Check(value)) {
        isArray = 1;
        for (i = 0; i < PyList_GET_SIZE(value); i++) {
            PyObject * obj = PyList_GET_ITEM(value, i);

            YacType tmpType = anpGetType(obj);
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

#include "anp_var.h"
#include "anp_exception.h"

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
    .tp_name = "anchor_python.Var",
    .tp_basicsize = sizeof(AnpVar),
    .tp_dealloc = (destructor)anpVarFree,
    .tp_repr = (reprfunc)anpVarRepr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = anpCalcMembers};

static PyTypeObject * anpPyTypeDecimal;

AncResult anpInitDecimal()
{
    PyObject *module;
    // import the decimal module for decimal support
    module = PyImport_ImportModule("decimal");
    if (module == NULL) {
        return ANC_ERROR;
    }
    anpPyTypeDecimal = (PyTypeObject*) PyObject_GetAttrString(module, "Decimal");
    Py_DECREF(module);
    if (anpPyTypeDecimal == NULL) {
        return ANC_ERROR;
    }
    return ANC_SUCCESS;
}

AncResult anpRegisteVarObject(PyObject* module)
{
    PyType_Ready(&anchorPyTypeVar);

    Py_INCREF(&anchorPyTypeVar);
    if (PyModule_AddObject(module, "Var", (PyObject*) &anchorPyTypeVar) < 0) {
        return ANC_ERROR;
    }
    return ANC_SUCCESS;
}

AnpVar* anpNewVar(AnpCursor* cursor, Py_ssize_t numElements, AncType type, Py_ssize_t size, AncBool isArray)
{
    AnpVar* var = (AnpVar*) anchorPyTypeVar.tp_alloc(&anchorPyTypeVar, 0);
    if (var == NULL) {
        return NULL;
    }

    Py_INCREF(cursor->connection);
    var->connection = cursor->connection;

    var->size = (AncUint32)size;
    var->elements = (AncUint32)numElements;
    var->isArray = isArray;
    var->bufferSize = var->size * var->elements;
    var->dbType = type;
    if(type == ANC_TYPE_NUMBER){
        var->transType = ANC_TYPE_VARCHAR;
    } else {
        var->transType = type;
    }
    var->data = PyMem_Malloc(var->bufferSize);
    if (var->data == NULL) {
        Py_DECREF(var);
        return (AnpVar*)PyErr_NoMemory();
    }
    var->indicator = PyMem_Malloc(var->elements * sizeof(AncInt32));
    if (var->indicator == NULL) {
        Py_DECREF(var);
        return (AnpVar*)PyErr_NoMemory();
    }

    return var;
}

AncBool anpCheckVar(PyObject* object)
{
    return (Py_TYPE(object) == &anchorPyTypeVar);
}

static PyObject *anpVarToPython(AncType type, AncChar* data)
{
    AncChar message[120];
    PyObject* result;

    switch (type) {
        case ANC_TYPE_TINYINT:
            result = PyLong_FromLong(*(AncInt8*)data);
            break;
        case ANC_TYPE_SMALLINT:
            result = PyLong_FromLong(*(AncInt16*)data);
            break;
        case ANC_TYPE_INTEGER:
            result =  PyLong_FromLong(*(AncInt32*)data);
            break;
        case ANC_TYPE_BIGINT:
            result =  PyLong_FromLongLong(*(AncInt64*)data);
            break;
        case ANC_TYPE_FLOAT:
            result =  PyFloat_FromDouble(*(float *)data);
            break;
        case ANC_TYPE_DOUBLE:
            result =  PyFloat_FromDouble(*(double *)data);
            break;
        case ANC_TYPE_NUMBER: {
            PyObject* stringObj = PyUnicode_Decode(data, strlen(data), NULL, NULL);
            result = PyObject_CallFunctionObjArgs((PyObject*)anpPyTypeDecimal, stringObj, NULL);
            Py_DECREF(stringObj);
            break;
        }
        case ANC_TYPE_DATE:
            result =  PyLong_FromLongLong(*(AncInt64*)data);
            break;
        case ANC_TYPE_CHAR:
        case ANC_TYPE_NCHAR:
        case ANC_TYPE_VARCHAR:
        case ANC_TYPE_NVARCHAR:
            result = PyBytes_FromString(data);
            break;
        default:
            snprintf(message, 120, "not support type %d", type);
            result = anpRaiseExceptionFromString(anpNotSupportedException, message);
            break;
    }
    return result;
}

PyObject* anpVarGetSingleValue(AnpVar* var, AncUint32 pos)
{
    if (pos > 1) {
        return anpRaiseExceptionFromString(anpNotSupportedException, "AnpVar not support multi value");
    }
    if (var->indicator[pos] == ANC_NULL_DATA) {
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
    if (ancBindParameter(cursor->hStmt, pos, ANC_PARAM_INPUT, var->dbType, var->data, var->size, var->indicator) !=
        ANC_SUCCESS) {
        return -1;
    }
    return 0;
}

int anpVarSetValue(AnpVar* var, uint32_t arrayPos, PyObject* value)
{
    if (value == Py_None){
        var->indicator[arrayPos] = ANC_NULL_DATA;
    }
    if (PyBool_Check(value)) {
        AncBool* b = (AncBool *)var->data;
        b[arrayPos] = PyObject_IsTrue(value);
        return 0;
    }
    if (PyUnicode_Check(value)) {
        PyObject* tmp = PyUnicode_AsEncodedString(value, "utf8", NULL);
        if (tmp == NULL) {
            return -1;
        }
        strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(tmp));
        return 0;
    }
    if (PyBytes_Check(value)) {
        strcpy(var->data + var->size*arrayPos, PyBytes_AS_STRING(value));
        return 0;
    }
    if (PyLong_Check(value)) {
        AncInt32 *iv = (AncInt32 *)var->data;
        iv[arrayPos] = PyLong_AsLong(value);
        return 0;
    }
    if (PyFloat_Check(value)) {
        double *dv = (double *)var->data;
        dv[arrayPos] = PyFloat_AsDouble(value);
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
        return (int)PyUnicode_GET_LENGTH(value);
    }
    if (PyBytes_Check(value)) {
        return (int)PyBytes_GET_SIZE(value);
    }
    if (PyLong_Check(value)) {
        return 4;
    }
    if (PyFloat_Check(value)) {
        return 8;
    }

    return 0;
}

AncType anpGetType(PyObject * value)
{
    if (value == Py_None) {
        return 0;
    }
    if (PyBool_Check(value)) {
        return ANC_TYPE_BOOL;
    }
    if (PyUnicode_Check(value)) {
        return ANC_TYPE_VARCHAR;
    }
    if (PyBytes_Check(value)) {
        return ANC_TYPE_VARCHAR;
    }
    if (PyLong_Check(value)) {
        return ANC_TYPE_INTEGER;
    }
    if (PyFloat_Check(value)) {
        return ANC_TYPE_DOUBLE;
    }

    return 0;
}

AnpVar* anpVarNewByValue(AnpCursor* cursor, PyObject* value, Py_ssize_t numElements)
{
    int isArray = 0;
    Py_ssize_t size = 0;
    Py_ssize_t i = 0;
    Py_ssize_t tempSize = 0;
    AncType type = 0;
    char message[250];

    if (PyList_Check(value)) {
        isArray = 1;
        for (i = 0; i < PyList_GET_SIZE(value); i++) {
            PyObject * obj = PyList_GET_ITEM(value, i);

            AncType tmpType = anpGetType(obj);
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

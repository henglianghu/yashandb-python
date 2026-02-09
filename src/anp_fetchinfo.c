#include "anp_fetchinfo.h"
#include "structmember.h"
#include "anp_module.h"
#include "anp_api_type.h"

// YapiVectorFormat enum values
static PyObject *yaspyVectorFormatFlex = NULL;
static PyObject *yaspyVectorFormatFloat16 = NULL;
static PyObject *yaspyVectorFormatFloat32 = NULL;
static PyObject *yaspyVectorFormatFloat64 = NULL;
static PyObject *yaspyVectorFormatInt8 = NULL;

// Helper function to get vector format object from YapiVectorFormat enum
PyObject *anpGetVectorFormatObject(YapiVectorFormat format)
{
    switch (format) {
        case YAPI_VECTOR_FORMAT_FLEX:
            Py_INCREF(yaspyVectorFormatFlex);
            return yaspyVectorFormatFlex;
        case YAPI_VECTOR_FORMAT_FLOAT16:
            Py_INCREF(yaspyVectorFormatFloat16);
            return yaspyVectorFormatFloat16;
        case YAPI_VECTOR_FORMAT_FLOAT32:
            Py_INCREF(yaspyVectorFormatFloat32);
            return yaspyVectorFormatFloat32;
        case YAPI_VECTOR_FORMAT_FLOAT64:
            Py_INCREF(yaspyVectorFormatFloat64);
            return yaspyVectorFormatFloat64;
        case YAPI_VECTOR_FORMAT_INT8:
            Py_INCREF(yaspyVectorFormatInt8);
            return yaspyVectorFormatInt8;
        default:
            Py_RETURN_NONE;
    }
}

// Register FetchInfo and YapiVectorFormat enum values
int anpRegisterFetchInfo(PyObject *module)
{
    // Create YapiVectorFormat enum values
    yaspyVectorFormatFlex = PyLong_FromLong(YAPI_VECTOR_FORMAT_FLEX);
    if (yaspyVectorFormatFlex == NULL) {
        return -1;
    }
    if (PyModule_AddObject(module, "VECTOR_FORMAT_FLEX", yaspyVectorFormatFlex) < 0) {
        Py_DECREF(yaspyVectorFormatFlex);
        return -1;
    }
    
    yaspyVectorFormatFloat16 = PyLong_FromLong(YAPI_VECTOR_FORMAT_FLOAT16);
    if (yaspyVectorFormatFloat16 == NULL) {
        return -1;
    }
    if (PyModule_AddObject(module, "VECTOR_FORMAT_FLOAT16", yaspyVectorFormatFloat16) < 0) {
        Py_DECREF(yaspyVectorFormatFloat16);
        return -1;
    }
    
    yaspyVectorFormatFloat32 = PyLong_FromLong(YAPI_VECTOR_FORMAT_FLOAT32);
    if (yaspyVectorFormatFloat32 == NULL) {
        return -1;
    }
    if (PyModule_AddObject(module, "VECTOR_FORMAT_FLOAT32", yaspyVectorFormatFloat32) < 0) {
        Py_DECREF(yaspyVectorFormatFloat32);
        return -1;
    }
    
    yaspyVectorFormatFloat64 = PyLong_FromLong(YAPI_VECTOR_FORMAT_FLOAT64);
    if (yaspyVectorFormatFloat64 == NULL) {
        return -1;
    }
    if (PyModule_AddObject(module, "VECTOR_FORMAT_FLOAT64", yaspyVectorFormatFloat64) < 0) {
        Py_DECREF(yaspyVectorFormatFloat64);
        return -1;
    }
    
    yaspyVectorFormatInt8 = PyLong_FromLong(YAPI_VECTOR_FORMAT_INT8);
    if (yaspyVectorFormatInt8 == NULL) {
        return -1;
    }
    if (PyModule_AddObject(module, "VECTOR_FORMAT_INT8", yaspyVectorFormatInt8) < 0) {
        Py_DECREF(yaspyVectorFormatInt8);
        return -1;
    }
    
    // Register the FetchInfo type
    if (PyType_Ready(&anchorPyTypeFetchInfo) < 0) {
        return -1;
    }
    
    Py_INCREF(&anchorPyTypeFetchInfo);
    if (PyModule_AddObject(module, "FetchInfo", (PyObject *)&anchorPyTypeFetchInfo) < 0) {
        Py_DECREF(&anchorPyTypeFetchInfo);
        return -1;
    }
    
    return 0;
}

// FetchInfo rich compare
static PyObject *
anpFetchInfoRichCompare(AnpFetchInfo *self, PyObject *other, int op)
{
    // Only handle equality comparison with sequences
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    
    // Check if other is a sequence
    if (!PySequence_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    
    // Check if sequence has the correct length (7 elements)
    Py_ssize_t len = PySequence_Length(other);
    if (len != 7) {
        if (op == Py_EQ) {
            Py_RETURN_FALSE;
        } else {
            Py_RETURN_TRUE;
        }
    }
    
    // Get items from sequence
    PyObject *other_items[7];
    for (int i = 0; i < 7; i++) {
        other_items[i] = PySequence_GetItem(other, i);
        if (other_items[i] == NULL) {
            // Clean up previously acquired items
            for (int j = 0; j < i; j++) {
                Py_DECREF(other_items[j]);
            }
            return NULL;  // Error occurred
        }
    }
    
    // Compare each element
    int comparisons[7];
    comparisons[0] = PyObject_RichCompareBool(self->name, other_items[0], Py_EQ);
    comparisons[1] = PyObject_RichCompareBool(self->type, other_items[1], Py_EQ);
    comparisons[2] = PyObject_RichCompareBool(self->display_size, other_items[2], Py_EQ);
    comparisons[3] = PyObject_RichCompareBool(self->internal_size, other_items[3], Py_EQ);
    comparisons[4] = PyObject_RichCompareBool(self->precision, other_items[4], Py_EQ);
    comparisons[5] = PyObject_RichCompareBool(self->scale, other_items[5], Py_EQ);
    comparisons[6] = PyObject_RichCompareBool(self->null_ok, other_items[6], Py_EQ);
    
    // Clean up items
    for (int i = 0; i < 7; i++) {
        Py_DECREF(other_items[i]);
    }
    
    // Check for errors in comparisons
    for (int i = 0; i < 7; i++) {
        if (comparisons[i] < 0) {
            return NULL;  // Error occurred
        }
    }
    
    int all_equal = 1;
    for (int i = 0; i < 7; i++) {
        if (!comparisons[i]) {
            all_equal = 0;
            break;
        }
    }
    
    if (op == Py_EQ) {
        return PyBool_FromLong(all_equal);
    } else {
        return PyBool_FromLong(!all_equal);
    }
}

// FetchInfo repr
static PyObject *
anpFetchInfoRepr(AnpFetchInfo *self)
{
    return PyUnicode_FromFormat(
        "FetchInfo(name=%R, type=%R, display_size=%R, internal_size=%R, precision=%R, scale=%R, null_ok=%R, vector_dimension=%R, vector_format=%R)",
        self->name, self->type, self->display_size, self->internal_size, self->precision, self->scale, self->null_ok, self->vector_dimension, self->vector_format);
}

// FetchInfo str
static PyObject *
anpFetchInfoStr(AnpFetchInfo *self)
{
    return anpFetchInfoRepr(self);
}

// FetchInfo dealloc
static void
anpFetchInfoDealloc(AnpFetchInfo *self)
{
    Py_XDECREF(self->name);
    Py_XDECREF(self->type);
    Py_XDECREF(self->display_size);
    Py_XDECREF(self->internal_size);
    Py_XDECREF(self->precision);
    Py_XDECREF(self->scale);
    Py_XDECREF(self->null_ok);
    Py_XDECREF(self->vector_dimension);
    Py_XDECREF(self->vector_format);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

// FetchInfo new
static PyObject *
anpFetchInfoNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    AnpFetchInfo *self;
    self = (AnpFetchInfo *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->name = Py_None;
        Py_INCREF(Py_None);
        self->type = Py_None;
        Py_INCREF(Py_None);
        self->display_size = Py_None;
        Py_INCREF(Py_None);
        self->internal_size = Py_None;
        Py_INCREF(Py_None);
        self->precision = Py_None;
        Py_INCREF(Py_None);
        self->scale = Py_None;
        Py_INCREF(Py_None);
        self->null_ok = Py_None;
        Py_INCREF(Py_None);
        self->vector_dimension = Py_None;
        Py_INCREF(Py_None);
        self->vector_format = Py_None;
        Py_INCREF(Py_None);
    }
    return (PyObject *)self;
}

// FetchInfo init
static int
anpFetchInfoInit(AnpFetchInfo *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "type", "display_size", "internal_size", "precision", "scale", "null_ok", "vector_dimension", "vector_format", NULL};
    
    PyObject *name = NULL, *type = NULL, *display_size = NULL, *internal_size = NULL;
    PyObject *precision = NULL, *scale = NULL, *null_ok = NULL;
    PyObject *vector_dimension = NULL, *vector_format = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOOOOOO|OO", kwlist,
                                     &name, &type, &display_size, &internal_size,
                                     &precision, &scale, &null_ok,
                                     &vector_dimension, &vector_format))
        return -1;
    
    // Set name
    Py_XDECREF(self->name);
    Py_INCREF(name);
    self->name = name;
    
    // Set type
    Py_XDECREF(self->type);
    Py_INCREF(type);
    self->type = type;
    
    // Set display_size
    Py_XDECREF(self->display_size);
    Py_INCREF(display_size);
    self->display_size = display_size;
    
    // Set internal_size
    Py_XDECREF(self->internal_size);
    Py_INCREF(internal_size);
    self->internal_size = internal_size;
    
    // Set precision
    Py_XDECREF(self->precision);
    Py_INCREF(precision);
    self->precision = precision;
    
    // Set scale
    Py_XDECREF(self->scale);
    Py_INCREF(scale);
    self->scale = scale;
    
    // Set null_ok
    Py_XDECREF(self->null_ok);
    Py_INCREF(null_ok);
    self->null_ok = null_ok;
    
    // Set vector_dimension (can be None for non-vector types)
    Py_XDECREF(self->vector_dimension);
    if (vector_dimension != NULL) {
        Py_INCREF(vector_dimension);
        self->vector_dimension = vector_dimension;
    } else {
        self->vector_dimension = Py_None;
        Py_INCREF(Py_None);
    }
    
    // Set vector_format (can be None for non-vector types)
    Py_XDECREF(self->vector_format);
    if (vector_format != NULL) {
        Py_INCREF(vector_format);
        self->vector_format = vector_format;
    } else {
        self->vector_format = Py_None;
        Py_INCREF(Py_None);
    }
    
    return 0;
}

// FetchInfo length (sequence protocol)
static Py_ssize_t
anpFetchInfoLength(AnpFetchInfo *self)
{
    return 7;  // Always 7 elements to match the original tuple
}

// FetchInfo item (sequence protocol)
static PyObject *
anpFetchInfoItem(AnpFetchInfo *self, Py_ssize_t i)
{
    // Handle negative indices
    if (i < 0) {
        i += 7;  // Convert negative index to positive
    }
    
    // Check bounds
    if (i < 0 || i >= 7) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }
    
    switch (i) {
        case 0:
            Py_INCREF(self->name);
            return self->name;
        case 1:
            Py_INCREF(self->type);
            return self->type;
        case 2:
            Py_INCREF(self->display_size);
            return self->display_size;
        case 3:
            Py_INCREF(self->internal_size);
            return self->internal_size;
        case 4:
            Py_INCREF(self->precision);
            return self->precision;
        case 5:
            Py_INCREF(self->scale);
            return self->scale;
        case 6:
            Py_INCREF(self->null_ok);
            return self->null_ok;
        default:
            PyErr_SetString(PyExc_IndexError, "index out of range");
            return NULL;
    }
}

// FetchInfo sq (sequence protocol)
static PySequenceMethods anpFetchInfoSeq = {
    .sq_length = (lenfunc)anpFetchInfoLength,
    .sq_item = (ssizeargfunc)anpFetchInfoItem,
};

// FetchInfo methods
static PyMethodDef anpFetchInfoMethods[] = {
    {NULL}  /* Sentinel */
};


// FetchInfo members
static PyMemberDef anpFetchInfoMembers[] = {
    {"name", T_OBJECT_EX, offsetof(AnpFetchInfo, name), READONLY, "column name"},
    {"type", T_OBJECT_EX, offsetof(AnpFetchInfo, type), READONLY, "type code"},
    {"display_size", T_OBJECT_EX, offsetof(AnpFetchInfo, display_size), READONLY, "display size"},
    {"internal_size", T_OBJECT_EX, offsetof(AnpFetchInfo, internal_size), READONLY, "internal size"},
    {"precision", T_OBJECT_EX, offsetof(AnpFetchInfo, precision), READONLY, "precision"},
    {"scale", T_OBJECT_EX, offsetof(AnpFetchInfo, scale), READONLY, "scale"},
    {"null_ok", T_OBJECT_EX, offsetof(AnpFetchInfo, null_ok), READONLY, "null ok"},
    {"vector_dimension", T_OBJECT_EX, offsetof(AnpFetchInfo, vector_dimension), READONLY, "vector dimension"},
    {"vector_format", T_OBJECT_EX, offsetof(AnpFetchInfo, vector_format), READONLY, "vector format"},
    {NULL}  /* Sentinel */
};

// FetchInfo type (placed at the end as requested)
PyTypeObject anchorPyTypeFetchInfo = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "yaspy.FetchInfo",
    .tp_doc = "Column metadata information",
    .tp_basicsize = sizeof(AnpFetchInfo),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = anpFetchInfoNew,
    .tp_init = (initproc)anpFetchInfoInit,
    .tp_dealloc = (destructor)anpFetchInfoDealloc,
    .tp_repr = (reprfunc)anpFetchInfoRepr,
    .tp_str = (reprfunc)anpFetchInfoStr,
    .tp_richcompare = (richcmpfunc)anpFetchInfoRichCompare,
    .tp_as_sequence = &anpFetchInfoSeq,
    .tp_members = anpFetchInfoMembers,
    .tp_methods = anpFetchInfoMethods,
};

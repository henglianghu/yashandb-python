#include "anp_api_type.h"

yaspyApiType *yaspyApiTypeBool = NULL;
yaspyApiType *yaspyApiTypeTinyint = NULL;
yaspyApiType *yaspyApiTypeSmallint = NULL;
yaspyApiType *yaspyApiTypeInteger = NULL;
yaspyApiType *yaspyApiTypeBigint = NULL;
yaspyApiType *yaspyApiTypeFloat = NULL;
yaspyApiType *yaspyApiTypeDouble = NULL;
yaspyApiType *yaspyApiTypeNumber = NULL;
yaspyApiType *yaspyApiTypeDate = NULL;
yaspyApiType *yaspyApiTypeTime = NULL;
yaspyApiType *yaspyApiTypeDatetime = NULL;
yaspyApiType *yaspyApiTypeTimedelta = NULL;
yaspyApiType *yaspyApiTypeChar = NULL;
yaspyApiType *yaspyApiTypeVarchar = NULL;
yaspyApiType *yaspyApiTypeNchar = NULL;
yaspyApiType *yaspyApiTypeNvarchar = NULL;
yaspyApiType *yaspyApiTypeBinary = NULL;
yaspyApiType *yaspyApiTypeBit = NULL;
yaspyApiType *yaspyApiTypeRowid = NULL;
yaspyApiType *yaspyApiTypeJson = NULL;
yaspyApiType *yaspyApiTypeNone = NULL;
yaspyApiType *yaspyApiTypeYeardelta = NULL;
yaspyApiType *yaspyApiTypeBlob = NULL;
yaspyApiType *yaspyApiTypeClob = NULL;
yaspyApiType *yaspyApiTypeNclob = NULL;

static void yaspyApiTypeFree(yaspyApiType *apiType)
{
    Py_TYPE(apiType)->tp_free((PyObject*) apiType);
}

YapiResult anpRegisterApiType(PyObject *module)
{
    PyType_Ready(&yasPyTypeApiType);

    if (yaspyModuleAddApiType(module, "BOOL", YAPI_TYPE_BOOL, &yaspyApiTypeBool) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "BYTE", YAPI_TYPE_TINYINT, &yaspyApiTypeTinyint) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "SHORT", YAPI_TYPE_SMALLINT, &yaspyApiTypeSmallint) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "INTEGER", YAPI_TYPE_INTEGER, &yaspyApiTypeInteger) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "BIGINT", YAPI_TYPE_BIGINT, &yaspyApiTypeBigint) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "FLOAT", YAPI_TYPE_FLOAT, &yaspyApiTypeFloat) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "DOUBLE", YAPI_TYPE_DOUBLE, &yaspyApiTypeDouble) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "NUMBER", YAPI_TYPE_NUMBER, &yaspyApiTypeNumber) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "DATE", YAPI_TYPE_DATE, &yaspyApiTypeDate) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "TIME", YAPI_TYPE_SHORTTIME, &yaspyApiTypeTime) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "DATETIME", YAPI_TYPE_TIMESTAMP, &yaspyApiTypeDatetime) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "TIMEDELTA", YAPI_TYPE_DS_INTERVAL, &yaspyApiTypeTimedelta) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "CHAR", YAPI_TYPE_CHAR, &yaspyApiTypeChar) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "VARCHAR", YAPI_TYPE_VARCHAR, &yaspyApiTypeVarchar) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "NCHAR", YAPI_TYPE_NCHAR, &yaspyApiTypeNchar) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "NVARCHAR", YAPI_TYPE_NVARCHAR, &yaspyApiTypeNvarchar) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "BINARY", YAPI_TYPE_BINARY, &yaspyApiTypeBinary) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "BIT", YAPI_TYPE_BIT, &yaspyApiTypeBit) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "ROWID", YAPI_TYPE_ROWID, &yaspyApiTypeRowid) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "JSON", YAPI_TYPE_JSON, &yaspyApiTypeJson) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "NONE", YAPI_TYPE_UNKNOWN, &yaspyApiTypeNone) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "YEARDELTA", YAPI_TYPE_YM_INTERVAL, &yaspyApiTypeYeardelta) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "BLOB", YAPI_TYPE_BLOB, &yaspyApiTypeBlob) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "CLOB", YAPI_TYPE_CLOB, &yaspyApiTypeClob) < 0) {
        return YAPI_ERROR;
    }
    if (yaspyModuleAddApiType(module, "NCLOB", YAPI_TYPE_NCLOB, &yaspyApiTypeNclob) < 0) {
        return YAPI_ERROR;
    }
    return YAPI_SUCCESS;
}

// dbapi type declaration
PyTypeObject yasPyTypeApiType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "yaspy.ApiType",
    .tp_basicsize = sizeof(yaspyApiType),
    .tp_dealloc = (destructor) yaspyApiTypeFree,
};
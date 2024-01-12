#ifndef ANCHOR_ANP_CURSOR_H
#define ANCHOR_ANP_CURSOR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_connection.h"

typedef struct StAnpCursor {
    PyObject_HEAD
    YapiStmt* hStmt;
    AnpConnection *connection;
    // list of AnpVar's as bind parameters
    PyObject *bindVariables;
    PyObject* fetchVariables;
    PyObject* statment;

    uint32_t sqlType;
    uint32_t arraySize;
    uint32_t setInputSizes;
    uint32_t fetchArraySize;
    uint64_t rowCount;

    bool isOpen;
    bool isFail;
} AnpCursor;

YapiResult anpRegistCursor(PyObject* module);
extern PyTypeObject anchorPyTypeCursor;
#endif //ANCHOR_ANP_CURSOR_H

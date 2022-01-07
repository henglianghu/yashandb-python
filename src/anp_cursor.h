#ifndef ANCHOR_ANP_CURSOR_H
#define ANCHOR_ANP_CURSOR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_connection.h"

typedef struct StAnpCursor {
    PyObject_HEAD
    YacHandle hStmt;
    AnpConnection *connection;
    PyObject *bindVariables;
    PyObject* fetchVariables;
    PyObject* statment;

    YacUint32 sqlType;
    YacUint32 arraySize;
    YacUint32 setInputSizes;
    YacUint32 fetchArraySize;
    YacUint64 rowCount;

    YacBool isOpen;
} AnpCursor;

YacResult anpRegistCursor(PyObject* module);
extern PyTypeObject anchorPyTypeCursor;
#endif //ANCHOR_ANP_CURSOR_H

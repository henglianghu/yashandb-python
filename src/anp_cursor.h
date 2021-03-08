#ifndef ANCHOR_ANP_CURSOR_H
#define ANCHOR_ANP_CURSOR_H

#include "Python.h"
#include "anp_cli.h"
#include "anp_connection.h"

typedef struct StAnpCursor {
    PyObject_HEAD
    AncHandle hStmt;
    AnpConnection *connection;
    PyObject *bindVariables;
    PyObject* fetchVariables;
    PyObject* statment;

    AncUint32 sqlType;
    AncUint32 arraySize;
    AncUint32 setInputSizes;
    AncUint32 fetchArraySize;
    AncUint64 rowCount;

    AncBool isOpen;
} AnpCursor;

AncResult anpRegistCursor(PyObject* module);
extern PyTypeObject anchorPyTypeCursor;
#endif //ANCHOR_ANP_CURSOR_H

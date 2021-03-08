
#ifndef ANCHOR_ANP_CONNECTION_H
#define ANCHOR_ANP_CONNECTION_H

#include "Python.h"
#include "anp_cli.h"

typedef struct {
    PyObject_HEAD
    AncHandle hConn;
    AncHandle hEnv;
    AncHandle hStmt;
    PyObject *username;
    PyObject *dsn;

    AncBool autocommit;
} AnpConnection;

AncResult anpRegistConnection(PyObject* module);
AncBool anpConnectionIsConnected(AnpConnection *conn);

extern PyTypeObject anchorPyTypeConnection;
#endif //ANCHOR_ANP_CONNECTION_H

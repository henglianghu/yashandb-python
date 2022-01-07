
#ifndef ANCHOR_ANP_CONNECTION_H
#define ANCHOR_ANP_CONNECTION_H

#include "Python.h"
#include "anp_cli.h"

typedef struct {
    PyObject_HEAD
    YacHandle hConn;
    YacHandle hEnv;
    YacHandle hStmt;
    PyObject *username;
    PyObject *dsn;

    YacBool autocommit;
} AnpConnection;

YacResult anpRegistConnection(PyObject* module);
YacBool anpConnectionIsConnected(AnpConnection *conn);

extern PyTypeObject anchorPyTypeConnection;
#endif //ANCHOR_ANP_CONNECTION_H

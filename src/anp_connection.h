
#ifndef ANCHOR_ANP_CONNECTION_H
#define ANCHOR_ANP_CONNECTION_H

#include "Python.h"
#include "anp_cli.h"

typedef struct {
    PyObject_HEAD
    YapiConnect *hConn;
    PyObject *username;
    PyObject *dsn;

    bool autocommit;
} AnpConnection;

YapiResult anpRegistConnection(PyObject* module);
bool anpConnectionIsConnected(AnpConnection *conn);

extern PyTypeObject anchorPyTypeConnection;

#endif //ANCHOR_ANP_CONNECTION_H

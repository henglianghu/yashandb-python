
#ifndef ANCHOR_ANP_CONNECTION_H
#define ANCHOR_ANP_CONNECTION_H

#include "Python.h"
#include "anp_cli.h"

typedef struct {
    PyObject_HEAD
    YapiConnect *hConn;
    YapiConnectPool *hConnPool;
    PyObject *username;
    PyObject *dsn;

    bool autocommit;
} AnpConnection;

YapiResult anpRegistConnection(PyObject* module);
int        anpGetModule(PyTypeObject* type, PyObject** module, PyObject** name);
bool       anpConnectionIsConnected(AnpConnection* conn);
PyObject*  anpNewConnection(PyTypeObject* type, PyObject* args, PyObject* keywordArgs);

extern PyTypeObject anchorPyTypeConnection;

#endif //ANCHOR_ANP_CONNECTION_H

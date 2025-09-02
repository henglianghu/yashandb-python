#ifndef ANCHOR_ANP_SESSION_POOL_H
#define ANCHOR_ANP_SESSION_POOL_H
#include "anp_module.h"

typedef struct AnpSessionPool AnpSessionPool;

struct AnpSessionPool {
    PyObject_HEAD
    YapiConnectPool *hConnPool;
    uint32_t  minSessions;
    uint32_t  maxSessions;
    uint32_t  sessionIncrement;
    uint32_t  mode;
    PyObject* username;
    PyObject* dsn;
    PyObject* password;
};

extern PyTypeObject anchorPyTypeSessionPool;

YapiResult anpRegistSessionPool(PyObject* module);

#endif // ANCHOR_ANP_SESSION_POOL_H

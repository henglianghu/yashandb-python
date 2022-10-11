#ifndef ANCHOR_ANP_EXCEPTION_H
#define ANCHOR_ANP_EXCEPTION_H

#include "Python.h"
#include "anp_cli.h"

typedef struct StAnpError {
    PyObject_HEAD
    int32_t code;
    uint32_t line;
    uint32_t column;
    PyObject *message;
    PyObject *sqlStat;
} AnpError;

extern PyObject *anpWarningException;
extern PyObject *anpErrorException;
extern PyObject *anpInterfaceErrorException;
extern PyObject *anpDatabaseErrorException;
extern PyObject *anpDataErrorException;
extern PyObject *anpOperationalErrorException;
extern PyObject *anpIntegrityErrorException;
extern PyObject *anpInternalErrorException;
extern PyObject *anpProgrammingErrorException;
extern PyObject *anpNotSupportedException;

YapiResult anpRegisterException(PyObject *module);

AnpError* anpExceptionNewFromInfo(YapiErrorInfo* info);
int       anpRaiseAndReturnIntException(void);
int       anpRaiseExceptionFromInfo(YapiErrorInfo* info);
PyObject* anpRaiseAndReturnNullException(void);
PyObject* anpRaiseExceptionFromString(PyObject *exceptionType, const char *message);


#endif  // ANCHOR_ANP_EXCEPTION_H

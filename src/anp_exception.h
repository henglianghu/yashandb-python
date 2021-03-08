#ifndef ANCHOR_ANP_EXCEPTION_H
#define ANCHOR_ANP_EXCEPTION_H

#include "Python.h"
#include "anp_cli.h"

typedef struct StAnpError {
    PyObject_HEAD
    AncInt32 code;
    AncUint32 line;
    AncUint32 column;
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

AncResult anpRegisterException(PyObject *module);

AnpError* anpExceptionNewFromInfo(AncUint32 code, const char * message, const char* sqlStat, AncTextPos *pos);
int       anpRaiseAndReturnIntException(void);
int       anpRaiseExceptionFromInfo(AncUint32 code, const char * message, const char* sqlStat, AncTextPos *pos);
PyObject* anpRaiseAndReturnNullException(void);
PyObject* anpRaiseExceptionFromString(PyObject *exceptionType, const char *message);


#endif  // ANCHOR_ANP_EXCEPTION_H

#ifndef ANCHOR_ANP_CLI_H
#define ANCHOR_ANP_CLI_H

#include "yacapi.h"

extern YapiEnv* anpEnv;

static inline uint32_t codSizeAlign4(uint32_t size)
{
    uint32_t margin = (size & (uint32_t)0x03);
    return (margin == 0) ? size : size + (4 - margin);
}

#define CONVERT_TO_LOB_SIZE (32000)
#define PROCEDURE_PARAM_LIST_BUFFER_SIZE (512)
#define PROCEDURE_SQL_BUFFER_SIZE (1024)

#endif  // ANCHOR_ANP_VAR_H

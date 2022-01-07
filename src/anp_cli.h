#ifndef ANCHOR_ANP_CLI_H
#define ANCHOR_ANP_CLI_H

#include "yacli.h"

#define YAC_TRUE true
#define YAC_FALSE false

static inline YacUint32 codSizeAlign4(YacUint32 size)
{
    YacUint32 margin = (size & (YacUint32)0x03);
    return (margin == 0) ? size : size + (4 - margin);
}

#endif  // ANCHOR_ANP_VAR_H

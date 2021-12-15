#ifndef ANCHOR_ANP_CLI_H
#define ANCHOR_ANP_CLI_H

#include "yacli.h"

#define ANC_TRUE true
#define ANC_FALSE false

static inline AncUint32 codSizeAlign4(AncUint32 size)
{
    AncUint32 margin = (size & (AncUint32)0x03);
    return (margin == 0) ? size : size + (4 - margin);
}

#endif  // ANCHOR_ANP_VAR_H

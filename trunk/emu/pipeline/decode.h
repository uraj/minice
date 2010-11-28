#ifndef __MINIEMU_DECODE_H__
#define __MINIEMU_DECODE_H__

#include <pipeline/globdefs.h>

typedef enum
{
    LShiftLeft,
    LShiftRight,
    AShiftRight,
    RotateRight
} ShiftType;

extern void ID_phrase(StoreArch * storage, PipeState * pipe_state);

#endif

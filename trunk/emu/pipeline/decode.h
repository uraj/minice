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

/* retrun cycle count (including flush or stalling penalty) */
extern int IDStage(StoreArch * storage, PipeState * pipe_state);

#endif
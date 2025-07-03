#ifndef __SYNCRONIZATION_C__
#define __SYNCRONIZATION_C__

#include "Types.h"

BOOL kLockForSystemData();
void kUnlockForSystemData(const BOOL bInterruptFlag);

#endif

#ifndef __SYNCRONIZATION_C__
#define __SYNCRONIZATION_C__

#include "Types.h"

#pragma pack(push, 1)

typedef struct kMutexStruct {
    volatile QWORD qwTaskID;
    volatile DWORD dwLockCount;

    volatile BOOL bLockFlag;

    BYTE vbPadding[3];
} MUTEX;

#pragma pack(pop)

BOOL kLockForSystemData();
void kUnlockForSystemData(const BOOL bInterruptFlag);
void kInitializeMutex(MUTEX *pstMutex);
void kLock(MUTEX *pstMutex);
void kUnlock(MUTEX *pstMutex);

#endif

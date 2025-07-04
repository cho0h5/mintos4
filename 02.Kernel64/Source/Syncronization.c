#include "Syncronization.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Task.h"

BOOL kLockForSystemData() {
    return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(const BOOL bInterruptFlag) {
    kSetInterruptFlag(bInterruptFlag);
}

void kInitializeMutex(MUTEX *pstMutex) {
    pstMutex->qwTaskID = TASK_INVALIDID;
    pstMutex->dwLockCount = 0;
    pstMutex->bLockFlag = FALSE;
}

void kLock(MUTEX *pstMutex) {
    if (!kTestAndSet(&pstMutex->bLockFlag, 0, 1)) {
        if (pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID) {
            pstMutex->dwLockCount++;
            return;
        }

        while (!kTestAndSet(&pstMutex->bLockFlag, 0, 1)) {
            kSchedule();
        }
    }

    pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
    pstMutex->dwLockCount = 1;
}

void kUnlock(MUTEX *pstMutex) {
    if (!pstMutex->bLockFlag || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID)) {
        return;
    }

    if (pstMutex->dwLockCount > 1) {
        pstMutex->dwLockCount--;
        return;
    }

    pstMutex->qwTaskID = TASK_INVALIDID;
    pstMutex->dwLockCount = 0;
    pstMutex->bLockFlag = FALSE;
}

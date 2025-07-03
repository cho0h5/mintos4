#include "Task.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Descriptor.h"
#include "List.h"
#include "Console.h"

static TCBPOOLMANAGER gs_stTCBPoolManager;
static SCHEDULER gs_stScheduler;

// Task

void kInitializeTCBPool() {
    kMemSet(&gs_stTCBPoolManager, 0, sizeof(gs_stTCBPoolManager));

    gs_stTCBPoolManager.pstStartAddress = (TCB *)TASK_TCBPOOLADDRESS;
    kMemSet((void *)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    for (int i = 0; i < TASK_MAXCOUNT; i++) {
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    }

    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

TCB *kAllocateTCB() {
    if (gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount) {
        return NULL;
    }

    TCB *pstEmptyTCB;
    int i = 0;
    for (; i < gs_stTCBPoolManager.iMaxCount; i++) {
        if ((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
            pstEmptyTCB = &gs_stTCBPoolManager.pstStartAddress[i];
            break;
        }
    }

    pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if (gs_stTCBPoolManager.iAllocatedCount == 0) {
        gs_stTCBPoolManager.iAllocatedCount = 1;
    }

    return pstEmptyTCB;
}

void kFreeTCB(QWORD qwID) {
    const int i = GETTCBOFFSET(qwID);

    kMemSet(&gs_stTCBPoolManager.pstStartAddress[i].stContext, 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

TCB *kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    TCB *pstTask = kAllocateTCB();
    if (pstTask == NULL) {
        return NULL;
    }

    void *pvStackAddress = (void *)(TASK_STACKPOOLADDRESS +
            TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID));

    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);
    kAddTaskToReadyList(pstTask);

    return pstTask;
}

void kSetUpTask(TCB *pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
        void *pvStackAddress, QWORD qwStackSize) {
    // Init
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    // Stack
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize;

    // Segment selector
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

    // RIP
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

    // RFLAGS
    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

    // ID, stack, flags
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}

// Scheduler

void kInitializeScheduler() {
    kInitializeTCBPool();

    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitializeList(&gs_stScheduler.vstReadyList[i]);
        gs_stScheduler.viExecuteCount[i] = 0;
    }
    kInitializeList(&gs_stScheduler.stWaitList);

    gs_stScheduler.pstRunningTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;
}

void kSetRunningTask(TCB *pstTask) {
    gs_stScheduler.pstRunningTask = pstTask;
}

TCB *kGetRunningTask() {
    return gs_stScheduler.pstRunningTask;
}

TCB *kGetNextTaskToRun() {
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            const int iTaskCount = kGetListCount(&gs_stScheduler.vstReadyList[i]);

            if (gs_stScheduler.viExecuteCount[i] < iTaskCount) {
                gs_stScheduler.viExecuteCount[i]++;
                return (TCB *)kRemoveListFromHeader(&gs_stScheduler.vstReadyList[i]);
            }

            gs_stScheduler.viExecuteCount[i] = 0;
        }
    }

    return NULL;
}

BOOL kAddTaskToReadyList(TCB *pstTask) {
    const BYTE bPriority = GETPRIORITY(pstTask->qwFlags);
    if (bPriority >= TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }
    kAddListToTail(&gs_stScheduler.vstReadyList[bPriority], pstTask);
    return TRUE;
}

TCB *kRemoveTaskFromReadyList(QWORD qwTaskID) {
    if (GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT) {
        return NULL;
    }

    TCB *pstTarget = &gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)];
    if (pstTarget->stLink.qwID != qwTaskID) {
        return NULL;
    }

    const BYTE bPriority = GETPRIORITY(pstTarget->qwFlags);

    return kRemoveList(&gs_stScheduler.vstReadyList[bPriority], qwTaskID);
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
    if (bPriority > TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }

    TCB *pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
        return TRUE;
    }

    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
        pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
        if (pstTarget != NULL) {
            SETPRIORITY(pstTarget->qwFlags, bPriority);
        }
        return TRUE;
    }

    SETPRIORITY(pstTarget->qwFlags, bPriority);
    kAddTaskToReadyList(pstTarget);
    return TRUE;
}

void kSchedule() {
    if (kGetReadyTaskCount() < 1) {
        return;
    }

    const BOOL bPreviousFlag = kSetInterruptFlag(FALSE);
    TCB *pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        kSetInterruptFlag(bPreviousFlag);
        return;
    }

    TCB *pstRunningTask = gs_stScheduler.pstRunningTask;
    kAddTaskToReadyList(pstRunningTask);

    if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask +=
            TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&gs_stScheduler.stWaitList, pstRunningTask);
        kSwitchContext(NULL, &pstNextTask->stContext);
    } else {
        gs_stScheduler.pstRunningTask = pstNextTask;
        kSwitchContext(&pstRunningTask->stContext, &pstNextTask->stContext);
    }

    kSetInterruptFlag(bPreviousFlag);
}

BOOL kScheduleInInterrupt() {
    TCB *pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        return FALSE;
    }

    char *pcContextAddress = (char *)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);
    TCB *pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
    }

    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&gs_stScheduler.stWaitList, pstRunningTask);
    } else {
        kMemCpy(&pstRunningTask->stContext, pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }

    kMemCpy(pcContextAddress, &pstNextTask->stContext, sizeof(CONTEXT));

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    return TRUE;
}

void kDecreaseProcessorTime() {
    if (gs_stScheduler.iProcessorTime > 0) {
        gs_stScheduler.iProcessorTime--;
    }
}

BOOL kIsProcessorTimeExpired() {
    if (gs_stScheduler.iProcessorTime <= 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL kEndTask(QWORD qwTaskID) {
    TCB *pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kSchedule();

        while (1) ;
    }

    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
        pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
        if (pstTarget != NULL) {
            pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
            SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        }
        return FALSE;
    }

    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
    kAddListToTail(&gs_stScheduler.stWaitList, pstTarget);
    return TRUE;
}

void kExitTask() {
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount() {
    int iTotalCount = 0;

    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&gs_stScheduler.vstReadyList[i]);
    }

    return iTotalCount;
}

int kGetTaskCount() {
    return kGetReadyTaskCount() + kGetListCount(&gs_stScheduler.stWaitList) + 1;
}

TCB *kGetTCBInTCBPool(const int iOffset) {
    if ((iOffset < -1) || (iOffset > TASK_MAXCOUNT)) {
        return NULL;
    }

    return &gs_stTCBPoolManager.pstStartAddress[iOffset];
}

BOOL kIsTaskExist(const QWORD qwID) {
    const TCB *pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
    if ((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID)) {
        return FALSE;
    }
    return TRUE;
}

QWORD kGetProcessorLoad() {
    return gs_stScheduler.qwProcessorLoad;
}

// Idle task
void kIdleTask() {
    QWORD qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    QWORD qwLastMeasureTickCount = kGetTickCount();

    while (1) {
        QWORD qwCurrentMeasureTickCount = kGetTickCount();
        QWORD qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        if (qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0) {
            gs_stScheduler.qwProcessorLoad = 0;
        } else {
            gs_stScheduler.qwProcessorLoad = 100 -
                (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
                100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        kHaltProcessorByLoad();

        if (kGetListCount(&gs_stScheduler.stWaitList) >= 0) {
            while (1) {
                TCB *pstTask = kRemoveListFromHeader(&gs_stScheduler.stWaitList);
                if (pstTask == NULL) {
                    break;
                }

                kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", pstTask->stLink.qwID);
                kFreeTCB(pstTask->stLink.qwID);
            }
        }

        kSchedule();
    }
}

void kHaltProcessorByLoad() {
    if (gs_stScheduler.qwProcessorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    } else if (gs_stScheduler.qwProcessorLoad < 80) {
        kHlt();
        kHlt();
    } else if (gs_stScheduler.qwProcessorLoad < 95) {
        kHlt();
    }
}

#include "Task.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Descriptor.h"
#include "List.h"
#include "Console.h"
#include "Syncronization.h"

static TCBPOOLMANAGER gs_stTCBPoolManager;
static SCHEDULER gs_stScheduler;

// Task

static void kInitializeTCBPool() {
    kMemSet(&gs_stTCBPoolManager, 0, sizeof(gs_stTCBPoolManager));

    gs_stTCBPoolManager.pstStartAddress = (TCB *)TASK_TCBPOOLADDRESS;
    kMemSet((void *)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    for (int i = 0; i < TASK_MAXCOUNT; i++) {
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    }

    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

static TCB *kAllocateTCB() {
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

static void kFreeTCB(QWORD qwID) {
    const int i = GETTCBOFFSET(qwID);

    kMemSet(&gs_stTCBPoolManager.pstStartAddress[i].stContext, 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

TCB *kCreateTask(QWORD qwFlags, void *pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress) {
    BOOL bPreviousFlag = kLockForSystemData();
    TCB *pstTask = kAllocateTCB();
    if (pstTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return NULL;
    }

    TCB *pstProcess = kGetProcessByThread(kGetRunningTask());
    if (pstProcess == NULL) {
        kFreeTCB(pstTask->stLink.qwID);
        kUnlockForSystemData(bPreviousFlag);
        return NULL;
    }

    if (qwFlags & TASK_FLAGS_THREAD) {
        pstTask->qwParentProcessID = pstProcess->stLink.qwID;
        pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
        pstTask->qwMemorySize = pstProcess->qwMemorySize;

        kAddListToTail(&pstProcess->stChildThreadList, &pstTask->stThreadLink);
    } else {
        pstTask->qwParentProcessID = pstProcess->stLink.qwID;
        pstTask->pvMemoryAddress = pvMemoryAddress;
        pstTask->qwMemorySize = qwMemorySize;
    }
    pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

    kUnlockForSystemData(bPreviousFlag);

    void *pvStackAddress = (void *)(TASK_STACKPOOLADDRESS +
            TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID));

    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    kInitializeList(&pstTask->stChildThreadList);

    pstTask->bFPUUsed = FALSE;

    bPreviousFlag = kLockForSystemData();
    kAddTaskToReadyList(pstTask);

    kUnlockForSystemData(bPreviousFlag);
    return pstTask;
}

static void kSetUpTask(TCB *pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
        void *pvStackAddress, QWORD qwStackSize) {
    // Init
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    // Return to kExitTask when terminated
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
    *(QWORD *)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

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

    TCB *pstTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask = pstTask;
    pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
    pstTask->qwParentProcessID = pstTask->stLink.qwID;
    pstTask->pvMemoryAddress = (void *)0x100000;
    pstTask->qwMemorySize = 0x500000;
    pstTask->pvStackAddress = (void *)0x600000;
    pstTask->qwStackSize = 0x100000;

    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;

    gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;
}

void kSetRunningTask(TCB *pstTask) {
    const BOOL bPreviousFlag = kLockForSystemData();

    gs_stScheduler.pstRunningTask = pstTask;

    kUnlockForSystemData(bPreviousFlag);
}

TCB *kGetRunningTask() {
    const BOOL bPreviousFlag = kLockForSystemData();

    TCB *pstRunningTask = gs_stScheduler.pstRunningTask;

    kUnlockForSystemData(bPreviousFlag);
    return pstRunningTask;
}

static TCB *kGetNextTaskToRun() {
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

static BOOL kAddTaskToReadyList(TCB *pstTask) {
    const BYTE bPriority = GETPRIORITY(pstTask->qwFlags);
    if (bPriority >= TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }
    kAddListToTail(&gs_stScheduler.vstReadyList[bPriority], pstTask);
    return TRUE;
}

static TCB *kRemoveTaskFromReadyList(QWORD qwTaskID) {
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

    const BOOL bPreviousFlag = kLockForSystemData();

    TCB *pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
        kUnlockForSystemData(bPreviousFlag);
        return TRUE;
    }

    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
        pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
        if (pstTarget != NULL) {
            SETPRIORITY(pstTarget->qwFlags, bPriority);
        }
        kUnlockForSystemData(bPreviousFlag);
        return TRUE;
    }

    SETPRIORITY(pstTarget->qwFlags, bPriority);
    kAddTaskToReadyList(pstTarget);

    kUnlockForSystemData(bPreviousFlag);
    return TRUE;
}

void kSchedule() {
    if (kGetReadyTaskCount() < 1) {
        return;
    }

    const BOOL bPreviousFlag = kLockForSystemData();
    TCB *pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return;
    }

    TCB *pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask +=
            TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }

    if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    } else {
        kClearTS();
    }

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&gs_stScheduler.stWaitList, pstRunningTask);
        kSwitchContext(NULL, &pstNextTask->stContext);
    } else {
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&pstRunningTask->stContext, &pstNextTask->stContext);
    }

    kUnlockForSystemData(bPreviousFlag);
}

BOOL kScheduleInInterrupt() {
    const BOOL bPreviousFlag = kLockForSystemData();

    TCB *pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
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

    kUnlockForSystemData(bPreviousFlag);

    if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    } else {
        kClearTS();
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
    const BOOL bPreviousFlag = kLockForSystemData();

    TCB *pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSystemData(bPreviousFlag);

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

        kUnlockForSystemData(bPreviousFlag);
        return FALSE;   // <- ? TRUE?
    }

    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
    kAddListToTail(&gs_stScheduler.stWaitList, pstTarget);

    kUnlockForSystemData(bPreviousFlag);
    return TRUE;
}

void kExitTask() {
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount() {
    const BOOL bPreviousFlag = kLockForSystemData();

    int iTotalCount = 0;
    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&gs_stScheduler.vstReadyList[i]);
    }

    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
}

int kGetTaskCount() {
    const BOOL bPreviousFlag = kLockForSystemData();

    int iTotalCount = kGetReadyTaskCount() + kGetListCount(&gs_stScheduler.stWaitList) + 1;

    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
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

static TCB *kGetProcessByThread(TCB *pstThread) {
    if (pstThread->qwFlags & TASK_FLAGS_PROCESS) {
        return pstThread;
    }

    TCB *pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

    if (pstProcess == NULL || pstProcess->stLink.qwID != pstThread->qwParentProcessID) {
        return NULL;
    }

    return pstProcess;
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
                const BOOL bPreviousFlag = kLockForSystemData();

                TCB *pstTask = kRemoveListFromHeader(&gs_stScheduler.stWaitList);
                if (pstTask == NULL) {
                    kUnlockForSystemData(bPreviousFlag);
                    break;
                }

                if (pstTask->qwFlags & TASK_FLAGS_PROCESS) {
                    const int iCount = kGetListCount(&pstTask->stChildThreadList);
                    for (int i = 0; i < iCount; i++) {
                        void *pstThreadLink = kRemoveListFromHeader(&pstTask->stChildThreadList);
                        if (pstThreadLink == NULL) {
                            break;
                        }

                        TCB *pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);
                        kAddListToTail(&pstTask->stChildThreadList, &pstChildThread->stThreadLink);
                        kEndTask(pstChildThread->stLink.qwID);
                    }

                    if (kGetListCount(&pstTask->stChildThreadList) > 0) {
                        kAddListToTail(&gs_stScheduler.stWaitList, pstTask);

                        kUnlockForSystemData(bPreviousFlag);
                        continue;
                    } else {
                        // TODO
                    }
                } else if (pstTask->qwFlags & TASK_FLAGS_THREAD) {
                    TCB *pstProcess = kGetProcessByThread(pstTask);
                    if (pstProcess != NULL) {
                        kRemoveList(&pstProcess->stChildThreadList, pstTask->stLink.qwID);
                    }
                }

                const QWORD qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(pstTask->stLink.qwID);

                kUnlockForSystemData(bPreviousFlag);
                kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
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

// FPU
QWORD kGetLastFPUUsedTaskID() {
    return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(QWORD qwTaskID) {
    gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}

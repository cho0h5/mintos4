#include "Types.h"
#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "HardDisk.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = { 0, };

    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintStringXY(0, 0, "========================================");
    kPrintStringXY(0, 1, "            Interrupt Occur             ");
    kPrintStringXY(0, 2, "              Vector:                   ");
    kPrintStringXY(0, 3, "========================================");
    kPrintStringXY(22, 2, vcBuffer);

    while (1) ;
}

void kCommonInterruptHandler(int iVectorNumber) {
    static int g_iCommonInterruptCount = 0;
    char vcBuffer[] = "[INT:  , ]";

    // print
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount += 1;
    g_iCommonInterruptCount %= 10;
    kPrintStringXY(70, 0, vcBuffer);

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber) {
    static int g_iKeyboardInterruptCount = 0;
    char vcBuffer[] = "[INT:  , ]";
    BYTE bTemp;

    // Print
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount += 1;
    g_iKeyboardInterruptCount %= 10;
    kPrintStringXY(70, 1, vcBuffer);

    if (kIsOutputBufferFull()) {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kTimerHandler(int iVectorNumber) {
    static int g_iTimerInterruptCount = 0;
    char vcBuffer[] = "[INT:  , ]";

    // print
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iTimerInterruptCount;
    g_iTimerInterruptCount += 1;
    g_iTimerInterruptCount %= 10;
    kPrintStringXY(70, 0, vcBuffer);

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

    g_qwTickCount++;

    kDecreaseProcessorTime();
    if (kIsProcessorTimeExpired()) {
        kScheduleInInterrupt();
    }
}

void kDeviceNotAvailableHandler(int iVectorNumber) {
    static int g_iFPUInterruptCount = 0;
    char vcBuffer[] = "[EXC:  , ]";

    // print
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iFPUInterruptCount;
    g_iFPUInterruptCount += 1;
    g_iFPUInterruptCount %= 10;
    kPrintStringXY(70, 2, vcBuffer);

    // Save
    kClearTS();

    const QWORD qwLastFPUUsedTaskID = kGetLastFPUUsedTaskID();
    TCB *pstCurrentTask = kGetRunningTask();

    if (qwLastFPUUsedTaskID == pstCurrentTask->stLink.qwID) {
        return;
    } else if (qwLastFPUUsedTaskID != TASK_INVALIDID) {
        TCB *pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUUsedTaskID));
        if (pstFPUTask != NULL && pstFPUTask->stLink.qwID == qwLastFPUUsedTaskID) {
            kSaveFPUContext(pstFPUTask->vqwFPUContext);
        }
    }

    // Restore
    if (!pstCurrentTask->bFPUUsed) {
        kInitializeFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    } else {
        kLoadFPUContext(pstCurrentTask->vqwFPUContext);
    }

    kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber) {
    static int g_iHDDInterruptCount = 0;
    char vcBuffer[] = "[HDD:  , ]";

    // print
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iHDDInterruptCount;
    g_iHDDInterruptCount += 1;
    g_iHDDInterruptCount %= 10;
    kPrintStringXY(70, 0, vcBuffer);

    if (iVectorNumber - PIC_IRQSTARTVECTOR == 14) {
        kSetHDDInterruptFlag(TRUE, TRUE);
    } else {
        kSetHDDInterruptFlag(FALSE, TRUE);
    }

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

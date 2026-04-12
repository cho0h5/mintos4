#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "Syncronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MPConfigurationTable.h"
#include "MultiProcessor.h"
#include "LocalAPIC.h"
#include "PIC.h"

SHELLCOMMANENTRY gs_vstCommandTable[] = {
    {"help", "Show help", kHelp},
    {"clear", "Clear screen", kCls},
    {"totalram", "Show total RAM size", kShowTotalRAMSize},
    {"strtod", "String to decimal/hex convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown and reboot OS", kShutdown},
    {"settimer", "Set PIT Controler Counter0. Usage: settimer 10(ms) 1(periodic)", kSetTimer},
    {"wait", "Wait ms Using PIT. Usage: wait 100(ms)", kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
    {"date", "Show Date and Time", kShowDateAndTime},
    {"createtask", "Create Task. Usage: createtask 1(type) 10(count)", kCreateTestTask},
    {"changepriority", "Change Task Priority. Usage: changepriority 1(ID) 2(Priority)",
        kChangeTaskPriority},
    {"tasklist", "Show Task List", kShowTaskList},
    {"killtask", "End Task. Usage: killtask 1(ID) or 0xffffffff(All Task)", kKillTask},
    {"cpuload", "Show Processor Load", kCPULoad},
    {"testmutex", "Test Mutex Function", kTestMutex},
    {"testthread", "Test Thread and Process Function", kTestThread},
    {"showmatrix", "Show Matrix Screen", kShowMatrix},
    {"testpie", "Test PIE Calculation", kTestPIE},
    {"dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation},
    {"testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
    {"testranalloc", "Test Random Allocation & Free", kTestRandomAllocation},
    {"hddinfo", "Show HDD Information", kShowHDDInformation},
    {"readsector", "Read HDD Sector, Usage: readsector 0(LBA) 10(count)", kReadSector},
    {"writesector", "Write HDD Sector, Usage: writesector 0(LBA) 10(count)", kWriteSector},
    {"mounthdd", "Mount HDD", kMountHDD},
    {"formathdd", "Format HDD", kFormatHDD},
    {"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
    {"touch", "Create File. Usage: touch a.txt", kCreateFileInRootDirectory},
    {"rm", "Delete File. Usage: rm a.txt", kDeleteFileInRootDirectory},
    {"ls", "Show Directory", kShowRootDirectory},
    {"writefile", "Write Data To File. Usage: writefile a.txt", kWriteDataToFile},
    {"readfile", "Read Data From File. Usage: readfile a.txt", kReadDataFromFile},
    {"testfileio", "Test File I/O Function", kTestFileIO},
    {"testperformance", "Test File Read/WritePerformance", kTestPerformance},
    {"flush", "Flush File System Cache", kFlushCache},
    {"download", "Download Data From Serial. Usage: download a.txt", kDownloadFile},
    {"showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable},
    {"startap", "Start Application Processor", kStartApplicationProcessor},
    {"startsymmetricio", "Start Symmetric I/O Mode", kStartSymmetricIOMode},
    {"showirqintinmap", "Show IRQ->INITIN Mapping Table", kShowIRQINTINMappingTable},
};

void kStartConsoleShell() {
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;

    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while (1) {
        const BYTE bKey = kGetCh();
        if (bKey == KEY_BACKSPACE) {
            if (iCommandBufferIndex > 0) {
                int iCursorX, iCursorY;
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStringXY(iCursorX - 1, iCursorY, " ");
                kSetCursor(iCursorX - 1, iCursorY);
                iCommandBufferIndex--;
            }
        } else if (bKey == KEY_ENTER) {
            kPrintf("\n");

            if (iCommandBufferIndex > 0) {
                vcCommandBuffer[iCommandBufferIndex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }

            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        } else if (bKey == KEY_LSHIFT || bKey == KEY_RSHIFT || bKey == KEY_CAPSLOCK
                || bKey == KEY_NUMLOCK || bKey == KEY_SCROLLLOCK) {
        } else {
            if (iCommandBufferIndex < 299) {
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

void kExecuteCommand(const char *pcParameterBuffer) {
    int iSpaceIndex = 0;
    int iCommandBufferLength = kStrLen(pcParameterBuffer);
    for (; iSpaceIndex < iCommandBufferLength; iSpaceIndex++) {
        if (pcParameterBuffer[iSpaceIndex] == ' ') {
            break;
        }
    }

    const int iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANENTRY);
    int i = 0;
    for (; i < iCount; i++) {
        int iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if (iCommandLength == iSpaceIndex &&
                kMemCmp(gs_vstCommandTable[i].pcCommand, pcParameterBuffer, iSpaceIndex) == 0) {
            gs_vstCommandTable[i].pfFunction(pcParameterBuffer + iSpaceIndex + 1);
            break;
        }
    }

    if (i >= iCount) {
        kPrintf("'%s' is not found\n", pcParameterBuffer);
    }
}

void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter) {
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter) {
    if (pstList->iLength <= pstList->iCurrentPosition) {
        return 0;
    }

    int i = pstList->iCurrentPosition;
    for (; i < pstList->iLength; i++) {
        if (pstList->pcBuffer[i] == ' ') {
            break;
        }
    }

    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    const int iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}

static void kHelp(const char *pcParameterBuffer) {
    kPrintf("========================================\n");
    kPrintf("           MINT64 Shell Help            \n");
    kPrintf("========================================\n");

    const int iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANENTRY);

    int iMaxCommandLength = 0;
    for (int i = 0; i < iCount; i++) {
        int iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if (iLength > iMaxCommandLength) {
            iMaxCommandLength = iLength;
        }
    }

    for (int i = 0; i < iCount; i++) {
        kPrintf("%s", gs_vstCommandTable[i].pcCommand);
        int iCursorX, iCursorY;
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);

        if (i != 0 && i % 20 == 0) {
            kPrintf("Press any key to continue... ('q' is exit): ");
            if (kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
            kPrintf("\n");
        }
    }
}

static void kCls(const char *pcParameterBuffer) {
    kClearScreen();
    kSetCursor(0, 1);
}

static void kShowTotalRAMSize(const char *pcParameterBuffer) {
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

static void kStringToDecimalHexTest(const char *pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;

    kInitializeParameter(&stList, pcParameterBuffer);

    int iCount = 0;
    while (1) {
        const int iLength = kGetNextParameter(&stList, vcParameter);
        if (iLength == 0) {
            break;
        }

        kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);
        if (kMemCmp(vcParameter, "0x", 2) == 0) {
            const long lValue = kAToI(vcParameter + 2, 16);
            kPrintf("HEX Value = %q\n", lValue);
        } else {
            const long lValue = kAToI(vcParameter, 10);
            kPrintf("Decimal Value = %d\n", lValue);
        }

        iCount++;
    }
}

static void kShutdown(const char *pcParameterBuffer) {
    kPrintf("System Shutdown Start...\n");

    kPrintf("Cache Flush...");
    if (kFlushFileSystemCache()) {
        kPrintf("Pass\n");
    } else {
        kPrintf("Fail\n");
    }

    kPrintf("Press Any Key To Reboot PC...");
    kGetCh();
    kReboot();
}

static void kSetTimer(const char *pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;

    kInitializeParameter(&stList, pcParameterBuffer);

    if (kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("Usage: settimer 10(ms) 1(periodic)\n");
        return;
    }
    const long lValue = kAToI(vcParameter, 10);

    if (kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("Usage: settimer 10(ms) 1(periodic)\n");
        return;
    }
    const BOOL bPeriodic = kAToI(vcParameter, 10);

    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

static void kWaitUsingPIT(const char *pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;

    kInitializeParameter(&stList, pcParameterBuffer);

    if (kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("Usage: wait 100(ms)\n");
        return;
    }
    const long lMillisecond = kAToI(vcParameter, 10);
    kPrintf("%d ms Sleep Start...\n", lMillisecond);

    kDisableInterrupt();
    for (int i = 0; i < lMillisecond / 30; i++) {
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
    kEnableInterrupt();
    kPrintf("%d ms Sleep Complete\n", lMillisecond);

    kInitializePIT(MSTOCOUNT(1), TRUE);
}

static void kReadTimeStampCounter(const char * pcParameterBuffer) {
    const QWORD qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

static void kMeasureProcessorSpeed(const char *pcParameterBuffer) {
    kPrintf("Now Measuring.");

    QWORD qwTotalTSC = 0;
    kDisableInterrupt();
    for (int i = 0; i < 200; i++) {
        QWORD qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;
        kPrintf(".");
    }

    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();

    kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

static void kShowDateAndTime(const char *pcParameterBuffer) {
    BYTE bHour, bMinute, bSecond;
    kReadRTCTime(&bHour, &bMinute, &bSecond);

    WORD wYear;
    BYTE bMonth, bDayOfMonth, bDayOfWeek;
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("Date: %d/%d/%d %s, ",
            wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("Time: %d:%d:%d\n", bHour, bMinute, bSecond);
}

static void kTestTask1() {
    CHARACTER *pstScreen = (CHARACTER *)CONSOLE_VIDEOMEMORYADDRESS;

    const TCB *pstRunningTask = kGetRunningTask();
    const int iMargin = (pstRunningTask->stLink.qwID & 0xffffffff) % 10;

    BYTE bData;
    int i = 0, iX = 0, iY = 0;
    for (int j = 0; j < 20000; j++) {
        switch (i) {
            case 0:
                iX++;
                if (iX >= (CONSOLE_WIDTH - iMargin)) i = 1;
                break;

            case 1:
                iY++;
                if (iY >= (CONSOLE_HEIGHT - iMargin)) i = 2;
                break;

            case 2:
                iX--;
                if (iX < iMargin) i = 3;
                break;

            case 3:
                iY--;
                if (iY < iMargin) i = 0;
                break;
        }

        pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
        pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
        bData++;

        // kSchedule();
    }

    kExitTask();
}

static void kTestTask2() {
    CHARACTER *pstScreen = (CHARACTER *)CONSOLE_VIDEOMEMORYADDRESS;
    char vcData[4] = {'-', '\\', '|', '/'};

    const TCB *pstRunningTask = kGetRunningTask();
    int iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    int i = 0;
    while (1) {
        pstScreen[iOffset].bCharactor = vcData[i % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        // kSchedule();
    }
}

static void kCreateTestTask(const char *pcParameterBuffer) {
    char vcType[30];
    char vcCount[30];
    PARAMETERLIST stList;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);

    const int iCount = kAToI(vcCount, 10);
    switch (kAToI(vcType, 10)) {
        case 1:
            for (int i = 0; i < iCount; i++) {
                if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL) {
                    break;
                }
            }

            kPrintf("Task1 %d Created\n", iCount);
            break;

        case 2:
        default:
            for (int i = 0; i < iCount; i++) {
                if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2) == NULL) {
                    break;
                }
            }

            kPrintf("Task2 %d Created\n", iCount);
            break;
    }

}

static void kChangeTaskPriority(const char *pcParameterBuffer) {
    char vcID[30];
    char vcPriority[30];
    PARAMETERLIST stList;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);
    kGetNextParameter(&stList, vcPriority);

    QWORD qwID;
    if (kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID + 2, 16);
    } else {
        qwID = kAToI(vcID, 10);
    }

    const BYTE bPriority = kAToI(vcPriority, 10);

    kPrintf("Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority);
    if (kChangePriority(qwID, bPriority)) {
        kPrintf("Success\n");
    } else {
        kPrintf("Fail\n");
    }
}

static void kShowTaskList(const char *pcParameterBuffer) {
    int iCount = 1;

    kPrintf("======== Task Total Count [%d] =========\n", kGetTaskCount());
    for (int i = 0; i < TASK_MAXCOUNT; i++) {
        const TCB *pstTCB = kGetTCBInTCBPool(i);
        if ((pstTCB->stLink.qwID >> 32) == 0) {
            continue;
        }

        if ((iCount != 0) && ((iCount % 10) == 0)) {
            kPrintf("Press any key to continue...('q') is exit): ");
            if (kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
            kPrintf("\n");
        }

        kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n",
                iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags),
                pstTCB->qwFlags, kGetListCount(&pstTCB->stChildThreadList));
        kPrintf("    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
                pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress,
                pstTCB->qwMemorySize);
    }
}

static void kKillTask(const char *pcParameterBuffer) {
    char vcID[30];
    PARAMETERLIST stList;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);

    QWORD qwID;
    if (kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID + 2, 16);
    } else {
        qwID = kAToI(vcID, 10);
    }

    if (qwID != 0xffffffff) {
        TCB *pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
        qwID = pstTCB->stLink.qwID;

        if (((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)) {
            kPrintf("Kill Task ID [0x%q] ", qwID);
            if (kEndTask(qwID)) {
                kPrintf("Success\n");
            } else {
                kPrintf("Fail\n");
            }
            return;
        } else {
            kPrintf("Task does not exist or task is system task\n");
            return;
        }
    }

    for (int i = 2; i < TASK_MAXCOUNT; i++) {
        TCB *pstTCB = kGetTCBInTCBPool(i);
        qwID = pstTCB->stLink.qwID;

        if (((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)) {
            kPrintf("Kill Task ID [0x%q] ", qwID);
            if (kEndTask(qwID)) {
                kPrintf("Success\n");
            } else {
                kPrintf("Fail\n");
            }
        }
    }
}

static void kCPULoad(const char *pcParameterBuffer) {
    kPrintf("Processor Load: %d%%\n", kGetProcessorLoad());
}

static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask() {
    QWORD qwTickCount = kGetTickCount();
    while (kGetTickCount() - qwTickCount < 50) {
        kSchedule();
    }

    for (int i = 0; i < 5; i++) {
        kLock(&gs_stMutex);
        kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);

        gs_qwAdder += 1;
        kUnlock(&gs_stMutex);

        for (int j = 0; j < 30000; j++) ;
    }

    qwTickCount = kGetTickCount();
    while (kGetTickCount() - qwTickCount < 1000) {
        kSchedule();
    }

    kExitTask();
}

static void kTestMutex(const char *pcParameterBuffer) {
    gs_qwAdder = 1;

    kInitializeMutex(&gs_stMutex);

    int i = 0;
    for (; i < 3; i++) {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumberTask);
    }

    kPrintf("Wait Util %d Task End...\n", i);
    kGetCh();
}

static void kCreateThreadTask() {
    for (int i = 0; i < 3; i++) {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2);
    }

    while (1) {
        kSleep(1);
    }
}

static void kTestThread(const char *pcParameterBuffer) {
    const TCB *pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS,
            (void *)0xeeeeeeee, 0x1000, (QWORD)kCreateThreadTask);

    if (pstProcess != NULL) {
        kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);
    } else {
        kPrintf("Process Create Fail\n");
    }
}

static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom() {
    gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
    return gs_qwRandomValue;
}

static void kDropCharacterThread() {
    char vcText[2] = { 0, };

    const int iX = kRandom() % CONSOLE_WIDTH;

    while (1) {
        kSleep(kRandom() % 20);

        if (kRandom() % 20 < 15) {
            vcText[0] = ' ';
            for (int i = 0; i < CONSOLE_HEIGHT - 1; i++) {
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
        } else {
            for (int i = 0; i < CONSOLE_HEIGHT - 1; i++) {
                vcText[0] = i + kRandom();
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
        }
    }
}

static void kMatrixProcess() {
    int i = 0;
    for (; i < 300; i++) {
        if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kDropCharacterThread) == NULL) {
            break;
        }

        kSleep(kRandom() % 5 + 5);
    }

    kPrintf("%d Thread is created\n", i);

    kGetCh();
}

static void kShowMatrix(const char *pcParameterBuffer) {
    TCB *pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void *)0xe00000, 0xe00000, (QWORD)kMatrixProcess);
    if (pstProcess != NULL) {
        kPrintf("Matrix Process [0x%Q] Create Success\n");

        while ((pstProcess->stLink.qwID >> 32) != 0) {
            kSleep(100);
        }
    } else {
        kPrintf("Matrix Process Create Fail");
    }
}

static void kFPUTestTask() {
    const char vcData[4] = {'-', '\\', '|', '/'};
    CHARACTER *pstScreen = (CHARACTER *)CONSOLE_VIDEOMEMORYADDRESS;

    const TCB *pstRunningTask = kGetRunningTask();
    int iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    QWORD qwCount = 0;
    while (1) {
        double dValue1 = 1;
        double dValue2 = 1;

        for (int i = 0; i < 10; i++) {
            QWORD qwRandomValue = kRandom();
            dValue1 *= (double)qwRandomValue;
            dValue2 *= (double)qwRandomValue;

            kSleep(1);

            qwRandomValue = kRandom();
            dValue1 /= (double)qwRandomValue;
            dValue2 /= (double)qwRandomValue;
        }

        if (dValue1 != dValue2) {
            kPrintf("Value Is Not Same. [%f] != [%f]\n", dValue1, dValue2);
            break;
        }

        qwCount++;
        pstScreen[iOffset].bCharactor = vcData[qwCount % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
    }
}

static void kTestPIE(const char *pcParameterBuffer) {
    kPrintf("PIE Calculation Test\n");
    kPrintf("Result: 355 / 113 = ");
    const double dResult = (double)355 / 113;
    kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult * 10) % 10),
            ((QWORD)(dResult * 100) % 10));

    for (int i = 0; i < 100; i++) {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask);
    }
}

static void kShowDynamicMemoryInformation(const char *pcParameterBuffer) {
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;
    kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);

    kPrintf("====== Dynamic Memory Information ======\n");
    kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
    kPrintf("Total Size:    [0x%Q] bytes, [%d] KB, [%d] MB\n", qwTotalSize,
            qwTotalSize / 1024, qwTotalSize / 1024 / 1024);
    kPrintf("Meta Size:     [0x%Q] bytes, [%d] KB\n", qwMetaSize, qwMetaSize / 1024);
    kPrintf("Used Size:     [0x%Q] bytes, [%d] KB\n", qwUsedSize, qwUsedSize / 1024);
}

static void kTestSequentialAllocation(const char *pcParameterBuffer) {
    kPrintf("========= Dynamic Memory Test ==========\n");
    DYNAMICMEMORY *pstMemory = kGetDynamicMemoryManager();

    // For each level
    for (int i = 0; i < pstMemory->iMaxLevelCount; i++) {
        kPrintf("Block List [%d] Test Start\n", i);
        kPrintf("Allocation and Compare: ");

        // Test allocation
        for (int j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++) {
            QWORD *pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
            if (pqwBuffer == NULL) {
                kPrintf("\nAllocation Fail\n");
                return;
            }

            for (int k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++) {
                pqwBuffer[k] = k;
            }

            for (int k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++) {
                if (pqwBuffer[k] != k) {
                    kPrintf("\nCompare Fail\n");
                    return;
                }
            }

            kPrintf(".");
        }

        // Test deallocation
        kPrintf("\nFree: ");
        for (int j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++) {
            if (!kFreeMemory((void *)pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE << i) * j)) {
                kPrintf("\nFree Fail\n");
                return;
            }
            kPrintf(".");
        }
        kPrintf("\n");
    }
    kPrintf("Test Complete!\n");
}

static void kRandomAllocationTask() {
    const TCB *pstTask = kGetRunningTask();
    const int iY = pstTask->stLink.qwID % 15 + 9;

    for (int j = 0; j < 10; j++) {
        QWORD qwMemorySize;
        BYTE *pbAllocationBuffer;
        do {
            qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
            pbAllocationBuffer = kAllocateMemory(qwMemorySize);

            if (pbAllocationBuffer == NULL) {
                kSleep(1);
            }
        } while (pbAllocationBuffer == NULL);

        char vcBuffer[200];
        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        kSleep(200);

        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...       ", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        for (int i = 0; i < qwMemorySize / 2; i++) {
            pbAllocationBuffer[i] = kRandom() % 0xff;
            pbAllocationBuffer[i + (qwMemorySize / 2)] = pbAllocationBuffer[i];
        }
        kSleep(200);

        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...       ", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        for (int i = 0; i < qwMemorySize / 2; i++) {
            if (pbAllocationBuffer[i] != pbAllocationBuffer[i + (qwMemorySize / 2)]) {
                kPrintf("Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID);
                kExitTask();
            }
        }

        kFreeMemory(pbAllocationBuffer);
        kSleep(200);
    }

    kExitTask();
}

static void kTestRandomAllocation(const char *pbCurrentBitmapPosition) {
    for (int i = 0; i < 1000; i++) {
        kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)kRandomAllocationTask);
    }
}

static void kShowHDDInformation(const char *pcParameterBuffer) {
    char vcBuffer[100];

    HDDINFORMATION stHDD;
    if (!kGetHDDInformation(&stHDD)) {
        kPrintf("HDD Information Read Fail\n");
        return;
    }

    kPrintf("==== Primary Master HDD Information ====\n");

    // Print model number
    kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
    vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
    kPrintf("Model Number:\t%s\n", vcBuffer);

    // Print serial number
    kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
    vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
    kPrintf("Serial Number:\t%s\n", vcBuffer);

    kPrintf("Head Count:\t\t%d\n", stHDD.wNumberOfHead);
    kPrintf("Cylinder Count:\t%d\n", stHDD.wNumberOfCylinder);
    kPrintf("Sector Count:\t%d\n", stHDD.wNumberOfSectorPerCylinder);
    kPrintf("Total Sector:\t%d, %d MB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors / 2 / 1024);
}

static void kReadSector(const char *pcParameterBuffer) {
    char vcLBA[50];
    char vcSectorCount[50];
    PARAMETERLIST stList;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    if (kGetNextParameter(&stList, vcLBA) == 0 || kGetNextParameter(&stList, vcSectorCount) == 0) {
        kPrintf("Usage: readsector 0(LBA) 10(count)\n");
        return;
    }
    const DWORD dwLBA = kAToI(vcLBA, 10);
    const int iSectorCount = kAToI(vcSectorCount, 10);

    // Read
    char *pcBuffer = kAllocateMemory(iSectorCount * 512);
    if (kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount) {
        kPrintf("Read Fail\n");
        kFreeMemory(pcBuffer);
        return;
    }

    kPrintf("LBA [%d], [%d] Sector Read Success", dwLBA, iSectorCount);
    BOOL bExit = FALSE;
    for (int j = 0; j < iSectorCount; j++) {
        for (int i = 0; i < 512; i++) {
            if (!(j == 0 && i == 0) && (i % 256 == 0)) {
                kPrintf("\nPress any key to continue... ('q' is exit): ");
                if (kGetCh() == 'q') {
                    bExit = TRUE;
                    break;
                }
            }

            if (i % 16 == 0) {
                kPrintf("\n[LBA: %d, Offset: %d\t| ", dwLBA + j, i);
            }

            const BYTE bData = pcBuffer[j * 512 + i] & 0xff;
            if (bData < 16) {
                kPrintf("0");
            }
            kPrintf("%X ", bData);
        }

        if (bExit) {
            break;
        }
    }
    kPrintf("\n");

    kFreeMemory(pcBuffer);
}

static void kWriteSector(const char *pcParameterBuffer) {
    static DWORD s_dwWriteCount = 0;
    char vcLBA[50];
    char vcSectorCount[50];
    PARAMETERLIST stList;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    if (kGetNextParameter(&stList, vcLBA) == 0 || kGetNextParameter(&stList, vcSectorCount) == 0) {
        kPrintf("Usage: writesector 0(LBA) 10(count)\n");
        return;
    }
    const DWORD dwLBA = kAToI(vcLBA, 10);
    const int iSectorCount = kAToI(vcSectorCount, 10);

    // Prepare data
    s_dwWriteCount++;
    char *pcBuffer = kAllocateMemory(iSectorCount * 512);
    for (int j = 0; j < iSectorCount; j++) {
        for (int i = 0; i < 512; i += 8) {
            *(DWORD *)&(pcBuffer[j * 512 + i]) = dwLBA + j;
            *(DWORD *)&(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
        }
    }

    // Write
    if (kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount) {
        kPrintf("Write Fail\n");
        kFreeMemory(pcBuffer);
        return;
    }
    kPrintf("LBA [%d], [%d] Sector Read Success", dwLBA, iSectorCount);

    BOOL bExit = FALSE;
    for (int j = 0; j < iSectorCount; j++) {
        for (int i = 0; i < 512; i++) {
            if (!(j == 0 && i == 0) && (i % 256 == 0)) {
                kPrintf("\nPress any key to continue... ('q' is exit): ");
                if (kGetCh() == 'q') {
                    bExit = TRUE;
                    break;
                }
            }

            if (i % 16 == 0) {
                kPrintf("\n[LBA: %d, Offset: %d]\t| ", dwLBA + j, i);
            }

            const BYTE bData = pcBuffer[j * 512 + i] & 0xff;
            if (bData < 16) {
                kPrintf("0");
            }
            kPrintf("%X ", bData);
        }

        if (bExit) {
            break;
        }
    }
}

static void kMountHDD(const char *pcParameterBuffer) {
    if (!kMount()) {
        kPrintf("HDD Mount Failed\n");
        return;
    }
    kPrintf("HDD Mount Success\n");
}

static void kFormatHDD(const char *pcParameterBuffer) {
    if (!kFormat()) {
        kPrintf("HDD Format Fail\n");
        return;
    }
    kPrintf("HDD Format Success\n");
}

static void kShowFileSystemInformation(const char *pcParameterBuffer) {
    FILESYSTEMMANAGER stManager;
    kGetFileSystemInformation(&stManager);

    kPrintf("======= File System Information ========\n");
    kPrintf("Mounted:\t\t\t\t\t\t\t%d\n", stManager.bMounted);
    kPrintf("Reserved Sector Count:\t\t\t\t%d Sector\n", stManager.dwReservedSectorCount);
    kPrintf("Cluster Link Table Start Address:\t%d Sector\n", stManager.dwClusterLinkAreaStartAddress);
    kPrintf("Cluster Link Table Size:\t\t\t%d Sector\n", stManager.dwClusterLinkAreaSize);
    kPrintf("Data Area Start Address:\t\t\t%d Sector\n", stManager.dwDataAreaStartAddress);
    kPrintf("Total Cluster Count:\t\t\t\t%d Cluster\n", stManager.dwTotalClusterCount);

}

static void kCreateFileInRootDirectory(const char *pcParameterBuffer) {
    char vcFileName[50];
    PARAMETERLIST stList;
    DIRECTORYENTRY stEntry;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    const int iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if (iLength == 0 || iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) {
        kPrintf("Too Short or Too Long File Name\n");
        return;
    }

    FILE *pstFile = fopen(vcFileName, "w");
    if (pstFile == NULL) {
        kPrintf("File Create Failed\n");
        return;
    }
    fclose(pstFile);
    kPrintf("File Create Success\n");
}

static void kDeleteFileInRootDirectory(const char *pcParameterBuffer) {
    char vcFileName[50];
    PARAMETERLIST stList;
    DIRECTORYENTRY stEntry;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    const int iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if (iLength == 0 || iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) {
        kPrintf("Too Short or Too Long File Name\n");
        return;
    }

    if  (remove(vcFileName) != 0) {
        kPrintf("File Not Found or File Opened\n");
        return;
    }
    kPrintf("File Delete Success\n");
}

static void kShowRootDirectory(const char *pcParameterBuffer) {
    FILESYSTEMMANAGER stManager;
    kGetFileSystemInformation(&stManager);

    DIR *pstDirectory = opendir("/");
    if (pstDirectory == NULL) {
        kPrintf("Root Directory Open Failed\n");
        return;
    }

    int iTotalCount = 0;
    DWORD dwTotalByte = 0;
    DWORD dwUsedClusterCount = 0;
    while (1) {
        struct dirent *pstEntry = readdir(pstDirectory);
        if (pstEntry == NULL) {
            break;
        }
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        if (pstEntry->dwFileSize == 0) {
            dwUsedClusterCount++;
        } else {
            dwUsedClusterCount += (pstEntry->dwFileSize + FILESYSTEM_CLUSTERSIZE - 1) / FILESYSTEM_CLUSTERSIZE;
        }
    }

    rewinddir(pstDirectory);
    int iCount = 0;
    while (1) {
        struct dirent *pstEntry = readdir(pstDirectory);
        if (pstEntry == NULL) {
            break;
        }

        char vcBuffer[400];
        char vcTempValue[400];
        kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
        vcBuffer[sizeof(vcBuffer) - 1] = '\0';

        kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

        kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize);
        kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

        kSPrintf(vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex);
        kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
        kPrintf("\t%s\n", vcBuffer);

        if (iCount != 0 && iCount % 20 == 0) {
            kPrintf("Press any key to continue... ('q' is exit): ");
            if (kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
        }
        iCount++;
    }

    kPrintf("\t\tTotal File Count: %d\n", iTotalCount);
    kPrintf("\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte / 1024, dwUsedClusterCount);
    kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n", (stManager.dwTotalClusterCount - dwUsedClusterCount)
            * FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - dwUsedClusterCount);

    closedir(pstDirectory);
}

static void kWriteDataToFile(const char *pcParameterBuffer) {
    char vcFileName[50];
    PARAMETERLIST stList;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    const int iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if (iLength == 0 || iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) {
        kPrintf("Too Short or Too Long File Name\n");
        return;
    }

    // Create a File
    FILE *fp = fopen(vcFileName, "w");
    if (fp == NULL) {
        kPrintf("%s File Open Failed\n", vcFileName);
        return;
    }

    int iEnterCount = 0;
    while (1) {
        const BYTE bKey = kGetCh();
        if (bKey == KEY_ENTER) {
            iEnterCount++;
            if (iEnterCount >= 3) {
                break;
            }
        } else {
            iEnterCount = 0;
        }

        kPrintf("%c", bKey);
        if (fwrite(&bKey, 1, 1, fp) != 1) {
            kPrintf("File Write Failed\n");
            break;
        }
    }

    kPrintf("File Create Success\n");
    fclose(fp);
}

static void kReadDataFromFile(const char *pcParameterBuffer) {
    char vcFileName[50];
    PARAMETERLIST stList;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    const int iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if (iLength == 0 || iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) {
        kPrintf("Too Short or Too Long File Name\n");
        return;
    }

    FILE *fp = fopen(vcFileName, "r");
    if (fp == NULL) {
        kPrintf("%s File Open Failed\n", vcFileName);
        return;
    }

    int iEnterCount = 0;
    while (1) {
        BYTE bKey;
        if (fread(&bKey, 1, 1, fp) != 1) {
            break;
        }

        kPrintf("%c", bKey);

        if (bKey == KEY_ENTER) {
            iEnterCount++;
            if (iEnterCount != 0 && iEnterCount % 20 == 0) {
                kPrintf("Press any key to continue... ('q' is exit): ");
                if (kGetCh() == 'q') {
                    kPrintf("\n");
                    break;
                }
                kPrintf("\n");
                iEnterCount = 0;
            }
        }
    }
    fclose(fp);
}

static void kTestFileIO(const char *pcParameterBuffer) {
    const DWORD dwMaxFileSize = 4 * 1024 * 1024;
    BYTE *pbBuffer = kAllocateMemory(dwMaxFileSize);
    if (pbBuffer == NULL) {
        kPrintf("Memory Allocation Failed\n");
        return;
    }
    remove("testfileio.bin");

    // Test1
    kPrintf("1. File Open Fail Test...");
    FILE *pstFile = fopen("testfileio.bin", "r");
    if (pstFile == NULL) {
        kPrintf("[Pass]\n");
    } else {
        kPrintf("[Fail] 1\n");
        fclose(pstFile);
        return;
    }

    // Test2
    kPrintf("2. File Create Test...");
    pstFile = fopen("testfileio.bin", "w");
    if (pstFile != NULL) {
        kPrintf("[Pass]\n");
        kPrintf("\tFile Handle [0x%Q]\n", pstFile);
    } else {
        kPrintf("[Fail] 2\n");
        fclose(pstFile);
        return;
    }

    // Test3
    kPrintf("3. Sequential Write Test (Cluster Size)...");
    int i = 0;
    for (; i < 100; i++) {
        kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);
        if (fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("[Fail] 3\n");
            kPrintf("\t%d Cluster Error\n", i);
            fclose(pstFile);
            return;
        }
    }
    if (i >= 100) {
        kPrintf("[Pass]\n");
    }

    // Test4
    kPrintf("4. Sequential Read And Verify Test (Cluster Size)...");
    fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

    for (int i = 0; i < 100; i++) {
        if (fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("[Fail] 4 - 1\n");
            fclose(pstFile);
            return;
        }

        for (int j = 0; j < FILESYSTEM_CLUSTERSIZE; j++) {
            if (pbBuffer[j] != (BYTE)i) {
                kPrintf("[Fail] 4 - 2\n");
                kPrintf("\t%d Cluster Error. [%X] != [%X]\n", i, pbBuffer[j], (BYTE)i);
                fclose(pstFile);
                return;
            }
        }
    }
    if (i >= 100) {
        kPrintf("[Pass]\n");
    }

    // Test5
    kPrintf("5. Random Write Test...\n");
    kMemSet(pbBuffer, 0, dwMaxFileSize);
    fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
    fread(pbBuffer, 1, dwMaxFileSize, pstFile);

    for (int i = 0; i < 100; i++) {
        char vbTempBuffer[1024];
        const DWORD dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
        const DWORD dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);

        kPrintf("\t[%d] Offset [%d]...", i, dwRandomOffset, dwByteCount);

        fseek(pstFile, dwRandomOffset, SEEK_SET);
        kMemSet(vbTempBuffer, i, dwByteCount);

        if (fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount) {
            kPrintf("[Fail] 5\n");
            fclose(pstFile);
            return;
        } else {
            kPrintf("[Pass]\n");
        }

        kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
    }
    fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
    fwrite(&i, 1, 1, pstFile);
    pbBuffer[dwMaxFileSize - 1] = (BYTE)i;

    // Test6
    kPrintf("6. Random Read And Verify Test...\n");
    for (int i = 0; i < 100; i++) {
        char vbTempBuffer[1024];
        const DWORD dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
        const DWORD dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);

        kPrintf("\t[%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);

        fseek(pstFile, dwRandomOffset, SEEK_SET);

        if (fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount) {
            kPrintf("[Fail] 6 - 1\n");
            kPrintf("\tRead Fail\n", dwRandomOffset);
            fclose(pstFile);
            return;
        }

        if (kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0) {
            kPrintf("[Fail] 6 - 2\n");
            kPrintf("\tCompare Fail\n", dwRandomOffset);
            fclose(pstFile);
            return;
        }

        kPrintf("[Pass]\n");
    }

    // Test7
    kPrintf("7. Sequential Write, Read and Verify Test(1024 Byte)...\n");

    fseek(pstFile, -dwMaxFileSize, SEEK_CUR);
    for (int i = 0; i < 2 * 1024; i++) {
        kPrintf("\t[%d] Offset [%d] Write...", i, i * 1024, 1024);

        if (fwrite(pbBuffer + i * 1024, 1, 1024, pstFile) != 1024) {
            kPrintf("[Fail] 7 - 1\n");
            fclose(pstFile);
            return;
        } else {
            kPrintf("[Pass]\n");
        }
    }

    fseek(pstFile, -dwMaxFileSize, SEEK_SET);
    for (int i = 0; i < dwMaxFileSize / 1024; i++) {
        kPrintf("\t[%d] Offset [%d] Read and Verify...", i, i * 1024, 1024);

        char vbTempBuffer[1024];
        if (fread(vbTempBuffer, 1, 1024, pstFile) != 1024) {
            kPrintf("[Fail] 7 - 2\n");
            fclose(pstFile);
            return;
        }

        if (kMemCmp(pbBuffer + i * 1024, vbTempBuffer, 1024) != 0) {
            kPrintf("[Fail] 7 - 3\n");
            fclose(pstFile);
            return;
        } else {
            kPrintf("[Pass]\n");
        }
    }

    // Test8
    kPrintf("8. File Delete Fail Test...");
    if (remove("testfileio.bin") != 0) {
        kPrintf("[Pass]\n");
    } else {
        kPrintf("[Fail] 8\n");
        fclose(pstFile);
        return;
    }

    // Test9
    kPrintf("9. File Close Test...");
    if (fclose(pstFile) == 0) {
        kPrintf("[Pass]\n");
    } else {
        kPrintf("[Fail] 9\n");
        fclose(pstFile);
        return;
    }

    // Test10
    kPrintf("10. File Delete Test...");
    if (remove("testfileio.bin") == 0) {
        kPrintf("[Pass]\n");
    } else {
        kPrintf("[Fail] 10\n");
        fclose(pstFile);
        return;
    }

    kFreeMemory(pbBuffer);
}

static void kTestPerformance(const char *pcParameterBuffer) {
    const DWORD dwClusterTestFileSize = 1024 * 1024;
    BYTE *pbBuffer = kAllocateMemory(dwClusterTestFileSize);
    if (pbBuffer == NULL) {
        kPrintf("Memory Allocate Failed\n");
        return;
    }
    kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

    kPrintf("====== File I/O Performance Test =======\n");
    kPrintf("1. Sequential Read/Write Test(Cluster Size)\n");

    remove("performance.txt");
    FILE *pstFile = fopen("performance.txt", "w");
    if (pstFile == NULL) {
        kPrintf("File Open Failed\n");
        kFreeMemory(pbBuffer);
        return;
    }

    QWORD qwLastTickCount = kGetTickCount();
    for (int i = 0; i < dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE; i++) {
        if (fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("Write Failed\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }
    kPrintf("\tSequential Write(Cluster Size): %d ms\n", kGetTickCount() - qwLastTickCount);

    fseek(pstFile, 0, SEEK_SET);
    qwLastTickCount = kGetTickCount();
    for (int i = 0; i < dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE; i++) {
        if (fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("Read Fail\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }
    kPrintf("\tSequential Read(Cluster Size): %d ms\n", kGetTickCount() - qwLastTickCount);

    kPrintf("2. Sequential Read/Write Test(1 Byte)\n");

    remove("performance.txt");
    pstFile = fopen("performance.txt", "w");
    if (pstFile == NULL) {
        kPrintf("FILE Open Failed\n");
        kFreeMemory(pbBuffer);
        return;
    }

    qwLastTickCount = kGetTickCount();
    const DWORD dwOneByteTestFileSize = 16 * 1024;
    for (int i = 0; i < dwOneByteTestFileSize; i++) {
        if (fwrite(pbBuffer, 1, 1, pstFile) != 1) {
            kPrintf("Write Fail\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }
    kPrintf("\tWequential Write(1 Byte): %d ms\n", kGetTickCount() - qwLastTickCount);

    fseek(pstFile, 0, SEEK_SET);

    qwLastTickCount = kGetTickCount();
    for (int i = 0; i < dwOneByteTestFileSize; i++) {
        if (fread(pbBuffer, 1, 1, pstFile) != 1) {
            kPrintf("Read Fail\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }
    kPrintf("\tSequential Read(1 Byte): %d ms \n", kGetTickCount() - qwLastTickCount);

    fclose(pstFile);
    kFreeMemory(pbBuffer);
}

static void kFlushCache(const char *pcParameterBuffer) {
    const QWORD qwTickCount = kGetTickCount();
    kPrintf("Cache Flush... ");
    if (kFlushFileSystemCache()) {
        kPrintf("Pass\n");
    } else {
        kPrintf("Fail\n");
    }
    kPrintf("Total Time = %d ms\n", kGetTickCount() - qwTickCount);
}

static void kDownloadFile(const char *pcParameterBuffer) {
    char vcFileName[50];
    PARAMETERLIST stList;

    // Parse args
    kInitializeParameter(&stList, pcParameterBuffer);
    const int iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if (iLength == 0 || iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) {
        kPrintf("Too Short or Too Long File Name\n");
        return;
    }

    // Clear FIFO
    kClearSerialFIFO();

    // Receive data length
    kPrintf("Waiting For Data Length...");
    DWORD dwReceiveSize = 0;
    DWORD dwDataLength;
    QWORD qwLastReceivedTickCount = kGetTickCount();
    while (dwReceiveSize < 4) {
        const DWORD dwTempSize = kReceiveSerialData((BYTE *)&dwDataLength + dwReceiveSize, 4 - dwReceiveSize);
        dwReceiveSize += dwTempSize;

        if (dwTempSize == 0) {
            kSleep(0);

            if (kGetTickCount() - qwLastReceivedTickCount > 30000) {
                kPrintf("Time Out Occur\n");
                return;
            }
        } else {
            qwLastReceivedTickCount = kGetTickCount();
        }
    }
    kPrintf("[%d] Byte\n", dwDataLength);
    kSendSerialData("A", 1);

    // Receive and save data
    FILE *fp = fopen(vcFileName, "w");
    if (fp == NULL) {
        kPrintf("%s File Open Fail\n", vcFileName);
        return;
    }

    kPrintf("Data Receive Start: ");
    dwReceiveSize = 0;
    qwLastReceivedTickCount = kGetTickCount();
    while (dwReceiveSize < dwDataLength) {
        BYTE vbDataBuffer[SERIAL_FIFOMAXSIZE];
        const DWORD dwTempSize = kReceiveSerialData(vbDataBuffer, SERIAL_FIFOMAXSIZE);
        dwReceiveSize += dwTempSize;

        if (dwTempSize != 0) {
            if ((dwReceiveSize % SERIAL_FIFOMAXSIZE) == 0 || dwReceiveSize == dwDataLength) {
                kSendSerialData("A", 1);
                kPrintf("#");
            }

            if (fwrite(vbDataBuffer, 1, dwTempSize, fp) != dwTempSize) {
                kPrintf("File Write Error Occur\n");
                break;
            }

            qwLastReceivedTickCount = kGetTickCount();
        } else {
            kSleep(0);

            if (kGetTickCount() - qwLastReceivedTickCount > 10000) {
                kPrintf("Time Out Occur\n");
                break;
            }
        }
    }

    if (dwReceiveSize != dwDataLength) {
        kPrintf("\nError Occur, Total Size: [%d], Received Size: [%d]\n", dwReceiveSize, dwDataLength);
    } else {
        kPrintf("\nReceive Complete. Total Size: [%d] Byte\n", dwReceiveSize);
    }

    fclose(fp);
    kFlushFileSystemCache();
}

static void kShowMPConfigurationTable(const char *pcParameterBuffer) {
    kPrintMPConfigurationTable();
}

static void kStartApplicationProcessor(const char *pcParameterBuffer) {
    if (!kStartUpApplicationProcessor()) {
        kPrintf("Application Processor Start Fail\n");
        return;
    }
    kPrintf("Application Processor Start Success\n");

    kPrintf("Bootstrap Processor[APIC ID: %d] Start Application Processor\n", kGetAPICID());
}

static void kStartSymmetricIOMode(const char *pcParameterBuffer) {
    if (!kAnalysisMPConfigurationTable()) {
        kPrintf("Analysis MP Configuration Table Fail\n");
        return;
    }

    MPCONFIGURATIONMANAGER *pstMPManager = kGetMPConfigurationManager();
    if (pstMPManager->bUsePICMode) {
        kOutPortByte(0x22, 0x70);   // IMCR Register
        kOutPortByte(0x23, 0x01);   // Disable PIC Mode
    }

    kPrintf("Mask All PIC Controller Interrupt\n");
    kMaskPICInterrupt(0xffff);  // Disable all interrupts of PIC

    kPrintf("Enable Global Local APIC\n");
    kEnableGlobalLocalAPIC();
    
    kPrintf("Enable Software Local APIC\n");
    kEnableSoftwareLocalAPIC();

    kPrintf("Disable CPU Interrupt Flag\n");
    const BOOL bInterruptFlag = kSetInterruptFlag(FALSE);

    kSetTaskPriority(0);

    kInitializeLocalVectorTable();

    kPrintf("Initialize IO Redirection Table\n");
    kInitializeIORedirectionTable();

    kPrintf("Restore CPU Interrupt Flag\n");
    kSetInterruptFlag(bInterruptFlag);

    kPrintf("Change Symmetric I/O Mode Complete\n");
}

static void kShowIRQINTINMappingTable(const char *pcParameterBuffer) {
    kPrintIRQToINTINMap();
}

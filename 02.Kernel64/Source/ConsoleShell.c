#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"

SHELLCOMMANENTRY gs_vstCommandTable[] = {
    {"help", "Show help", kHelp},
    {"clear", "Clear screen", kCls},
    {"totalram", "Show total RAM size", kShowTotalRAMSize},
    {"strtod", "String to decimal/hex convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown and reboot OS", kShutdown},
    {"settimer", "Set PIT Controller Counter0. Usage: settimer 10(ms) 1(periodic)", kSetTimer},
    {"wait", "Wait ms Using PIT. Usage: wait 100(ms)", kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
    {"date", "Show Date and Time", kShowDateAndTime},
    {"createtask", "Create Task. Usage: createtask 1(type) 10(count)", kCreateTestTask},
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

void kHelp(const char *pcParameterBuffer) {
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
    }
}

void kCls(const char *pcParameterBuffer) {
    kClearScreen();
    kSetCursor(0, 1);
}

void kShowTotalRAMSize(const char *pcParameterBuffer) {
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

void kStringToDecimalHexTest(const char *pcParameterBuffer) {
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

void kShutdown(const char *pcParameterBuffer) {
    kPrintf("System Shutdown Start...\n");
    kPrintf("Press Any Key To Reboot PC...");
    kGetCh();
    kReboot();
}

void kSetTimer(const char *pcParameterBuffer) {
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

void kWaitUsingPIT(const char *pcParameterBuffer) {
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

void kReadTimeStampCounter(const char * pcParameterBuffer) {
    const QWORD qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

void kMeasureProcessorSpeed(const char *pcParameterBuffer) {
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

void kShowDateAndTime(const char *pcParameterBuffer) {
    BYTE bHour, bMinute, bSecond;
    kReadRTCTime(&bHour, &bMinute, &bSecond);

    WORD wYear;
    BYTE bMonth, bDayOfMonth, bDayOfWeek;
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("Date: %d/%d/%d %s, ",
            wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("Time: %d:%d:%d\n", bHour, bMinute, bSecond);
}

void kTestTask1() {
    CHARACTER *pstScreen = (CHARACTER *)CONSOLE_VIDEOMEMORYADDRESS;

    const TCB *pstRunningTask = kGetRunningTask();
    const int iMargin = (pstRunningTask->stLink.qwID & 0xffffffff) % 10;

    BYTE bData;
    int i = 0, iX = 0, iY = 0;
    while (1) {
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

        kSchedule();
    }
}

void kTestTask2() {
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

        kSchedule();
    }
}

void kCreateTestTask(const char *pcParameterBuffer) {
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
                if (kCreateTask(TASK_FLAGS_LOW, (QWORD)kTestTask1) == NULL) {
                    break;
                }
            }

            kPrintf("Task1 %d Created\n", iCount);
            break;

        case 2:
        default:
            for (int i = 0; i < iCount; i++) {
                if (kCreateTask(TASK_FLAGS_LOW, (QWORD)kTestTask2) == NULL) {
                    break;
                }
            }

            kPrintf("Task2 %d Created\n", iCount);
            break;
    }

}

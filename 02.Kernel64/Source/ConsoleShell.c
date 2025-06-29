#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"

SHELLCOMMANENTRY gs_vstCommandTable[] = {
    {"help", "Show help", kHelp},
    {"clear", "Clear screen", kCls},
    {"totalram", "Show total RAM size", kShowTotalRAMSize},
    {"strtod", "String to decimal/hex convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown and reboot OS", kShutdown},
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
    kPrintf(0, 0, "========================================");
    kPrintf(0, 1, "           MINT64 Shell Help            ");
    kPrintf(0, 2, "========================================");

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

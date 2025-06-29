#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"
#include "AssemblyUtility.h"
#include "Utility.h"

CONSOLEMANAGER gs_stConsoleManager = { 0, };

void kSetCursor(const int iX, const int iY) {
    const int iLinearValue = iY * CONSOLE_WIDTH + iX;

    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
    kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
    kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xff);

    gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

void kGetCursor(int *piX, int *piY) {
    *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
    *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf(const char *pcFormatString, ...) {
    va_list ap;
    char vcBuffer[1024];

    va_start(ap, pcFormatString);
    kVSPrintf(vcBuffer, pcFormatString, ap);
    va_end(ap);

    int iNextPrintOffset = kConsolePrintString(vcBuffer);

    kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

int kConsolePrintString(const char *pcBuffer) {
    CHARACTER *pstScreen = (CHARACTER *)CONSOLE_VIDEOMEMORYADDRESS;

    int iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;
    int iLength = kStrLen(pcBuffer);
    for (int i = 0; i < iLength; i++) {
        if (pcBuffer[i] == '\n') {
            iPrintOffset += CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH);
        } else if (pcBuffer[i] == '\t') {
            iPrintOffset += 4 - (iPrintOffset % 4);
        } else {
            pstScreen[iPrintOffset].bCharactor = pcBuffer[i];
            pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            iPrintOffset++;
        }

        if (iPrintOffset >= CONSOLE_HEIGHT * CONSOLE_WIDTH) {
            kMemCpy((void *)CONSOLE_VIDEOMEMORYADDRESS,
                    (void *)(CONSOLE_VIDEOMEMORYADDRESS + CONSOLE_WIDTH * sizeof(CHARACTER)),
                    (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

            for (int j = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
                    j < CONSOLE_HEIGHT * CONSOLE_WIDTH; j++) {
                pstScreen[j].bCharactor = ' ';
                pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            }

            iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
        }
    }

    return iPrintOffset;
}

BYTE kGetCh() {
    KEYDATA stData;

    while (1) {
        while (!kGetKeyFromKeyQueue(&stData)) ;

        if (stData.bFlags & KEY_FLAGS_DOWN) {
            return stData.bASCIICode;
        }
    }
}

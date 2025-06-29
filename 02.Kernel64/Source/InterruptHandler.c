#include "Types.h"
#include "InterruptHandler.h"
#include "PIC.h"
#include "Utility.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = { 0, };

    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintString(0, 0, "========================================");
    kPrintString(0, 1, "            Interrupt Occur             ");
    kPrintString(0, 2, "              Vector:                   ");
    kPrintString(0, 3, "========================================");
    kPrintString(22, 2, vcBuffer);

    while (1) ;
}

void kCommonInterruptHandler(int iVectorNumber) {
    static int g_iCommonInterruptCount = 0;
    char vcBuffer[] = "[INT:  , ]";

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iCommonInterruptCount;

    g_iCommonInterruptCount += 1;
    g_iCommonInterruptCount %= 10;

    kPrintString(70, 0, vcBuffer);

    kSendEOIToPIC(iVectorNumber - 32);
}

void kKeyboardHandler(int iVectorNumber) {
    static int g_iKeyboardInterruptCount = 0;
    char vcBuffer[] = "[INT:  , ]";

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;

    g_iKeyboardInterruptCount += 1;
    g_iKeyboardInterruptCount %= 10;

    kPrintString(70, 1, vcBuffer);

    kSendEOIToPIC(iVectorNumber - 32);
}

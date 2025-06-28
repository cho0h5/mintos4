#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"

void kPrintString(const int iX, const int iY, const char *pcString);

void Main() {
    kPrintString(1, 10, "Pass");
    kPrintString(0, 11, "[Pass] IA-32e C Language Kernel Start");

    kPrintString(0, 12, "[    ] Keyboard Activate");

    if (kActivateKeyboard()) {
        kPrintString(1, 12, "Pass");
    } else {
        kPrintString(1, 12, "Fail");
        while (1) ;
    }

    kInitializeGDTTableAndTSS();
    kLoadGDTR(0x142000);
    kLoadTR(0x18);

    kInitializeIDTTables();
    kLoadIDTR(0x1420a0);

    int i = 0;
    while (1) {
        if (kIsOutputBufferFull()) {
            const BYTE bTemp = kGetKeyboardScanCode();

            char vcTemp[2] = { 0, };
            BYTE bFlags;
            if (kConvertScanCodeToASCIICode(bTemp, &vcTemp[0], &bFlags)) {
                if (bFlags & KEY_FLAGS_DOWN) {
                    kPrintString(i++, 13, vcTemp);
                }
            }
        }
    }
}

void kPrintString(const int iX, const int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xb8000;

    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != '\0'; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}


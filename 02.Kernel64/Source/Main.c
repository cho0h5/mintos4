#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"

void kPrintString(const int iX, const int iY, const char *pcString);

void Main() {
    kPrintString(1, 10, "Pass");
    kPrintString(0, 11, "[Pass] IA-32e C Language Kernel Start");

    kPrintString(0, 12, "[    ] GDT Initialize And Switch For IA-32e Mode");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kPrintString(1, 12, "Pass");

    kPrintString(0, 13, "[    ] TSS Segment Load");
    kLoadTR(GDT_TSSSEGMENT);
    kPrintString(1, 13, "Pass");

    kPrintString(0, 14, "[    ] IDT Initialize");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kPrintString(1, 14, "Pass");

    kPrintString(0, 15, "[    ] Keyboard Activate");
    if (kActivateKeyboard()) {
        kPrintString(1, 15, "Pass");
    } else {
        kPrintString(1, 15, "Fail");
        while (1) ;
    }

    int i = 0;
    while (1) {
        if (kIsOutputBufferFull()) {
            const BYTE bTemp = kGetKeyboardScanCode();

            char vcTemp[2] = { 0, };
            BYTE bFlags;
            if (kConvertScanCodeToASCIICode(bTemp, &vcTemp[0], &bFlags)) {
                if (bFlags & KEY_FLAGS_DOWN) {
                    kPrintString(i++, 16, vcTemp);
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


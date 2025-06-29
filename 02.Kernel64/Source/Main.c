#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "PIC.h"

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

    kPrintString(0, 15, "[    ] Keyboard Activate And Queue Initialize");
    if (kInitializeKeyboard()) {
        kPrintString(1, 15, "Pass");
    } else {
        kPrintString(1, 15, "Fail");
        while (1) ;
    }

    kPrintString(0, 16, "[    ] PIC Controller and Interrupt Initialize");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kPrintString(1, 16, "Pass");

    int i = 0;
    while (1) {
        KEYDATA stData;
        if (kGetKeyFromKeyQueue(&stData)) {
            char vcTemp[2] = { 0, };
            if (stData.bFlags & KEY_FLAGS_DOWN) {
                vcTemp[0] = stData.bASCIICode;
                kPrintString(i++, 17, vcTemp);

                if (vcTemp[0] == '0') {
                    int tmp = 0;
                    tmp = 1 / tmp;
                }
            }
        }
    }
}

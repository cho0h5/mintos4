#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Utility.h"

void Main() {
    kInitializeConsole(0, 10);

    int iCursorX, iCursorY;
    kGetCursor(&iCursorX, &iCursorY);

    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("[Pass] IA-32e C Language Kernel Start\n");
    iCursorY++;

    kPrintf("[    ] GDT Initialize And Switch For IA-32e Mode\n");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("[    ] TSS Segment Load\n");
    kLoadTR(GDT_TSSSEGMENT);
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("[    ] IDT Initialize\n");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("[    ] Total RAM Size Check\n");
    kCheckTotalRAMSize();
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("[    ] Keyboard Activate And Queue Initialize\n");
    if (kInitializeKeyboard()) {
        kSetCursor(1, iCursorY++);
        kPrintf("Pass\n");
    } else {
        kSetCursor(1, iCursorY++);
        kPrintf("Fail\n");
        while (1) ;
    }

    kPrintf("[    ] PIC Controller and Interrupt Initialize\n");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kStartConsoleShell();
}

#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Utility.h"
#include "PIT.h"
#include "Task.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MultiProcessor.h"

void MainForApplicationProcessor();

void Main() {
    if (*((BYTE *)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
        MainForApplicationProcessor();
    }
    *((BYTE *)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

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

    kPrintf("[    ] TCB PoolAnd Scheduler Initialize\n");
    kInitializeScheduler();
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("[    ] Dynamic Memory Initialize\n");
    kInitializeDynamicMemory();
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kInitializePIT(MSTOCOUNT(1), 1);

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

    kPrintf("[    ] File System Initialize\n");
    if (kInitializeFileSystem()) {
        kSetCursor(1, iCursorY++);
        kPrintf("Pass\n");
    } else {
        kSetCursor(1, iCursorY++);
        kPrintf("Fail\n");
    }

    kPrintf("[    ] Serial Port Initialize\n");
    kInitializeSerialPort();
    kSetCursor(1, iCursorY++);
    kPrintf("Pass\n");

    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE,
            0, 0, (QWORD)kIdleTask);
    kStartConsoleShell();
}

void MainForApplicationProcessor() {
    // GDT
    kLoadGDTR(GDTR_STARTADDRESS);

    // TSS
    kLoadTR(GDT_TSSSEGMENT + kGetAPICID() * sizeof(GDTENTRY16));

    // IDT
    kLoadIDTR(IDTR_STARTADDRESS);

    // Print
    QWORD qwTickCount = kGetTickCount();
    while (1) {
        if (kGetTickCount() - qwTickCount > 1000) {
            qwTickCount = kGetTickCount();
            kPrintf("Application Processor[APIC ID: %d] is Activated\n", kGetAPICID());
        }
    }
}

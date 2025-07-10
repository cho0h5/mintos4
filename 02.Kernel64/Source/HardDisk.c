#include "HardDisk.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Console.h"

static HDDMANAGER gs_stHDDManager;

BOOL kInitializeHDD() {
    kInitializeMutex(&gs_stHDDManager.stMutex);

    gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
    gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

    kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
    kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

    if (!kReadHDDInformation(TRUE, TRUE, &gs_stHDDManager.stHDDInformation)) {
        gs_stHDDManager.bHDDDetected = FALSE;
        gs_stHDDManager.bCanWrite = FALSE;
        return FALSE;
    }

    gs_stHDDManager.bHDDDetected = TRUE;
    if (kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0) {
        gs_stHDDManager.bCanWrite = TRUE;
    } else {
        gs_stHDDManager.bCanWrite = FALSE;
    }
    return TRUE;
}

static BYTE kReadHDDStatus(const BOOL bPrimary) {
    if (bPrimary) {
        return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
    }
    return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

static BOOL kWaitForHDDNoBusy(const BOOL bPrimary) {
    const QWORD qwStartTickCount = kGetTickCount();

    while (kGetTickCount() - qwStartTickCount <= HDD_WAITTIME) {
        const BYTE bStatus = kReadHDDStatus(bPrimary);
        if ((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) {
            return TRUE;
        }

        kSleep(1);
    }
    return FALSE;
}

static BOOL kWaitForHDDReady(const BOOL bPrimary) {
    QWORD qwStartTickCount = kGetTickCount();

    while (kGetTickCount() - qwStartTickCount <= HDD_WAITTIME) {
        const BYTE bStatus = kReadHDDStatus(bPrimary);

        if ((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY) {
            return TRUE;
        }

        kSleep(1);
    }
    return FALSE;
}

void kSetHDDInterruptFlag(const BOOL bPrimary, const BOOL bFlag) {
    if (bPrimary) {
        gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
    } else {
        gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
    }
}

static BOOL kWaitForHDDInterrupt(const BOOL bPrimary) {
    const QWORD qwTickCount = kGetTickCount();

    while (kGetTickCount() - qwTickCount <= HDD_WAITTIME) {
        if (bPrimary && gs_stHDDManager.bPrimaryInterruptOccur) {
            return TRUE;
        } else if (!bPrimary && gs_stHDDManager.bSecondaryInterruptOccur) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL kReadHDDInformation(const BOOL bPrimary, const BOOL bMaster, HDDINFORMATION *pstHDDInformation) {
    WORD wPortBase;
    if (bPrimary) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    } else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    kLock(&gs_stHDDManager.stMutex);

    if (!kWaitForHDDNoBusy(bPrimary)) {
        kUnlock(&gs_stHDDManager.stMutex);
        return FALSE;
    }

    BYTE bDriveFlag;
    if (bMaster) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    } else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

    // Wait for ready
    if (!kWaitForHDDReady(bPrimary)) {
        kUnlock(&gs_stHDDManager.stMutex);
        return FALSE;
    }
    kSetHDDInterruptFlag(bPrimary, FALSE);  // Enable interrupt

    // Send a command
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

    // Wait for interrupt
    const BOOL bWaitResult = kWaitForHDDInterrupt(bPrimary);
    const BYTE bStatus = kReadHDDStatus(bPrimary);
    if (!bWaitResult || (bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
        kUnlock(&gs_stHDDManager.stMutex);
        return FALSE;
    }

    // Receive
    for (int i = 0; i < 512 / 2; i++) {
        ((WORD *)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
    }

    kSwapByteInWord(pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber) / 2);
    kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber) / 2);

    kUnlock(&gs_stHDDManager.stMutex);
    return TRUE;
}

static void kSwapByteInWord(WORD *pwData, const int iWordCount) {
    for (int i = 0; i < iWordCount; i++) {
        const WORD wTemp = pwData[i];
        pwData[i] = (wTemp >> 8) | (wTemp << 8);
    }
}

// Read

int kReadHDDSector(const BOOL bPrimary, const BOOL bMaster, const DWORD dwLBA,
        const int iSectorCount, char *pcBuffer) {
    if (!gs_stHDDManager.bHDDDetected || iSectorCount <= 0 || 256 < iSectorCount ||
            dwLBA + iSectorCount >= gs_stHDDManager.stHDDInformation.dwTotalSectors) {
        return 0;
    }

    WORD wPortBase;
    if (bPrimary) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    } else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    kLock(&gs_stHDDManager.stMutex);

    if (!kWaitForHDDNoBusy(bPrimary)) {
        kUnlock(&gs_stHDDManager.stMutex);
        return 0;
    }

    // Set registers
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    BYTE bDriveFlag;
    if (bMaster) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    } else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0f));

    // Send a command
    if (!kWaitForHDDReady(bPrimary)) {
        kUnlock(&gs_stHDDManager.stMutex);
        return FALSE;
    }
    kSetHDDInterruptFlag(bPrimary, FALSE);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

    // Wait for interrupt and receive
    long lReadCount = 0;
    int i = 0;
    for (; i < iSectorCount; i++) {
        const BYTE bStatus = kReadHDDStatus(bPrimary);
        if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kPrintf("Error Occur\n");
            kUnlock(&gs_stHDDManager.stMutex);
            return i;
        }

        if ((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            const BOOL bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);
            if (bWaitResult == FALSE) {
                kPrintf("Interrupt Not Occur\n");
                kUnlock(&gs_stHDDManager.stMutex);
                return FALSE;
            }
        }

        for (int j = 0; j < 512 / 2; j++) {
            ((WORD *)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
        }
    }

    kUnlock(&gs_stHDDManager.stMutex);
    return i;
}

// Write

int kWriteHDDSector(const BOOL bPrimary, const BOOL bMaster, const DWORD dwLBA,
        const int iSectorCount, const char *pcBuffer) {
    if (!gs_stHDDManager.bCanWrite || iSectorCount <= 0 || 256 < iSectorCount ||
            dwLBA + iSectorCount >= gs_stHDDManager.stHDDInformation.dwTotalSectors) {
        return 0;
    }

    WORD wPortBase;
    if (bPrimary) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    } else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    kLock(&gs_stHDDManager.stMutex);

    if (!kWaitForHDDNoBusy(bPrimary)) {
        return FALSE;
    }

    // Set registers
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    BYTE bDriveFlag;
    if (bMaster) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    } else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0f));

    // Send a command
    if (!kWaitForHDDReady(bPrimary)) {
        kUnlock(&gs_stHDDManager.stMutex);
        return FALSE;
    }
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

    // Wait for send
    while (1) {
        const BYTE bStatus = kReadHDDStatus(bPrimary);
        if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kUnlock(&gs_stHDDManager.stMutex);
            return 0;
        }

        if ((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST) {
            break;
        }

        kSleep(1);
    }

    // Send
    long lReadCount = 0;
    int i = 0;
    for (; i < iSectorCount; i++) {
        kSetHDDInterruptFlag(bPrimary, FALSE);

        for (int j = 0; j < 512 / 2; j++) {
            kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((WORD *)pcBuffer)[lReadCount++]);
        }

        const BYTE bStatus = kReadHDDStatus(bPrimary);
        if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kUnlock(&gs_stHDDManager.stMutex);
            return i;
        }

        if ((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            const BOOL bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);
            if (!bWaitResult) {
                kUnlock(&gs_stHDDManager.stMutex);
                return FALSE;
            }
        }
    }

    kUnlock(&gs_stHDDManager.stMutex);
    return i;
}

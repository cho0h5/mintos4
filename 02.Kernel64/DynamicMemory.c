#include "DynamicMemory.h"
#include "Utility.h"
#include "Syncronization.h"

static DYNAMICMEMORY gs_stDynamicMemory;

// Initialize

void kInitializeDynamicMemory() {
    const QWORD qwDynamicMemorySize = kCalculateDynamicMemorySize();
    const int iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

    gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

    int i = 0;
    for (; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++) ;
    gs_stDynamicMemory.iMaxLevelCount = i;

    // Block List Index Address
    gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE *)DYNAMICMEMORY_START_ADDRESS;
    for (int i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++) {
        gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xff;
    }

    // Block List BITMAP Address
    gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP *)(DYNAMICMEMORY_START_ADDRESS +
            (sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock));

    // Actual Block List Bitmap Address
    BYTE *pbCurrentBitmapPosition = (BYTE *)gs_stDynamicMemory.pstBitmapOfLevel +
        (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);

    for (int j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++) {
        gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
        gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;
        const int iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

        for (int i = 0; i < iBlockCountOfLevel / 8; i++) {
            *pbCurrentBitmapPosition = 0x00;
            pbCurrentBitmapPosition++;
        }

        if (iBlockCountOfLevel % 8 != 0) {
            *pbCurrentBitmapPosition = 0x00;
            const int i = iBlockCountOfLevel % 8;

            if (i % 2 == 1) {
                *pbCurrentBitmapPosition |= DYNAMICMEMORY_EXIST << (i - 1);
                gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
            }
            pbCurrentBitmapPosition++;
        }
    }

    gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
    gs_stDynamicMemory.qwEndAddress = DYNAMICMEMORY_START_ADDRESS + kCalculateDynamicMemorySize();
    gs_stDynamicMemory.qwUsedSize = 0;
}

static QWORD kCalculateDynamicMemorySize() {
    QWORD qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);
    if (qwRAMSize > (QWORD)3 * 1024 * 1024 * 1024) {
        qwRAMSize = (QWORD)3 * 1024 * 1024 * 1024;
    }

    return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}

static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize) {
    const long lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;
    const DWORD dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);

    DWORD dwSizeOfBitmap = 0;
    for (int i = 0; (lBlockCountOfSmallestBlock >> i) > 0; i++) {
        dwSizeOfBitmap += sizeof(BITMAP);
        dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
    }

    return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap +
            DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}

// Allocation

void *kAllocateMemory(const QWORD qwSize) {
    const QWORD qwAlignedSize = kGetBuddyBlockSize(qwSize);
    if (qwAlignedSize == 0) {
        return NULL;
    }

    if (gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize + qwAlignedSize >
            gs_stDynamicMemory.qwEndAddress) {
        return NULL;
    }

    const long lOffset = kAllocationBuddyBlock(qwAlignedSize);
    if (lOffset == -1) {
        return NULL;
    }

    const int iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

    const QWORD qwRelativeAddress = qwAlignedSize * lOffset;
    const int iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;
    gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = (BYTE)iIndexOfBlockList;
    gs_stDynamicMemory.qwUsedSize += qwAlignedSize;

    return (void *)(qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}

static QWORD kGetBuddyBlockSize(const QWORD qwSize) {
    for (long i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        if (qwSize <= (DYNAMICMEMORY_MIN_SIZE << i)) {
            return DYNAMICMEMORY_MIN_SIZE << i;
        }
    }
    return 0;
}

static int kAllocationBuddyBlock(const QWORD qwAlignedSize) {
    const int iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
    if (iBlockListIndex == -1) {
        return -1;
    }

    const BOOL bPreviousInterruptFlag = kLockForSystemData();

    int iFreeOffset;
    int i = iBlockListIndex;
    for (; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        iFreeOffset = kFindFreeBlockInBitmap(i);
        if (iFreeOffset != -1) {
            break;
        }
    }

    if (iFreeOffset == -1) {
        kUnlockForSystemData(bPreviousInterruptFlag);
        return -1;
    }

    kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

    if (i <= iBlockListIndex) {
        kUnlockForSystemData(bPreviousInterruptFlag);
        return iFreeOffset;
    }

    for (i = i - 1; i >= iBlockListIndex; i--) {
        kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);
        kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);
        iFreeOffset = iFreeOffset * 2;
    }

    kUnlockForSystemData(bPreviousInterruptFlag);
    return iFreeOffset;
}

static int kGetBlockListIndexOfMatchSize(const QWORD qwAlignedSize) {
    for (int i = 0; i< gs_stDynamicMemory.iMaxLevelCount; i++) {
        if (qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i)) {
            return i;
        }
    }
    return -1;
}

static int kFindFreeBlockInBitmap(const int iBlockListIndex) {
    if (gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0) {
        return -1;
    }

    const int iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
    const BYTE *pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
    for (int i = 0; i < iMaxCount; ) {
        if ((iMaxCount - i) / 64 > 0) {
            const QWORD *pqwBitmap = (QWORD *)&(pbBitmap[i / 8]);
            if (*pqwBitmap == 0) {
                i += 64;
                continue;
            }
        }

        if ((pbBitmap[i / 8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0) {
            return i;
        }
        i++;
    }
    return -1;
}

static void kSetFlagInBitmap(const int iBlockListIndex, const int iOffset, const BYTE bFlag) {
    BYTE *pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
    if (bFlag == DYNAMICMEMORY_EXIST) {
        if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) == 0) {
            gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
        }
        pbBitmap[iOffset / 8] |= 0x01 << (iOffset % 8);
    } else {
        if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0) {
            gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
        }
        pbBitmap[iOffset / 8] &= ~(0x01 << (iOffset % 8));
    }
}

// Deallocation

BOOL kFreeMemory(const void *pvAddress) {
    if (pvAddress == NULL) {
        return FALSE;
    }

    const QWORD qwRelativeAddress = (QWORD)pvAddress - gs_stDynamicMemory.qwStartAddress;
    const int iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

    if (gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xff) {
        return FALSE;
    }

    const int iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset];
    gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = 0xff;
    const QWORD qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

    const int iBitmapOffset = qwRelativeAddress / qwBlockSize;
    if (kFreeBuddyBlock(iBlockListIndex, iBitmapOffset)) {
        gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
        return TRUE;
    }
    return FALSE;
}

static BOOL kFreeBuddyBlock(const int iBlockListIndex, int iBlockOffset) {
    const BOOL bPreviousInterruptFlag = kLockForSystemData();

    for (int i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);

        int iBuddyBlockOffset;
        if (iBlockOffset % 2 == 0) {
            iBuddyBlockOffset = iBlockOffset + 1;
        } else {
            iBuddyBlockOffset = iBlockOffset - 1;
        }
        BOOL bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

        if (bFlag == DYNAMICMEMORY_EMPTY) {
            break;
        }

        kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
        kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

        iBlockOffset = iBlockOffset / 2;
    }

    kUnlockForSystemData(bPreviousInterruptFlag);
    return TRUE;
}

static BYTE kGetFlagInBitmap(const int iBlockListIndex, const int iOffset) {
    const BYTE *pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
    if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0x00) {
        return DYNAMICMEMORY_EXIST;
    }
    return DYNAMICMEMORY_EMPTY;
}

// ETC

void kGetDynamicMemoryInformation(QWORD *pqwDynamicMemoryStartAddress, QWORD *pqwDynamicMemoryTotalSize,
        QWORD *pqwMetaDataSize, QWORD *pqwUsedMemorySize) {
    *pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
    *pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
    *pqwMetaDataSize = kCalculateMetaBlockCount(*pqwDynamicMemoryTotalSize * DYNAMICMEMORY_MIN_SIZE);
    *pqwUsedMemorySize = gs_stDynamicMemory.qwUsedSize;
}

DYNAMICMEMORY *kGetDynamicMemoryManager() {
    return &gs_stDynamicMemory;
}

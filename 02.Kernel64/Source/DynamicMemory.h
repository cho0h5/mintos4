#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"
#include "Task.h"

#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + \
            (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)
#define DYNAMICMEMORY_MIN_SIZE      (1 * 1024)

#define DYNAMICMEMORY_EXIST         0x01    // Not being used
#define DYNAMICMEMORY_EMPTY         0x00    // Being used

#pragma pack(push, 1)

typedef struct kBitmapStruct {
    BYTE *pbBitmap;
    QWORD qwExistBitCount;
} BITMAP;

typedef struct kDynamicMemoryManagerStruct {
    int iMaxLevelCount;
    int iBlockCountOfSmallestBlock;
    QWORD qwUsedSize;

    QWORD qwStartAddress;
    QWORD qwEndAddress;

    BYTE *pbAllocatedBlockListIndex;
    BITMAP *pstBitmapOfLevel;
} DYNAMICMEMORY;

#pragma pack(pop)

// Initialize

void kInitializeDynamicMemory();
static QWORD kCalculateDynamicMemorySize();
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);

// Allocation

void *kAllocateMemory(const QWORD qwSize);
static QWORD kGetBuddyBlockSize(const QWORD qwSize);
static int kAllocationBuddyBlock(const QWORD qwAlignedSize);
static int kGetBlockListIndexOfMatchSize(const QWORD qwAlignedSize);
static int kFindFreeBlockInBitmap(const int iBlockListIndex);
static void kSetFlagInBitmap(const int iBlockListIndex, const int iOffset, const BYTE bFlag);

// Deallocation

BOOL kFreeMemory(const void *pvAddress);
static BOOL kFreeBuddyBlock(const int iBlockListIndex, const int iBlockOffset);
static BYTE kGetFlagInBitmap(const int iBlockListIndex, const int iOffset);

#endif

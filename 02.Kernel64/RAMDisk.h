#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include "Types.h"
#include "Syncronization.h"
#include "HardDisk.h"

#define RDD_TOTALSECTORCOUNT    (8 * 1024 * 1024 / 512)

typedef struct kRDDManagerStruct {
    BYTE *pbBuffer;
    DWORD dwTotalSectorCount;
    MUTEX stMutex;
} RDDMANAGER;

BOOL kInitializeRDD(const DWORD dwTotalSectorCount);
BOOL kReadRDDInformation(const BOOL bPrimary, const BOOL bMaster, HDDINFORMATION *pstHDDInformation);
int kReadRDDSector(const BOOL bPrimary, const BOOL bMaster, const DWORD dwLBA,
        const int  iSectorCount, char *pcBuffer);
int kWriteRDDSector(const BOOL bPrimary, const BOOL bMaster, const DWORD dwLBA,
        const int iSectorCount, const char *pcBuffer);
#endif

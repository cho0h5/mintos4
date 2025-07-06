#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Syncronization.h"
#include "HardDisk.h"

#define FILESYSTEM_SIGNATURE                0xb300cf54
#define FILESYSTEM_SECTORSPERCLUSTER        8
#define FILESYSTEM_LASTCLUSTER              0xffffffff
#define FILESYSTEM_FREECLUSTER              0x00
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT   ((FILESYSTEM_SECTORSPERCLUSTER * 512) / sizeof(DIRECTORYENTRY))
#define FILESYSTEM_CLUSTERSIZE              (FILESYSTEM_SECTORSPERCLUSTER * 512)
#define FILESYSTEM_MAXFILENAMELENGTH        24

typedef BOOL (*fReadHDDInformation)(const BOOL bPrimary, const BOOL bMaster,
        HDDINFORMATION *pstHDDInformation);
typedef int (*fReadHDDSector)(const BOOL bPrimary, const BOOL bMaster,
        const DWORD dwLBA, const int iSectorCount, char *pcBuffer);
typedef int (*fWriteHDDSector)(const BOOL bPrimary, const BOOL bMaster,
        const DWORD dwLBA, const int iSectorCount, const char *pcBuffer);

#pragma pack(push, 1)

typedef struct kPartitionStruct {
    BYTE bBootableFlag;
    BYTE vbStartingCHSAddress[3];
    BYTE bPartitionType;
    BYTE vbEndingCHSAddress[3];
    DWORD dwStartingLBAAddress;
    DWORD dwSizeInSector;
} PARTITION;

typedef struct kMBRStruct {
    BYTE vbBootCode[430];

    DWORD dwSignature;
    DWORD dwReservedSectorCount;    // Unit: Sector
    DWORD dwClusterLinkSectorCount; // Unit: Sector
    DWORD dwTotalClusterCount;      // Unit: Cluster

    PARTITION vstPartition[4];
    BYTE vbBootLoaderSignature[2];
} MBR;

typedef struct kDirectoryEntryStruct {
    char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];
    DWORD dwFileSize;
    DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack(pop)

typedef struct kFileHandleStruct {
    int iDirectoryEntryOffset;
    DWORD dwFileSize;
    DWORD dwStartClusterIndex;
    DWORD dwCurrentClusterIndex;
    DWORD dwPreviousClusterIndex;
    DWORD dwCurrentOffset;
} FILEHANDLE;

typedef struct kDirectoryHandleStruct {
    DIRECTORYENTRY *pstDirectoryBuffer;
    int iCurrentOffset;
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct {
    BYTE bType;

    union {
        FILEHANDLE stFileHandle;
        DIRECTORYHANDLE stDirectoryHandle;
    };
} FILE, DIR;

typedef struct kFileSystemManagerStruct {
    BOOL bMounted;

    DWORD dwReservedSectorCount;
    DWORD dwClusterLinkAreaStartAddress;
    DWORD dwClusterLinkAreaSize;                    // Unit: Sector
    DWORD dwDataAreaStartAddress;
    DWORD dwTotalClusterCount;                      // Unit: Cluster

    DWORD dwLastAllocatedClusterLinkSectorOffset;   // Unit: Sector, For optimization

    MUTEX stMutex;
} FILESYSTEMMANAGER;

BOOL kInitializeFileSystem();
BOOL kMount();
BOOL kFormat();
BOOL kGetHDDInformation(HDDINFORMATION *pstInformation);
BOOL kReadClusterLinkTable(const DWORD dwOffset, BYTE *pbBuffer);
BOOL kWriteClusterLinkTable(const DWORD dwOffset, const BYTE *pbBuffer);
DWORD kFindFreeCluster();
BOOL kSetClusterLinkData(const DWORD dwClusterIndex, const DWORD dwData);
BOOL kGetClusterLinkData(const DWORD dwClusterIndex, DWORD *pdwData);
BOOL kReadCluster(const DWORD dwOffset, BYTE *pbBuffer);
BOOL kWriteCluster(const DWORD dwOffset, const BYTE *pbBuffer);
int kFindFreeDirectoryEntry();
BOOL kSetDirectoryEntryData(const int iIndex, const DIRECTORYENTRY *pstEntry);
BOOL kGetDirectoryEntryData(const int iIndex, DIRECTORYENTRY *pstEntry);
int kFindDirectoryEntry(const char *pcFileName, DIRECTORYENTRY *pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER *pstManager);

#endif

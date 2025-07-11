#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Syncronization.h"
#include "HardDisk.h"
#include "CacheManager.h"

#define FILESYSTEM_SIGNATURE                0xb300cf54
#define FILESYSTEM_SECTORSPERCLUSTER        8
#define FILESYSTEM_LASTCLUSTER              0xffffffff
#define FILESYSTEM_FREECLUSTER              0x00
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT   ((FILESYSTEM_SECTORSPERCLUSTER * 512) / sizeof(DIRECTORYENTRY))
#define FILESYSTEM_CLUSTERSIZE              (FILESYSTEM_SECTORSPERCLUSTER * 512)

#define FILESYSTEM_HANDLE_MAXCOUNT          (TASK_MAXCOUNT * 3)

#define FILESYSTEM_MAXFILENAMELENGTH        24

#define FILESYSTEM_TYPE_FREE                0
#define FILESYSTEM_TYPE_FILE                1
#define FILESYSTEM_TYPE_DIRECTORY           2

#define FILESYSTEM_SEEK_SET                 0
#define FILESYSTEM_SEEK_CUR                 1
#define FILESYSTEM_SEEK_END                 2

#define fopen                               kOpenFile
#define fread                               kReadFile
#define fwrite                              kWriteFile
#define fseek                               kSeekFile
#define fclose                              kCloseFile
#define remove                              kRemoveFile
#define opendir                             kOpenDirectory
#define readdir                             kReadDirectory
#define rewinddir                           kRewindDirectory
#define closedir                            kCloseDirectory

#define SEEK_SET                            FILESYSTEM_SEEK_SET
#define SEEK_CUR                            FILESYSTEM_SEEK_CUR
#define SEEK_END                            FILESYSTEM_SEEK_END

#define dirent                              kDirectoryEntryStruct
#define d_name                              vcFileName

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
    int iDirectoryEntryOffset;      // Directory Entry Offset
    DWORD dwFileSize;               // File Size                Unit: Byte
    DWORD dwStartClusterIndex;      // Start Cluster Index      Unit: Cluster
    DWORD dwCurrentClusterIndex;    // Current Cluster Index    Unit: Cluster
    DWORD dwPreviousClusterIndex;   // Previous Cluster Index   Unit: Cluster
    DWORD dwCurrentOffset;          // Current Offset           Unit: Byte
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

    FILE *pstHandlePool;

    BOOL bCacheEnable;
} FILESYSTEMMANAGER;

BOOL kInitializeFileSystem();
BOOL kMount();
BOOL kFormat();
BOOL kGetHDDInformation(HDDINFORMATION *pstInformation);

// Low Level Function
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

// Cache
static BOOL kInternalReadClusterLinkTableWithoutCache(const DWORD dwOffset, BYTE *pbBuffer);
static BOOL kInternalReadClusterLinkTableWithCache(const DWORD dwOffset, BYTE *pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithoutCache(const DWORD dwOffset, const BYTE *pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithCache(const DWORD dwOffset, const BYTE *pbBuffer);
static BOOL kInternalReadClusterWithoutCache(const DWORD dwOffset, BYTE *pbBuffer);
static BOOL kInternalReadClusterWithCache(const DWORD dwOffset, BYTE *pbBuffer);
static BOOL kInternalWriteClusterWithoutCache(const DWORD dwOffset, const BYTE *pbBuffer);
static BOOL kInternalWriteClusterWithCache(const DWORD dwOffset, const BYTE *pbBuffer);

static CACHEBUFFER *kAllocateCacheBufferWithFlush(const int iCacheTableIndex);
BOOL kFlushFileSystemCache();

// High Level Function
FILE *kOpenFile(const char *pcFileName, const char *pcMode);
DWORD kReadFile(void *pvBuffer, const DWORD dwSize, const DWORD dwCount, FILE *pstFile);
DWORD kWriteFile(const void *pvBuffer, const DWORD dwSize, const DWORD dwCount, FILE *pstFile);
int kSeekFile(FILE *pstFile, const int iOffset, const int iOrigin);
int kCloseFile(FILE *pstFile);
int kRemoveFile(const char *pcFileName);
DIR *kOpenDirectory(const char *pcDirectoryName);
struct kDirectoryEntryStruct *kReadDirectory(DIR *pstDirectory);
void kRewindDirectory(DIR *pstDirectory);
int kCloseDirectory(DIR *pstDirectory);
BOOL kWriteZero(FILE *pstFile, const DWORD dwCount);
BOOL kIsFileOpened(const DIRECTORYENTRY *pstEntry);

static void *kAllocateFileDirectoryHandle();
static void kFreeFileDirectoryHandle(FILE *pstFile);
static BOOL kCreateFile(const char *pcFileName, DIRECTORYENTRY *pstEntry, int *piDirectoryEntryIndex);
static BOOL kFreeClusterUntilEnd(const DWORD dwClusterIndex);
static BOOL kUpdateDirectoryEntry(const FILEHANDLE *pstFileHandle);

#endif

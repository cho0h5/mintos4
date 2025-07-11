#include "FileSystem.h"
#include "Utility.h"
#include "DynamicMemory.h"
#include "Console.h"

static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];

fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

BOOL kInitializeFileSystem() {
    BOOL bCacheEnable = FALSE;

    kMemSet(&gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
    kInitializeMutex(&gs_stFileSystemManager.stMutex);

    if (kInitializeHDD()) {
        gs_pfReadHDDInformation = kReadHDDInformation;
        gs_pfReadHDDSector = kReadHDDSector;
        gs_pfWriteHDDSector = kWriteHDDSector;

        bCacheEnable = TRUE;
    } else {
        return FALSE;
    }

    if (!kMount()) {
        return FALSE;
    }

    gs_stFileSystemManager.pstHandlePool = (FILE *)kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
    if (gs_stFileSystemManager.pstHandlePool == NULL) {
        gs_stFileSystemManager.bMounted = FALSE;
        return TRUE;
    }

    kMemSet(gs_stFileSystemManager.pstHandlePool, 0, FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

    if (bCacheEnable) {
        gs_stFileSystemManager.bCacheEnable = kInitializeCacheManager();
    }
    return TRUE;
}

BOOL kMount() {
    kLock(&gs_stFileSystemManager.stMutex);

    if (!gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return FALSE;
    }

    const MBR *pstMBR = (MBR *)gs_vbTempBuffer;
    if (pstMBR->dwSignature != FILESYSTEM_SIGNATURE) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return FALSE;
    }

    gs_stFileSystemManager.bMounted = TRUE;

    gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress = 1 + pstMBR->dwReservedSectorCount;
    gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
    gs_stFileSystemManager.dwDataAreaStartAddress = 1 + pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount;
    gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

    kUnlock(&gs_stFileSystemManager.stMutex);
    return TRUE;
}

BOOL kFormat() {
    kLock(&gs_stFileSystemManager.stMutex);

    HDDINFORMATION *pstHDD = (HDDINFORMATION *)gs_vbTempBuffer;
    if (!gs_pfReadHDDInformation(TRUE, TRUE, pstHDD)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return FALSE;
    }
    const DWORD dwTotalSectorCount = pstHDD->dwTotalSectors;

    // Estimate max number of clusters
    const DWORD dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;
    DWORD dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;
    const DWORD dwReaminSectorCount = dwTotalSectorCount - 1 - dwClusterLinkSectorCount;
    const DWORD dwClusterCount = dwReaminSectorCount / FILESYSTEM_SECTORSPERCLUSTER;
    dwClusterLinkSectorCount = (dwClusterCount + 127) / 128;

    // Update MBR
    if (!gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return FALSE;
    }
    MBR *pstMBR = (MBR *)gs_vbTempBuffer;
    kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
    pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
    pstMBR->dwReservedSectorCount = 0;
    pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
    pstMBR->dwTotalClusterCount = dwClusterCount;
    if (!gs_pfWriteHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return FALSE;
    }

    // Format ClusterLinkTable and RootDirectory
    kMemSet(gs_vbTempBuffer, 0, 512);
    for (int i = 0; i < dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER; i++) {
        if (i == 0) {
            ((DWORD *)gs_vbTempBuffer)[0] = FILESYSTEM_LASTCLUSTER;
        } else {
            ((DWORD *)gs_vbTempBuffer)[0] = FILESYSTEM_FREECLUSTER;
        }

        if (!gs_pfWriteHDDSector(TRUE, TRUE, i + 1, 1, gs_vbTempBuffer)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return FALSE;
        }
    }

    // Clear Cache
    if (gs_stFileSystemManager.bCacheEnable) {
        kDiscardAllCacheBuffer(CACHE_CLUSTERLINKTABLEAREA);
        kDiscardAllCacheBuffer(CACHE_DATAAREA);
    }

    kUnlock(&gs_stFileSystemManager.stMutex);
    return TRUE;
}

BOOL kGetHDDInformation(HDDINFORMATION *pstInformation) {
    kLock(&gs_stFileSystemManager.stMutex);

    const BOOL bResult = gs_pfReadHDDInformation(TRUE, TRUE, pstInformation);

    kUnlock(&gs_stFileSystemManager.stMutex);
    return bResult;
}

BOOL kReadClusterLinkTable(const DWORD dwOffset, BYTE *pbBuffer) {
    if (!gs_stFileSystemManager.bCacheEnable) {
        return kInternalReadClusterLinkTableWithoutCache(dwOffset, pbBuffer);
    } else {
        return kInternalReadClusterLinkTableWithCache(dwOffset, pbBuffer);
    }
}

static BOOL kInternalReadClusterLinkTableWithoutCache(const DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

static BOOL kInternalReadClusterLinkTableWithCache(const DWORD dwOffset, BYTE *pbBuffer) {
    CACHEBUFFER *pstCacheBuffer = kFindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, dwOffset);

    if (pstCacheBuffer != NULL) {
        kMemCpy(pbBuffer, pstCacheBuffer->pbBuffer, 512);
        return TRUE;
    }

    if (!kInternalReadClusterLinkTableWithoutCache(dwOffset, pbBuffer)) {
        return FALSE;
    }

    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
    if (pstCacheBuffer == NULL) {
        return FALSE;
    }

    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
    pstCacheBuffer->dwTag = dwOffset;

    pstCacheBuffer->bChanged = FALSE;
    return TRUE;
}

static CACHEBUFFER *kAllocateCacheBufferWithFlush(const int iCacheTableIndex) {
    CACHEBUFFER *pstCacheBuffer = kAllocateCacheBuffer(iCacheTableIndex);
    if (pstCacheBuffer != NULL) {
        return pstCacheBuffer;
    }

    pstCacheBuffer = kGetVictimInCacheBuffer(iCacheTableIndex);
    if (pstCacheBuffer == NULL) {
        kPrintf("Cache Allocate Failed\n");
        return NULL;
    }

    if (!pstCacheBuffer->bChanged) {
        return pstCacheBuffer;
    }

    switch(iCacheTableIndex) {
        case CACHE_CLUSTERLINKTABLEAREA:
            if (!kInternalWriteClusterLinkTableWithoutCache(pstCacheBuffer->dwTag, pstCacheBuffer->pbBuffer)) {
                kPrintf("Cache Buffer Write Failed\n");
                return NULL;
            }
            break;

        case CACHE_DATAAREA:
            if (!kInternalWriteClusterWithoutCache(pstCacheBuffer->dwTag, pstCacheBuffer->pbBuffer)) {
                kPrintf("Cache Buffer Write Failed\n");
                return NULL;
            }
            break;

        default:
            kPrintf("kAllocateCacheBufferWithFlush Failed\n");
            return NULL;
            break;
    }

    return pstCacheBuffer;
}

BOOL kWriteClusterLinkTable(const DWORD dwOffset, const BYTE *pbBuffer) {
    if (!gs_stFileSystemManager.bCacheEnable) {
        return kInternalWriteClusterLinkTableWithoutCache(dwOffset, pbBuffer);
    } else {
        return kInternalWriteClusterLinkTableWithoutCache(dwOffset, pbBuffer);
    }
}

static BOOL kInternalWriteClusterLinkTableWithoutCache(const DWORD dwOffset, const BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

static BOOL kInternalWriteClusterLinkTableWithCache(const DWORD dwOffset, const BYTE *pbBuffer) {
    CACHEBUFFER *pstCacheBuffer = kFindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, dwOffset);
    if (pstCacheBuffer != NULL) {
        kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);

        pstCacheBuffer->bChanged = TRUE;
        return TRUE;
    }

    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
    if (pstCacheBuffer == NULL) {
        return FALSE;
    }

    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
    pstCacheBuffer->dwTag = dwOffset;

    pstCacheBuffer->bChanged = TRUE;

    return TRUE;
}

BOOL kReadCluster(const DWORD dwOffset, BYTE *pbBuffer) {
    if (!gs_stFileSystemManager.bCacheEnable) {
        return kInternalReadClusterWithoutCache(dwOffset, pbBuffer);
    } else {
        return kInternalReadClusterWithCache(dwOffset, pbBuffer);
    }
}

static BOOL kInternalReadClusterWithoutCache(const DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE,
            dwOffset * FILESYSTEM_SECTORSPERCLUSTER + gs_stFileSystemManager.dwDataAreaStartAddress,
            FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

static BOOL kInternalReadClusterWithCache(const DWORD dwOffset, BYTE *pbBuffer) {
    CACHEBUFFER *pstCacheBuffer = kFindCacheBuffer(CACHE_DATAAREA, dwOffset);

    if (pstCacheBuffer != NULL) {
        kMemCpy(pbBuffer, pstCacheBuffer->pbBuffer, FILESYSTEM_CLUSTERSIZE);
        return TRUE;
    }

    if (!kInternalReadClusterWithoutCache(dwOffset, pbBuffer)) {
        return FALSE;
    }

    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_DATAAREA);
    if (pstCacheBuffer == NULL) {
        return FALSE;
    }

    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
    pstCacheBuffer->dwTag = dwOffset;

    pstCacheBuffer->bChanged = FALSE;
    return TRUE;
}

BOOL kWriteCluster(const DWORD dwOffset, const BYTE *pbBuffer) {
    if (!gs_stFileSystemManager.bCacheEnable) {
        kInternalWriteClusterWithoutCache(dwOffset, pbBuffer);
    } else {
        kInternalWriteClusterWithCache(dwOffset, pbBuffer);
    }
}

static BOOL kInternalWriteClusterWithoutCache(const DWORD dwOffset, const BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE,
            dwOffset * FILESYSTEM_SECTORSPERCLUSTER + gs_stFileSystemManager.dwDataAreaStartAddress,
            FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

static BOOL kInternalWriteClusterWithCache(const DWORD dwOffset, const BYTE *pbBuffer) {
    CACHEBUFFER *pstCacheBuffer = kFindCacheBuffer(CACHE_DATAAREA, dwOffset);

    if (pstCacheBuffer != NULL) {
        kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);

        pstCacheBuffer->bChanged = TRUE;
        return TRUE;
    }

    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_DATAAREA);
    if (pstCacheBuffer == NULL) {
        return FALSE;
    }

    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
    pstCacheBuffer->dwTag = dwOffset;

    pstCacheBuffer->bChanged = TRUE;
    return TRUE;
}

DWORD kFindFreeCluster() {
    if (!gs_stFileSystemManager.bMounted) {
        return FILESYSTEM_LASTCLUSTER;
    }

    const DWORD dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

    for (int i = 0; i < gs_stFileSystemManager.dwClusterLinkAreaSize; i++) {
        DWORD dwLinkCountInSector;
        if (dwLastSectorOffset + i == gs_stFileSystemManager.dwClusterLinkAreaSize - 1) {
            dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
        } else {
            dwLinkCountInSector = 128;
        }

        const DWORD dwCurrentSectorOffset = (dwLastSectorOffset + i) % gs_stFileSystemManager.dwClusterLinkAreaSize;
        if (!kReadClusterLinkTable(dwCurrentSectorOffset, gs_vbTempBuffer)) {
            return FILESYSTEM_LASTCLUSTER;
        }

        int j = 0;
        for (; j < dwLinkCountInSector; j++) {
            if (((DWORD *)gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER) {
                break;
            }
        }

        if (j != dwLinkCountInSector) {
            gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;
            return (dwCurrentSectorOffset * 128) + j;
        }
    }

    return FILESYSTEM_LASTCLUSTER;
}

BOOL kSetClusterLinkData(const DWORD dwClusterIndex, const DWORD dwData) {
    if (!gs_stFileSystemManager.bMounted) {
        return FALSE;
    }

    const DWORD dwSectorOffset = dwClusterIndex / 128;
    if (!kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer)) {
        return FALSE;
    }

    ((DWORD *)gs_vbTempBuffer)[dwClusterIndex % 128] = dwData;

    if (!kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer)) {
        return FALSE;
    }

    return TRUE;
}

BOOL kGetClusterLinkData(const DWORD dwClusterIndex, DWORD *pdwData) {
    if (!gs_stFileSystemManager.bMounted) {
        return FALSE;
    }

    const DWORD dwSectorOffset = dwClusterIndex / 128;
    if (dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize) {
        return FALSE;
    }

    if (!kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer)) {
        return FALSE;
    }

    *pdwData = ((DWORD *)gs_vbTempBuffer)[dwClusterIndex % 128];
    return TRUE;
}

int kFindFreeDirectoryEntry() {
    if (!gs_stFileSystemManager.bMounted) {
        return -1;
    }

    if (!kReadCluster(0, gs_vbTempBuffer)) {
        return -1;
    }

    DIRECTORYENTRY *pstEntry = (DIRECTORYENTRY *)gs_vbTempBuffer;
    for (int i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if (pstEntry[i].dwStartClusterIndex == FILESYSTEM_FREECLUSTER) {
            return i;
        }
    }

    return -1;
}

BOOL kSetDirectoryEntryData(const int iIndex, const DIRECTORYENTRY *pstEntry) {
    if (!gs_stFileSystemManager.bMounted || iIndex < 0 || iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        return FALSE;
    }

    if (!kReadCluster(0, gs_vbTempBuffer)) {
        return FALSE;
    }

    DIRECTORYENTRY *pstRootEntry = (DIRECTORYENTRY *)gs_vbTempBuffer;
    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));

    if (!kWriteCluster(0, gs_vbTempBuffer)) {
        return FALSE;
    }
    return TRUE;
}

BOOL kGetDirectoryEntryData(const int iIndex, DIRECTORYENTRY *pstEntry) {
    if (!gs_stFileSystemManager.bMounted || iIndex < 0 || iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        return FALSE;
    }

    if (!kReadCluster(0, gs_vbTempBuffer)) {
        return FALSE;
    }

    const DIRECTORYENTRY *pstRootEntry = (DIRECTORYENTRY *)gs_vbTempBuffer;
    kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));
    return TRUE;
}

int kFindDirectoryEntry(const char *pcFileName, DIRECTORYENTRY *pstEntry) {
    if (!gs_stFileSystemManager.bMounted) {
        return -1;
    }

    if (!kReadCluster(0, gs_vbTempBuffer)) {
        return -1;
    }

    const int iLength = kStrLen(pcFileName);
    DIRECTORYENTRY *pstRootEntry = (DIRECTORYENTRY *)gs_vbTempBuffer;

    for (int i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if (kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLength) == 0) {
            kMemCpy(pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
            return i;
        }
    }
    return -1;
}

void kGetFileSystemInformation(FILESYSTEMMANAGER *pstManager) {
    kMemCpy(pstManager, &gs_stFileSystemManager, sizeof(gs_stFileSystemManager));
}

static void *kAllocateFileDirectoryHandle() {
    FILE *pstFile = gs_stFileSystemManager.pstHandlePool;
    for (int i = 0; i <FILESYSTEM_HANDLE_MAXCOUNT; i++) {
        if (pstFile->bType == FILESYSTEM_TYPE_FREE) {
            pstFile->bType = FILESYSTEM_TYPE_FILE;
            return pstFile;
        }

        pstFile++;
    }

    return NULL;
}

static void kFreeFileDirectoryHandle(FILE *pstFile) {
    kMemSet(pstFile, 0, sizeof(FILE));
    pstFile->bType = FILESYSTEM_TYPE_FREE;
}

static BOOL kCreateFile(const char *pcFileName, DIRECTORYENTRY *pstEntry, int *piDirectoryEntryIndex) {
    // Find free cluster
    const DWORD dwCluster = kFindFreeCluster();
    if (dwCluster == FILESYSTEM_LASTCLUSTER || !kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER)) {
        return FALSE;
    }

    // Find free directory entry
    *piDirectoryEntryIndex = kFindFreeDirectoryEntry();
    if (*piDirectoryEntryIndex == -1) {
        kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        return FALSE;
    }

    // Set directory entry
    kMemCpy(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName) + 1);
    pstEntry->dwStartClusterIndex = dwCluster;
    pstEntry->dwFileSize = 0;

    if (!kSetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry)) {
        kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        return FALSE;
    }

    return TRUE;
}

static BOOL kFreeClusterUntilEnd(const DWORD dwClusterIndex) {
    DWORD dwCurrentClusterIndex = dwClusterIndex;

    while (dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER) {
        DWORD dwNextClusterIndex;
        if (!kGetClusterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex)) {
            return FALSE;
        }

        if (!kSetClusterLinkData(dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER)) {
            return FALSE;
        }

        dwCurrentClusterIndex = dwNextClusterIndex;
    }

    return TRUE;
}

FILE *kOpenFile(const char *pcFileName, const char *pcMode) {
    const int iFileNameLength = kStrLen(pcFileName);
    DIRECTORYENTRY stEntry;
    if (iFileNameLength == 0 || iFileNameLength > sizeof(stEntry.vcFileName) - 1) {
        return NULL;
    }

    kLock(&gs_stFileSystemManager.stMutex);

    int iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
    if (iDirectoryEntryOffset == -1) {
        if (pcMode[0] == 'r') {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return NULL;
        }

        if (!kCreateFile(pcFileName, &stEntry, &iDirectoryEntryOffset)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return NULL;
        }
    } else if (pcMode[0] == 'w') {
        DWORD dwSecondCluster;
        if (!kGetClusterLinkData(stEntry.dwStartClusterIndex, &dwSecondCluster)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return NULL;
        }

        if (!kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_LASTCLUSTER)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return NULL;
        }

        if (!kFreeClusterUntilEnd(dwSecondCluster)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return NULL;
        }

        stEntry.dwFileSize = 0;
        if (!kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return NULL;
        }
    }

    FILE *pstFile = kAllocateFileDirectoryHandle();
    if (pstFile == NULL) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return NULL;
    }

    pstFile->bType = FILESYSTEM_TYPE_FILE;
    pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
    pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
    pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentOffset = 0;

    if (pcMode[0] == 'a') {
        kSeekFile(pstFile, 0, FILESYSTEM_SEEK_END);
    }

    kUnlock(&gs_stFileSystemManager.stMutex);
    return pstFile;
}

DWORD kReadFile(void *pvBuffer, const DWORD dwSize, const DWORD dwCount, FILE *pstFile) {
    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE) {
        return 0;
    }

    FILEHANDLE *pstFileHandle = &pstFile->stFileHandle;
    if (pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize ||
            pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) {
        return 0;
    }

    const DWORD dwTotalCount = MIN(dwSize * dwCount, pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset);

    kLock(&gs_stFileSystemManager.stMutex);

    DWORD dwReadCount = 0;
    while (dwReadCount != dwTotalCount) {
        if (!kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer)) {
            break;
        }

        const DWORD dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
        const DWORD dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwReadCount);

        kMemCpy((char *)pvBuffer + dwReadCount, gs_vbTempBuffer + dwOffsetInCluster, dwCopySize);

        dwReadCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;

        if ((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0) {
            DWORD dwNextClusterIndex;
            if (!kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex)) {
                break;
            }

            pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }

    kUnlock(&gs_stFileSystemManager.stMutex);

    return dwReadCount / dwSize;
}

DWORD kWriteFile(const void *pvBuffer, const DWORD dwSize, const DWORD dwCount, FILE *pstFile) {
    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE) {
        return 0;
    }
    FILEHANDLE *pstFileHandle = &pstFile->stFileHandle;
    const DWORD dwTotalCount = dwSize * dwCount;

    kLock(&gs_stFileSystemManager.stMutex);

    DWORD dwWriteCount = 0;
    while (dwWriteCount != dwTotalCount) {
        if (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) {
            const DWORD dwAllocatedClusterIndex = kFindFreeCluster();
            if (dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER) {
                break;
            }

            if (!kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER)) {
                break;
            }

            if (!kSetClusterLinkData(pstFileHandle->dwPreviousClusterIndex, dwAllocatedClusterIndex)) {
                kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER);
                break;
            }

            pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;
            kMemSet(gs_vbTempBuffer, 0, sizeof(gs_vbTempBuffer));
        } else if (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE != 0 ||
                dwTotalCount - dwWriteCount < FILESYSTEM_CLUSTERSIZE) {
            if (!kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer)) {
                break;
            }
        }

        const DWORD dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
        const DWORD dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwWriteCount);
        kMemCpy(gs_vbTempBuffer + dwOffsetInCluster, (char *)pvBuffer + dwWriteCount, dwCopySize);

        if (!kWriteCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer)) {
            break;
        }

        dwWriteCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;

        if (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE == 0) {
            DWORD dwNextClusterIndex;
            if (!kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex)) {
                break;
            }

            pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }

    if (pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset) {
        pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
        kUpdateDirectoryEntry(pstFileHandle);
    }

    kUnlock(&gs_stFileSystemManager.stMutex);
    return dwWriteCount / dwSize;
}

static BOOL kUpdateDirectoryEntry(const FILEHANDLE *pstFileHandle) {
    DIRECTORYENTRY stEntry;
    if (pstFileHandle == NULL || !kGetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry)) {
        return FALSE;
    }

    stEntry.dwFileSize = pstFileHandle->dwFileSize;
    stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;

    if (!kSetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry)) {
        return FALSE;
    }
    return TRUE;
}

int kSeekFile(FILE *pstFile, const int iOffset, const int iOrigin) {
    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE) {
        return 0;
    }
    FILEHANDLE *pstFileHandle = &pstFile->stFileHandle;

    DWORD dwRealOffset = 0;
    switch(iOrigin) {
        case FILESYSTEM_SEEK_SET:
            if (iOffset <= 0) {
                dwRealOffset = 0;
            } else {
                dwRealOffset = iOffset;
            }
            break;

        case FILESYSTEM_SEEK_CUR:
            if (iOffset < 0 && pstFileHandle->dwCurrentOffset <= (DWORD)-iOffset) {
                dwRealOffset = 0;
            } else {
                dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
            }
            break;

        case FILESYSTEM_SEEK_END:
            if (iOffset < 0 && pstFileHandle->dwFileSize <= (DWORD)-iOffset) {
                dwRealOffset = 0;
            } else {
                dwRealOffset = pstFileHandle->dwFileSize + iOffset;
            }
            break;
    }

    const DWORD dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
    const DWORD dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
    const DWORD dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

    DWORD dwMoveCount;
    DWORD dwStartClusterIndex;
    if (dwLastClusterOffset < dwClusterOffsetToMove) {
        dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    } else if (dwCurrentClusterOffset <= dwClusterOffsetToMove) {
        dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    } else {
        dwMoveCount = dwClusterOffsetToMove;
        dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
    }

    kLock(&gs_stFileSystemManager.stMutex);

    DWORD dwCurrentClusterIndex = dwStartClusterIndex;
    DWORD dwPreviousClusterIndex;
    for (int i = 0; i < dwMoveCount; i++) {
        dwPreviousClusterIndex = dwCurrentClusterIndex;

        if (!kGetClusterLinkData(dwPreviousClusterIndex, &dwCurrentClusterIndex)) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return -1;
        }
    }

    if (dwMoveCount > 0) {
        pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
    } else if (dwStartClusterIndex == pstFileHandle->dwStartClusterIndex) {
        pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = pstFileHandle->dwStartClusterIndex;
    }

    if (dwLastClusterOffset < dwClusterOffsetToMove) {
        pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;
        kUnlock(&gs_stFileSystemManager.stMutex);

        if (!kWriteZero(pstFile, dwRealOffset - pstFileHandle->dwFileSize)) {
            return 0;
        }
    }

    pstFileHandle->dwCurrentOffset = dwRealOffset;

    kUnlock(&gs_stFileSystemManager.stMutex);
    return 0;
}

BOOL kWriteZero(FILE *pstFile, const DWORD dwCount) {
    if (pstFile == NULL) {
        return FALSE;
    }

    BYTE *pbBuffer = (BYTE *)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if (pbBuffer == NULL) {
        return FALSE;
    }

    kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);
    DWORD dwRemainCount = dwCount;

    while (dwRemainCount != 0) {
        const DWORD dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE);
        if (kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) != dwWriteCount) {
            kFreeMemory(pbBuffer);
            return FALSE;
        }
        dwRemainCount -= dwWriteCount;
    }

    kFreeMemory(pbBuffer);
    return TRUE;
}

int kCloseFile(FILE *pstFile) {
    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE) {
        return -1;
    }

    kFreeFileDirectoryHandle(pstFile);
    return 0;
}

BOOL kIsFileOpened(const DIRECTORYENTRY *pstEntry) {
    FILE *pstFile = gs_stFileSystemManager.pstHandlePool;
    for (int i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
        if (pstFile[i].bType == FILESYSTEM_TYPE_FILE
                && pstFile[i].stFileHandle.dwStartClusterIndex == pstEntry->dwStartClusterIndex) {
            return TRUE;
        }
    }
    return FALSE;
}

int kRemoveFile(const char *pcFileName) {
    DIRECTORYENTRY stEntry;
    const int iFileNameLength = kStrLen(pcFileName);
    if (iFileNameLength > sizeof(stEntry.vcFileName) - 1 || iFileNameLength == 0) {
        return -1;
    }

    kLock(&gs_stFileSystemManager.stMutex);

    const int iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
    if (iDirectoryEntryOffset == -1) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return -1;
    }

    if (kIsFileOpened(&stEntry)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return -1;
    }

    if (!kFreeClusterUntilEnd(stEntry.dwStartClusterIndex)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return -1;
    }

    kMemSet(&stEntry, 0, sizeof(stEntry));
    if (!kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry)) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return -1;
    }

    kUnlock(&gs_stFileSystemManager.stMutex);
    return 0;
}

DIR *kOpenDirectory(const char *pcDirectoryName) {
    kLock(&gs_stFileSystemManager.stMutex);

    DIR *pstDirectory = kAllocateFileDirectoryHandle();
    if (pstDirectory == NULL) {
        kUnlock(&gs_stFileSystemManager.stMutex);
        return NULL;
    }

    DIRECTORYENTRY *pstDirectoryBuffer = (DIRECTORYENTRY *)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if (pstDirectoryBuffer == NULL) {
        kFreeFileDirectoryHandle(pstDirectory);

        kUnlock(&gs_stFileSystemManager.stMutex);
        return NULL;
    }

    if (!kReadCluster(0, (BYTE *)pstDirectoryBuffer)) {
        kFreeFileDirectoryHandle(pstDirectory);
        kFreeMemory(pstDirectoryBuffer);

        kUnlock(&gs_stFileSystemManager.stMutex);
        return NULL;
    }

    pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
    pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
    pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

    kUnlock(&gs_stFileSystemManager.stMutex);
    return pstDirectory;
}

struct kDirectoryEntryStruct *kReadDirectory(DIR *pstDirectory) {
    if (pstDirectory == NULL || pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY) {
        return NULL;
    }
    DIRECTORYHANDLE *pstDirectoryHandle = &pstDirectory->stDirectoryHandle;

    if (pstDirectoryHandle->iCurrentOffset < 0 ||
            pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        return NULL;
    }

    kLock(&gs_stFileSystemManager.stMutex);

    DIRECTORYENTRY *pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
    while (pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        if (pstEntry[pstDirectoryHandle->iCurrentOffset].dwStartClusterIndex != 0) {
            kUnlock(&gs_stFileSystemManager.stMutex);
            return &pstEntry[pstDirectoryHandle->iCurrentOffset++];
        }

        pstDirectoryHandle->iCurrentOffset++;
    }

    kUnlock(&gs_stFileSystemManager.stMutex);
    return NULL;
}

void kRewindDirectory(DIR *pstDirectory) {
    if (pstDirectory == NULL || pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY) {
        return;
    }
    DIRECTORYHANDLE *pstDirectoryHandle = &pstDirectory->stDirectoryHandle;

    kLock(&gs_stFileSystemManager.stMutex);

    pstDirectoryHandle->iCurrentOffset = 0;

    kUnlock(&gs_stFileSystemManager.stMutex);
}

int kCloseDirectory(DIR *pstDirectory) {
    if (pstDirectory == NULL || pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY) {
        return -1;
    }
    DIRECTORYHANDLE *pstDirectoryHandle = &pstDirectory->stDirectoryHandle;

    kLock(&gs_stFileSystemManager.stMutex);

    kFreeMemory(pstDirectoryHandle->pstDirectoryBuffer);
    kFreeFileDirectoryHandle(pstDirectory);

    kUnlock(&gs_stFileSystemManager.stMutex);
    return 0;
}

BOOL kFlushFileSystemCache() {
    if (!gs_stFileSystemManager.bCacheEnable) {
        return TRUE;
    }

    kLock(&gs_stFileSystemManager.stMutex);

    CACHEBUFFER *pstCacheBuffer;
    int iCacheCount;
    kGetCacheBufferAndCount(CACHE_CLUSTERLINKTABLEAREA, &pstCacheBuffer, &iCacheCount);
    for (int i = 0; i < iCacheCount; i++) {
        if (pstCacheBuffer[i].bChanged) {
            if (!kInternalWriteClusterLinkTableWithoutCache(pstCacheBuffer[i].dwTag, pstCacheBuffer[i].pbBuffer)) {
                return FALSE;
            }

            pstCacheBuffer[i].bChanged = FALSE;
        }
    }

    kGetCacheBufferAndCount(CACHE_DATAAREA, &pstCacheBuffer, &iCacheCount);
    for (int i = 0; i < iCacheCount; i++) {
        if (pstCacheBuffer[i].bChanged == TRUE) {
            if (!kInternalWriteClusterWithoutCache(pstCacheBuffer[i].dwTag, pstCacheBuffer[i].pbBuffer)) {
                return FALSE;
            }

            pstCacheBuffer[i].bChanged = FALSE;
        }
    }

    kUnlock(&gs_stFileSystemManager.stMutex);
    return TRUE;
}

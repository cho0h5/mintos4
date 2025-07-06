#include "FileSystem.h"
#include "Utility.h"

static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];

fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

BOOL kInitializeFileSystem() {
    kMemSet(&gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
    kInitializeMutex(&gs_stFileSystemManager.stMutex);

    if (kInitializeHDD() == TRUE) {
        gs_pfReadHDDInformation = kReadHDDInformation;
        gs_pfReadHDDSector = kReadHDDSector;
        gs_pfWriteHDDSector = kWriteHDDSector;
    } else {
        return FALSE;
    }

    if (!kMount()) {
        return FALSE;
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
    return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

BOOL kWriteClusterLinkTable(const DWORD dwOffset, const BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
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

BOOL kReadCluster(const DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE,
            dwOffset * FILESYSTEM_SECTORSPERCLUSTER + gs_stFileSystemManager.dwDataAreaStartAddress,
            FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

BOOL kWriteCluster(const DWORD dwOffset, const BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE,
            dwOffset * FILESYSTEM_SECTORSPERCLUSTER + gs_stFileSystemManager.dwDataAreaStartAddress,
            FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
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

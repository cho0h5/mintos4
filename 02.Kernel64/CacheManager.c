#include "CacheManager.h"
#include "Console.h"
#include "Utility.h"
#include "DynamicMemory.h"
#include "FileSystem.h"

static CACHEMANAGER gs_stCacheManager;

BOOL kInitializeCacheManager() {
    kMemSet(&gs_stCacheManager, 0, sizeof(gs_stCacheManager));

    gs_stCacheManager.vdwAccessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
    gs_stCacheManager.vdwAccessTime[CACHE_DATAAREA] = 0;

    gs_stCacheManager.vdwMaxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLEAREACOUNT;
    gs_stCacheManager.vdwMaxCount[CACHE_DATAAREA] = CACHE_MAXDATAAREACOUNT;

    gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] = (BYTE *)kAllocateMemory(CACHE_MAXCLUSTERLINKTABLEAREACOUNT * 512);
    if (gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] == NULL) {
        return FALSE;
    }

    for (int i = 0; i < CACHE_MAXCLUSTERLINKTABLEAREACOUNT; i++) {
        gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].pbBuffer =
            gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] + (i * 512);

        gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].dwTag = CACHE_INVALIDTAG;
    }

    gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] = (BYTE *)kAllocateMemory(CACHE_MAXDATAAREACOUNT * FILESYSTEM_CLUSTERSIZE);

    if (gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] == NULL) {
        kFreeMemory(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA]);
        return FALSE;
    }

    for (int i = 0; i < CACHE_MAXDATAAREACOUNT; i++) {
        gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].pbBuffer =
            gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] + (i * FILESYSTEM_CLUSTERSIZE);

        gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].dwTag = CACHE_INVALIDTAG;
    }

    return TRUE;
}

CACHEBUFFER *kAllocateCacheBuffer(const int iCacheTableIndex) {
    if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return NULL;
    }

    kCutDownAccessTime(iCacheTableIndex);

    CACHEBUFFER *pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for (int i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if (pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
            pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG - 1;
            pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;
            return &pstCacheBuffer[i];
        }
    }

    return NULL;
}

CACHEBUFFER *kFindCacheBuffer(const int iCacheTableIndex, DWORD dwTag) {
    if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return FALSE;
    }

    kCutDownAccessTime(iCacheTableIndex);

    CACHEBUFFER *pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for (int i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if (pstCacheBuffer[i].dwTag == dwTag) {
            pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;
            return &pstCacheBuffer[i];
        }
    }

    return NULL;
}

static void kCutDownAccessTime(const int iCacheTableIndex) {
    if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return;
    }

    if (gs_stCacheManager.vdwAccessTime[iCacheTableIndex] < 0xfffffffe) {
        return;
    }

    CACHEBUFFER *pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for (int j = 0; j < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1; j++) {
        BOOL bSorted = TRUE;
        for (int i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1 - j; i++) {
            if (pstCacheBuffer[i].dwAccessTime > pstCacheBuffer[i + 1].dwAccessTime) {
                bSorted = FALSE;

                CACHEBUFFER stTemp;
                kMemCpy(&stTemp, &pstCacheBuffer[i], sizeof(CACHEBUFFER));
                kMemCpy(&pstCacheBuffer[i], &pstCacheBuffer[i + 1], sizeof(CACHEBUFFER));
                kMemCpy(&pstCacheBuffer[i + 1], &stTemp, sizeof(CACHEBUFFER));
            }
        }

        if (bSorted) {
            break;
        }
    }

    int i = 0;
    for (; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        pstCacheBuffer[i].dwAccessTime = i;
    }

    gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = i;
}

CACHEBUFFER *kGetVictimInCacheBuffer(const int iCacheTableIndex) {
    if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return NULL;
    }

    int iOldIndex = -1;
    DWORD dwOldTime = 0xffffffff;

    CACHEBUFFER *pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for (int i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if (pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
            iOldIndex = i;
            break;
        }

        if (pstCacheBuffer[i].dwAccessTime < dwOldTime) {
            dwOldTime = pstCacheBuffer[i].dwAccessTime;
            iOldIndex = i;
        }
    }

    if (iOldIndex == -1) {
        kPrintf("Cache Buffer Find Error\n");
        return NULL;
    }

    pstCacheBuffer[iOldIndex].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

    return &pstCacheBuffer[iOldIndex];
}

void kDiscardAllCacheBuffer(const int iCacheTableIndex) {
    CACHEBUFFER *pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for (int i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG;
    }

    gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = 0;
}

BOOL kGetCacheBufferAndCount(const int iCacheTableIndex, CACHEBUFFER **ppstCacheBuffer, int *piMaxCount) {
    if (iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return FALSE;
    }

    *ppstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    *piMaxCount = gs_stCacheManager.vdwMaxCount[iCacheTableIndex];
    return TRUE;
}

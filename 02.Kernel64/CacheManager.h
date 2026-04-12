#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include "Types.h"

#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT  16
#define CACHE_MAXDATAAREACOUNT              32
#define CACHE_INVALIDTAG                    0xffffffff

#define CACHE_MAXCACHETABLEINDEX            2
#define CACHE_CLUSTERLINKTABLEAREA          0
#define CACHE_DATAAREA                      1

typedef struct kCacheBufferStruct {
    DWORD dwTag;
    DWORD dwAccessTime;
    BOOL bChanged;
    BYTE *pbBuffer;
} CACHEBUFFER;

typedef struct kCacheManagerStruct {
    DWORD vdwAccessTime[CACHE_MAXCACHETABLEINDEX];
    BYTE *vpbBuffer[CACHE_MAXCACHETABLEINDEX];
    CACHEBUFFER vvstCacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT];
    DWORD vdwMaxCount[CACHE_MAXCACHETABLEINDEX];
} CACHEMANAGER;

BOOL kInitializeCacheManager();
CACHEBUFFER *kAllocateCacheBuffer(const int iCacheTableIndex);
CACHEBUFFER *kFindCacheBuffer(const int iCacheTableIndex, DWORD dwTag);
static void kCutDownAccessTime(const int iCacheTableIndex);
CACHEBUFFER *kGetVictimInCacheBuffer(const int iCacheTableIndex);
void kDiscardAllCacheBuffer(const int iCacheTableIndex);
BOOL kGetCacheBufferAndCount(const int iCacheTableIndex, CACHEBUFFER **ppstCacheBuffer, int *piMaxCount);

#endif

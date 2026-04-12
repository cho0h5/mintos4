#include "List.h"

void kInitializeList(LIST *pstList) {
    pstList->iItemCount = 0;
    pstList->pvHeader = NULL;
    pstList->pvTail = NULL;
}

int kGetListCount(const LIST *pstList) {
    return pstList->iItemCount;
}

void kAddListToTail(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink = (LISTLINK *)pvItem;
    pstLink->pvNext = NULL;

    if (pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return;
    }

    pstLink = (LISTLINK *)pstList->pvTail;
    pstLink->pvNext = pvItem;

    pstList->pvTail = pvItem;
    pstList->iItemCount++;
}

void kAddListToHeader(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink = (LISTLINK *)pvItem;
    pstLink->pvNext = pstList->pvHeader;

    if (pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return;
    }

    pstList->pvHeader = pvItem;
    pstList->iItemCount++;
}

void *kRemoveList(LIST *pstList, QWORD qwID) {
    LISTLINK *pstPreviousLink = (LISTLINK *)pstList->pvHeader;
    LISTLINK *pstLink = pstPreviousLink;
    for (;pstLink != NULL; pstLink = pstLink->pvNext) {
        if (pstLink->qwID != qwID) {
            pstPreviousLink = pstLink;
            continue;
        }

        if (pstLink == pstList->pvHeader && pstLink == pstList->pvTail) {
            pstList->pvHeader = NULL;
            pstList->pvTail = NULL;
        } else if (pstLink == pstList->pvHeader) {
            pstList->pvHeader = pstLink->pvNext;
        } else if (pstLink == pstList->pvTail) {
            pstList->pvTail = pstPreviousLink;
        } else {
            pstPreviousLink->pvNext = pstLink->pvNext;
        }

        pstList->iItemCount--;
        return pstLink;
    }

    return NULL;
}

void *kRemoveListFromHeader(LIST *pstList) {
    if (pstList->iItemCount == 0) {
        return NULL;
    }

    LISTLINK *pstLink = (LISTLINK *)pstList->pvHeader;
    return kRemoveList(pstList, pstLink->qwID);
}

void *kRemoveListFromTail(LIST *pstList) {
    if (pstList->iItemCount == 0) {
        return NULL;
    }

    LISTLINK *pstLink = (LISTLINK *)pstList->pvTail;
    return kRemoveList(pstList, pstLink->qwID);
}

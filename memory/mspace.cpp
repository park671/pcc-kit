//
// Created by Park Yu on 2024/10/10.
//

#include "mspace.h"
#include "../logger/logger.h"

#define MSPACE_TAG "mspace"

using namespace std;

struct MemNode {
    void *addr;
    MemNode *next;
};

struct MemHead {
    const char *tag;
    MemHead *next;

    MemNode *memNode;
};

static MemHead *memHead = nullptr;

MemHead *foundOrCreateMemHead(const char *spaceTag) {
    MemHead *p = memHead;
    while (p != nullptr) {
        if (p->tag == spaceTag) {
            return p;
        }
        p = p->next;
    }
    MemHead *currentMemHead = (MemHead *) malloc(sizeof(MemHead));
    currentMemHead->tag = spaceTag;
    currentMemHead->next = nullptr;
    currentMemHead->memNode = nullptr;
    if (memHead == nullptr) {
        memHead = currentMemHead;
    } else {
        p = memHead;
        while (p->next != nullptr) {
            p = p->next;
        }
        p->next = currentMemHead;
    }
    return currentMemHead;
}

void appendMemNodeToHead(MemHead *currentMemHead, MemNode *currentMemNode) {
    if (currentMemHead->memNode == nullptr) {
        currentMemHead->memNode = currentMemNode;
        return;
    }
    MemNode *memNode = currentMemHead->memNode;
    while (memNode->next != nullptr) {
        memNode = memNode->next;
    }
    memNode->next = currentMemNode;
}

void *pccMalloc(const char *spaceTag, size_t size) {
    MemNode *memNode = (MemNode *) malloc(sizeof(MemNode));
    memNode->addr = malloc(size);
    memNode->next = nullptr;
    MemHead *currentMemHead = foundOrCreateMemHead(spaceTag);
    appendMemNodeToHead(currentMemHead, memNode);
    return memNode->addr;
}

void pccFree(const char *spaceTag, void *pointer) {
    MemHead *currentMemHead = foundOrCreateMemHead(spaceTag);
    while (currentMemHead != nullptr) {
        if (currentMemHead->tag == spaceTag) {
            MemNode *memNode = currentMemHead->memNode;
            MemNode *preMemNode = nullptr;
            while (memNode != nullptr) {
                if (memNode->addr == pointer) {
                    free(memNode->addr);

                    if (preMemNode != nullptr) {
                        preMemNode->next = memNode->next;
                    }
                    free(memNode);
                    return;
                }
                preMemNode = memNode;
                memNode = memNode->next;
            }
            break;
        }
        currentMemHead = currentMemHead->next;
    }
    loge(MSPACE_TAG, "internal error: free unknown memory, space=%s addr=%p", spaceTag, pointer);
}

void pccFreeSpace(const char *spaceTag) {
    logd(MSPACE_TAG, "free memory space: %s", spaceTag);
    MemHead *currentMemHead = foundOrCreateMemHead(spaceTag);
    while (currentMemHead != nullptr) {
        if (currentMemHead->tag == spaceTag) {
            MemNode *memNode = currentMemHead->memNode;
            while (memNode != nullptr) {
                free(memNode->addr);
                MemNode *p = memNode;
                memNode = memNode->next;
                free(p);
            }
            break;
        }
        currentMemHead = currentMemHead->next;
    }
}
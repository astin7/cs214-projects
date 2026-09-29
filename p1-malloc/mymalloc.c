#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mymalloc.h"

static const size_t memSize = 4096;
static const size_t headerSize = sizeof(size_t);
static const size_t alignBytes = 8;

// Minimum chunk = header + at least 8 bytes payload = 16 bytes
static const size_t minChunk = sizeof(size_t) + 8;

// Heap storage (union forces 8-byte alignment)
static union {
    unsigned char bytes[4096];
    long double align;
} heap;

static int initialized = 0;


// Header layout (stored in the first 8 bytes of each chunk):
//      - size in bytes (including header), multiple of 8
//      - low bit: 1 = allocated, 0 = free
// Because size is multiple of 8, low 3 bits are available for flags.

static inline size_t packHeader(size_t sizeBytes, int allocated) {
    size_t header = sizeBytes & ~(size_t)0x7;

    if (allocated) {
        header |= 1;
    }

    return header;
}

static inline size_t chunkSize(size_t header){
    return header & ~(size_t)0x7;
}

static inline int isAllocated(size_t header){
    return (header & (size_t)0x1) != 0;
}

static inline size_t roundUp8(size_t n) {
    return (n + (alignBytes - 1)) & ~(size_t)(alignBytes - 1);
}

static unsigned char *heapStart(void) { return heap.bytes; }
static unsigned char *heapEnd(void)   { return heap.bytes + memSize; }

static void leakCheck(void);

static void initHeap(void) {
    // Start with one big free chunk
    size_t *firstHeader = (size_t *)heap.bytes;
    *firstHeader = packHeader(memSize, 0);

    initialized = 1;
    atexit(leakCheck);
}

static unsigned char *nextChunk(unsigned char *chunkHeaderPtr) {
    size_t header = *(size_t *)chunkHeaderPtr;
    size_t sizeBytes = chunkSize(header);
    if (sizeBytes == 0) return NULL;
    return chunkHeaderPtr + sizeBytes;
}

// Find the chunk header that corresponds to a payload pointer
// Valid free pointers must be exactly the start of a chunk payload

static unsigned char *findHeaderFromPayload(void *ptr) {
    if (ptr == NULL) {
        return NULL;
    }

    unsigned char *p = (unsigned char *)ptr;

    // Must be inside heap and not before the first payload area
    if (p < heapStart() + headerSize || p >= heapEnd()) {
        return NULL;
    }

    unsigned char *curr = heapStart();
    while (curr < heapEnd()) {
        size_t header = *(size_t *)curr;
        size_t sizeBytes = chunkSize(header);

        
        if (sizeBytes < minChunk || curr + sizeBytes > heapEnd()) {
            return NULL;
        }

        if (curr + headerSize == p) {
            return curr; // exact payload start
        }

        curr += sizeBytes;
    }
    return NULL;
}

static void coalesceNext(unsigned char *curr) {
    unsigned char *next = nextChunk(curr);
    if (next == NULL || next >= heapEnd()) {
        return;
    }

    size_t currHeader = *(size_t *)curr;
    size_t nextHeader = *(size_t *)next;

    if (!isAllocated(currHeader) && !isAllocated(nextHeader)) {
        size_t newSize = chunkSize(currHeader) + chunkSize(nextHeader);
        *(size_t *)curr = packHeader(newSize, 0);
    }
}

static void coalescePrev(unsigned char *curr) {
    unsigned char *prev = NULL;
    unsigned char *scan = heapStart();

    while (scan < heapEnd()) {
        unsigned char *next = nextChunk(scan);
        if (next == curr) {
            prev = scan; break;
        }
        if (next == NULL || next <= scan){
            break;
        }
        scan = next;
    }

    if (prev != NULL) {
        coalesceNext(prev);
    }
}

void *mymalloc(size_t size, char *file, int line) {
    if (!initialized) {
        initHeap();
    }

    if (size == 0) {
        fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
        return NULL;
    }

    size_t payloadSize = roundUp8(size);
    size_t need = payloadSize + headerSize;

    unsigned char *curr = heapStart();
    while (curr < heapEnd()) {
        size_t header = *(size_t *)curr;
        size_t sizeBytes = chunkSize(header);

        
        if (sizeBytes < minChunk || curr + sizeBytes > heapEnd()) {
            fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
            return NULL;
        }

        if (!isAllocated(header) && sizeBytes >= need) {
            size_t remainder = sizeBytes - need;

            if (remainder >= minChunk) {
                // Split: allocated chunk + free remainder chunk
                *(size_t *)curr = packHeader(need, 1);

                unsigned char *rest = curr + need;
                *(size_t *)rest = packHeader(remainder, 0);
            }
            else{
                // Take whole chunk, avoid tiny remainder
                *(size_t *)curr = packHeader(sizeBytes, 1);
            }
            return (void *)(curr + headerSize);
        }
        curr += sizeBytes;
    }
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (!initialized) {
        initHeap();
    }

    if (ptr == NULL) {
        return;
    }

    unsigned char *headerPtr = findHeaderFromPayload(ptr);
    if (headerPtr == NULL) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }

    size_t header = *(size_t *)headerPtr;
    if (!isAllocated(header)) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }

    // Mark free
    *(size_t *)headerPtr = packHeader(chunkSize(header), 0);

    // Coalesce neighbors
    coalesceNext(headerPtr);
    coalescePrev(headerPtr);
}

static void leakCheck(void) {
    unsigned char *curr = heapStart();
    size_t leakedBytes = 0;
    size_t leakedObjects = 0;

    while (curr < heapEnd()) {
        size_t header = *(size_t *)curr;
        size_t sizeBytes = chunkSize(header);

        if (sizeBytes < minChunk || curr + sizeBytes > heapEnd()) {
            break;
        }

        if (isAllocated(header)) {
            leakedObjects++;
            leakedBytes += (sizeBytes - headerSize);
        }

        curr += sizeBytes;
    }

    if (leakedObjects > 0) {
        fprintf(stderr, "mymalloc: %zu bytes leaked in %zu objects.\n", leakedBytes, leakedObjects);
    }
}

//
// Created by canyie on 2020/3/11.
//

#include <sys/user.h>
#include <sys/mman.h>
#include <bits/sysconf.h>
#include <mutex>
#include "memory.h"
#include "../utils/log.h"
#include "../utils/lock.h"

using namespace pine;

static uintptr_t address = 0;
static size_t offset = 0;
static std::mutex mutex;

const long Memory::page_size = sysconf(_SC_PAGESIZE);

void *Memory::AllocUnprotected(size_t size) {
    if (UNLIKELY(size > page_size)) {
        LOGE("Attempting to allocate too much memory space (%x bytes)", size);
        errno = ENOMEM;
        return nullptr;
    }

    ScopedLock lock(mutex);

    if (LIKELY(address)) {
        size_t next_offset = offset + size;
        if (LIKELY(next_offset <= page_size)) {
            void *ptr = reinterpret_cast<void *>(address + offset);
            offset = next_offset;
            return ptr;
        }
    }

    void *mapped = mmap(nullptr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (UNLIKELY(mapped == MAP_FAILED)) {
        LOGE("Unable to allocate executable memory: %s (%d)", strerror(errno), errno);
        return nullptr;
    }
    memset(mapped, 0, page_size);
    address = reinterpret_cast<uintptr_t>(mapped);
    offset = size;
    return mapped;
}

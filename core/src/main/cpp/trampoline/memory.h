//
// Created by canyie on 2020/3/11.
//

#ifndef PINE_MEMORY_H
#define PINE_MEMORY_H

#include <cerrno>
#include <sys/mman.h>
#include "../utils/macros.h"
#include "../utils/log.h"

namespace pine {
    class Memory {
    public:
        static void *AllocUnprotected(size_t size);

        static inline bool Unprotect(void *ptr) {
            size_t alignment = (uintptr_t) ptr % page_size;
            void *aligned_ptr = (void *) ((uintptr_t) ptr - alignment);
            int result = mprotect(aligned_ptr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
            if (UNLIKELY(result == -1)) {
                LOGE("mprotect failed for %p: %s (%d)", ptr, strerror(errno), errno);
                return false;
            }
            return true;
        }

        static inline void FlushCache(void *addr, size_t size) {
            __builtin___clear_cache((char *) addr, (char *) ((uintptr_t) addr + size));
        }
    private:
        static const long page_size;
        DISALLOW_IMPLICIT_CONSTRUCTORS(Memory);
    };
}

#endif //PINE_MEMORY_H

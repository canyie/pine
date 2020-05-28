//
// Created by canyie on 2020/3/31.
//

#ifndef PINE_EXTRAS_H
#define PINE_EXTRAS_H

#include <cstdint>
#include "arch/arch.h"
#include "../utils/macros.h"

namespace pine {
    /** Hook extras data. DO NOT modify the member layout of this class because it is hard-coded in trampolines. */
    class PACKED(4) Extras final {
    public:
        Extras() {
        }

        void ReleaseLock() {
            CHECK(lock_flag == 0, "Unexpected lock_flag %d", lock_flag);

            dmb(); // Ensure all previous accesses are observed before the lock is released.
            lock_flag = 1;
            dsb(); // Ensure completion of the store that cleared the lock before sending the event.
            sev(); // Wake up the thread that is waiting for the lock.
        }

        /** Thread lock flag, 1: unlocked, 0: locked. */
        volatile uint32_t lock_flag = 1;
        /** r1(32 bit) or x1(64 bit) register */
        void* r1 = nullptr;
        /** r2(32 bit) or x2(64 bit) register */
        void* r2 = nullptr;
        /** r3(32 bit) or x3(64 bit) register */
        void* r3 = nullptr;
    };
}

#endif //PINE_EXTRAS_H

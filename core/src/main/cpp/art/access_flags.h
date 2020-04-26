//
// Created by canyie on 2020/3/19.
//

#ifndef PINE_ACCESS_FLAGS_H
#define PINE_ACCESS_FLAGS_H

#include <cstdint>
#include "../utils/macros.h"

namespace pine {
    class AccessFlags {
    public:
        static constexpr uint32_t kAccPublic = 0x0001;
        static constexpr uint32_t kAccPrivate = 0x0002;
        static constexpr uint32_t kAccProtected = 0x0004;
        static constexpr uint32_t kAccStatic = 0x0008;
        static constexpr uint32_t kAccFinal = 0x0010;
        static constexpr uint32_t kAccSynchronized = 0x0020;
        static constexpr uint32_t kAccNative = 0x0100;
        static constexpr uint32_t kAccConstructor = 0x00010000;
        static constexpr uint32_t kAccDeclaredSynchronized = 0x00020000;
        static constexpr uint32_t kAccSkipAccessChecks = 0x00080000;
        static constexpr uint32_t kAccMiranda = 0x00200000;
        static constexpr uint32_t kAccFastNative = 0x00080000;
        static constexpr uint32_t kAccCriticalNative = 0x00200000;
        static constexpr uint32_t kAccCompileDontBother_N = 0x01000000;
        static constexpr uint32_t kAccCompileDontBother_O_MR1 = 0x02000000;
        static constexpr uint32_t kAccPublicApi = 0x10000000;
        static constexpr uint32_t kAccCorePlatformApi = 0x20000000;
        static constexpr uint32_t kAccFastInterpreterToInterpreterInvoke = 0x40000000;
    private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(AccessFlags);
    };
}

#endif //PINE_ACCESS_FLAGS_H

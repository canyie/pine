//
// Created by canyie on 2020/3/15.
//

#ifndef PINE_ANDROID_H
#define PINE_ANDROID_H

#include <jni.h>
#include "utils/macros.h"
#include "utils/elf_img.h"

namespace pine {
    class Android final {
    public:
        static inline bool Is64Bit() {
            return sizeof(void*) == 8;
        }

        static void Init(JNIEnv* env, int sdk_version);

        static bool DisableProfileSaver();

        static int version;
        static JavaVM* jvm;

        static void (*suspend_vm)();

        static void (*resume_vm)();

        static constexpr int VERSION_K = 19;
        static constexpr int VERSION_L = 21;
        static constexpr int VERSION_L_MR1 = 22;
        static constexpr int VERSION_M = 23;
        static constexpr int VERSION_N = 24;
        static constexpr int VERSION_N_MR1 = 25;
        static constexpr int VERSION_O = 26;
        static constexpr int VERSION_O_MR1 = 27;
        static constexpr int VERSION_P = 28;
        static constexpr int VERSION_Q = 29;
        static constexpr int VERSION_R = 30;
    private:
        static void DisableHiddenApiPolicy(const ElfImg* handle);

        DISALLOW_IMPLICIT_CONSTRUCTORS(Android);
    };
}

#endif //PINE_ANDROID_H

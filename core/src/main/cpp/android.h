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

        static void Init(JNIEnv* env, int sdk_version, bool disable_hiddenapi_policy, bool disable_hiddenapi_policy_for_platform);
        static void DisableHiddenApiPolicy(bool application, bool platform) {
            ElfImg handle("libart.so");
            DisableHiddenApiPolicy(&handle, application, platform);
        }
        static bool DisableProfileSaver();

        static int version;
        static JavaVM* jvm;

        static void (*suspend_vm)();

        static void (*resume_vm)();

        static constexpr int kK = 19;
        static constexpr int kL = 21;
        static constexpr int kLMr1 = 22;
        static constexpr int kM = 23;
        static constexpr int kN = 24;
        static constexpr int kNMr1 = 25;
        static constexpr int kO = 26;
        static constexpr int kOMr1 = 27;
        static constexpr int kP = 28;
        static constexpr int kQ = 29;
        static constexpr int kR = 30;
    private:
        static void DisableHiddenApiPolicy(const ElfImg* handle, bool application, bool platform);
        static void DisableInterpreterForHookedMethods(const ElfImg* handle);

        DISALLOW_IMPLICIT_CONSTRUCTORS(Android);
    };
}

#endif //PINE_ANDROID_H

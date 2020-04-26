//
// Created by canyie on 2020/3/15.
//

#ifndef PINE_ANDROID_H
#define PINE_ANDROID_H

#include <jni.h>
#include "utils/macros.h"

namespace pine {
    class Android {
    public:
        static inline bool Is64Bit() {
            return sizeof(void *) == 8;
        }

        static void Init(JNIEnv *env, int sdk_version);

        static int version;
        static JavaVM *jvm;

        static void (*suspend_vm)();
        static void (*resume_vm)();

        static const int VERSION_K = 19;
        static const int VERSION_L = 21;
        static const int VERSION_L_MR1 = 22;
        static const int VERSION_M = 23;
        static const int VERSION_N = 24;
        static const int VERSION_N_MR1 = 25;
        static const int VERSION_O = 26;
        static const int VERSION_O_MR1 = 27;
        static const int VERSION_P = 28;
        static const int VERSION_Q = 29;
        static const int VERSION_R = 30;
    private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(Android);
    };
}

#endif //PINE_ANDROID_H

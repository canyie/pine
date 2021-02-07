//
// Created by canyie on 2020/3/15.
//

#ifndef PINE_ANDROID_H
#define PINE_ANDROID_H

#include <jni.h>
#include "utils/log.h"
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
        static void SetClassLinker(void* class_linker) {
            LOGI("Got class linker %p", class_linker);
            class_linker_ = class_linker;
        }
        static void* GetClassLinker() {
            return class_linker_;
        }

        static void MakeInitializedClassesVisiblyInitialized(void* thread, bool wait) {
            // If symbol MakeInitializedClassesVisiblyInitialized not found,
            // class_linker_ won't be initialized.
            if (UNLIKELY(!class_linker_)) {
                LOGE("No ClassLinker, skip MakeInitializedClassesVisiblyInitialized.");
                return;
            }
            make_visibly_initialized_(class_linker_, thread, wait);
        }

        static int version;
        static JavaVM* jvm;

        static void SuspendVM(void* cookie, const char* cause) {
            if (suspend_vm) {
                suspend_vm();
            } else if (suspend_all) {
                suspend_all(cookie, cause, false);
            }
        }

        static void ResumeVM(void* cookie) {
            if (resume_vm) {
                resume_vm();
            } else if (resume_all) {
                resume_all(cookie);
            }
        }

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
        static void HookClassLinkerForR(const ElfImg* handle);
        static void DisableInterpreterForHookedMethods(const ElfImg* handle);

        static void (*suspend_vm)();
        static void (*resume_vm)();
        static void (*suspend_all)(void*, const char*, bool);
        static void (*resume_all)(void*);

        static void* class_linker_;
        static void (*make_visibly_initialized_)(void*, void*, bool);
        DISALLOW_IMPLICIT_CONSTRUCTORS(Android);
    };
}

#endif //PINE_ANDROID_H

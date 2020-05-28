//
// Created by canyie on 2020/3/11.
//

#ifndef PINE_THREAD_H
#define PINE_THREAD_H

#include <pthread.h>
#include "../android.h"
#include "../utils/log.h"
#include "../utils/macros.h"
#include "../utils/elf_img.h"

#if defined(__aarch64__)
#define __get_tls() ({ void** __val; __asm__("mrs %0, tpidr_el0" : "=r"(__val)); __val; })
#elif defined(__arm__)
#define __get_tls() ({ void** __val; __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(__val)); __val; })
#else
#error unsupported architecture
#endif

namespace pine::art {
    class Thread final {
    public:
        static void Init(const ElfImg* handle);

        static inline Thread* Current() {
            Thread* thread;
            if (Android::version >= Android::VERSION_N) {
                thread = reinterpret_cast<Thread*>(__get_tls()[7/*TLS_SLOT_ART_THREAD_SELF*/]);
            } else if (current) {
                thread = current();
            } else if (key_self) {
                thread = static_cast<Thread*>(pthread_getspecific(*key_self));
            } else {
                // This function only called when Thread.nativePeer is unavailable.
                LOGE("Unable to get art::Thread by any means... this's crazy!");
                thread = nullptr;
            }
            return thread;
        }

        inline int32_t GetStateAndFlags() {
            return *GetStateAndFlagsPtr();
        }

        inline void SetStateAndFlags(int32_t state_and_flags) {
            *GetStateAndFlagsPtr() = state_and_flags;
        }

        jobject AddLocalRef(JNIEnv* env, void* o) {
            if (LIKELY(new_local_ref)) {
                return new_local_ref(env, o);
            }
            jweak global_weak_ref = add_weak_global_ref(Android::jvm, this, o);
            jobject local_ref = env->NewLocalRef(global_weak_ref);
            env->DeleteWeakGlobalRef(global_weak_ref);
            return local_ref;
        }

        void* DecodeJObject(jobject o) {
            return decode_jobject(this, o);
        }

    private:
        inline int32_t* GetStateAndFlagsPtr() {
            // class Thread {
            //  struct PACKED(4) tls_32bit_sized_values {
            //    union StateAndFlags state_and_flags (32 bit)
            //    ...
            //  }
            //  ...
            // }
            return reinterpret_cast<int32_t*>(this);
        }

        static Thread* (*current)();

        static pthread_key_t* key_self;

        static jobject (*new_local_ref)(JNIEnv*, void*);

        static jweak (*add_weak_global_ref)(JavaVM*, Thread*, void*);

        static void* (*decode_jobject)(Thread*, jobject);

        DISALLOW_IMPLICIT_CONSTRUCTORS(Thread);
    };

    class ScopedSuspendVM {
    public:
        ScopedSuspendVM() {
            if (Android::suspend_vm && Android::resume_vm) {
                Android::suspend_vm();
            } else {
                LOGW("Skip suspend VM: Suspend VM API is unavailable.");
            }
        }

        ~ScopedSuspendVM() {
            if (Android::suspend_vm && Android::resume_vm) {
                Android::resume_vm();
            }
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(ScopedSuspendVM);
    };

}

#endif //PINE_THREAD_H

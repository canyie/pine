//
// Created by canyie on 2020/3/11.
//

#ifndef PINE_THREAD_H
#define PINE_THREAD_H

#include <pthread.h>
#include <cstdlib>
#include "object.h"
#include "../android.h"
#include "../utils/log.h"
#include "../utils/macros.h"
#include "../utils/elf_img.h"

#if defined(__aarch64__)
#define __get_tls() ({ void** __val; __asm__("mrs %0, tpidr_el0" : "=r"(__val)); __val; })
#elif defined(__arm__)
#define __get_tls() ({ void** __val; __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(__val)); __val; })
#elif defined(__i386__)
#define __get_tls() ({ void** __val; __asm__("movl %%gs:0, %0" : "=r"(__val)); __val; })
#else
#error unsupported architecture
#endif

namespace pine::art {
    class Thread final {
    public:
        static void Init(const ElfImg* handle);

        static inline Thread* Current() {
            Thread* thread;
            if (Android::version >= Android::kN) {
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

        jobject AddLocalRef(JNIEnv* env, Object* obj) {
            if (UNLIKELY(obj->IsForwardingAddress())) {
                // Bug #3: Invalid state during hashcode ForwardingAddress
                // Caused by gc moved the object?
                // The object has moved to new address, forwarding to it.
                Object* forwarding = obj->GetForwardingAddress();
                LOGW("Detected forwarding address object (origin %p, monitor %u, forwarding to %p)",
                        obj, obj->GetMonitor(), forwarding);
                CHECK(forwarding != nullptr, "Forwarding to nullptr");
                // FIXME: Will this check fail under normal circumstances?
                CHECK_EQ(obj->GetClass(), forwarding->GetClass(),
                        "Forwarding object type mismatch (origin %p, forwarding %p)", obj->GetClass(), forwarding->GetClass());
                obj = forwarding;
            }
            if (LIKELY(new_local_ref)) {
                return new_local_ref(env, obj);
            }
            jweak global_weak_ref = add_weak_global_ref(Android::jvm, this, obj);
            jobject local_ref = env->NewLocalRef(global_weak_ref);
            env->DeleteWeakGlobalRef(global_weak_ref);
            return local_ref;
        }

        void* DecodeJObject(jobject o) {
            return decode_jobject(this, o);
        }

        void* AllocNonMovable(jclass cls) {
            if (LIKELY(alloc_non_movable)) {
                void* real_cls = DecodeJObject(cls);
                return alloc_non_movable(real_cls, this);
            }
            return nullptr;
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

        static void* (*alloc_non_movable)(void*, Thread*);

        DISALLOW_IMPLICIT_CONSTRUCTORS(Thread);
    };

    class ScopedSuspendVM {
    public:
        ScopedSuspendVM() {
            Android::SuspendVM(this, "pine hook method");
        }

        ~ScopedSuspendVM() {
            Android::ResumeVM(this);
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(ScopedSuspendVM);
    };

}

#endif //PINE_THREAD_H

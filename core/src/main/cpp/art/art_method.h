//
// Created by canyie on 2020/2/9.
//

#ifndef PINE_ART_METHOD_H
#define PINE_ART_METHOD_H

#include <cstdlib>
#include <jni.h>
#include <atomic>
#include "access_flags.h"
#include "../utils/macros.h"
#include "../utils/elf_img.h"
#include "../utils/member.h"
#include "../android.h"
#include "../utils/log.h"
#include "jit.h"

namespace pine::art {
    class ArtMethod final {
    public:
        static void Init(ElfImg *handle);

        static void InitMembers(ArtMethod *m1, ArtMethod *m2, uint32_t access_flags);

        static ArtMethod *FromReflectedMethod(JNIEnv *env, jobject javaMethod) {
            return FromMethodID(env->FromReflectedMethod(javaMethod));
        }

        static ArtMethod *FromMethodID(jmethodID method) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
            return reinterpret_cast<ArtMethod *> (method);
#pragma clang diagnostic pop
        }

        static ArtMethod *New() {
            return static_cast<ArtMethod *>(malloc(size));
        }

        jmethodID ToMethodID() {
            return reinterpret_cast<jmethodID> (this);
        }

        // Only work on android 7.0+
        uint32_t GetDeclaringClass() {
            return declaring_class->Get(this);
        }

        // Only work on android 7.0+
        void SetDeclaringClass(uint32_t new_declaring_class) {
            declaring_class->Set(this, new_declaring_class);
        }

        bool IsCompiled() {
            return GetEntryPointFromCompiledCode() != GetInterpreterBridge();
        }

        bool Compile(Thread *thread) {
            if (LIKELY(IsCompiled())) return true;
            if (UNLIKELY(Android::version < Android::VERSION_N)) return false;
            if (UNLIKELY(HasAccessFlags(kAccCompileDontBother))) return false;
            return Jit::CompileMethod(thread, this);
        }

        bool CompileOrSetNonCompilable(Thread *thread) {
            if (Android::version < Android::VERSION_N) return IsCompiled();
            if (LIKELY(Compile(thread))) return true;
            SetNonCompilable();
            return false;
        }

        bool Decompile(bool disableJit) {
            void *interpreter_bridge = GetInterpreterBridge();
            if (LIKELY(interpreter_bridge)) {
                if (disableJit) {
                    SetNonCompilable();
                }

                SetEntryPointFromCompiledCode(interpreter_bridge);
                // TODO When Android version is lower than 7.0(24), set entry_point_from_interpreter_
                return true;
            } else {
                LOGE("Failed to decompile method: interpreter bridge not found");
                return false;
            }
        }

        void SetNonCompilable() {
            if (Android::version < Android::VERSION_N) return;
            AddAccessFlags(kAccCompileDontBother);
        }

        void SetFastNative() {
            assert(IsNative());
            AddAccessFlags(AccessFlags::kAccFastNative);
        }

        bool IsStatic() {
            return HasAccessFlags(AccessFlags::kAccStatic);
        }

        bool IsNative() {
            return HasAccessFlags(AccessFlags::kAccNative);
        }

        uint32_t GetAccessFlags() {
            return access_flags_.Get(this);
        }

        bool HasAccessFlags(uint32_t flags) {
            return (GetAccessFlags() & flags) == flags;
        }

        void SetAccessFlags(uint32_t new_access_flags) {
            access_flags_.Set(this, new_access_flags);
        }

        void AddAccessFlags(uint32_t flags) {
            SetAccessFlags(GetAccessFlags() | flags);
        }

        void RemoveAccessFlags(uint32_t flags) {
            SetAccessFlags(GetAccessFlags() & ~flags);
        }

        void *GetEntryPointFromCompiledCode() {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_compiled_code_ is a uint64_t
                return reinterpret_cast<void *> (entry_point_from_compiled_code_.GetAs<uint64_t>(this));
            }
            return entry_point_from_compiled_code_.Get(this);
        }

        void SetEntryPointFromCompiledCode(void *entry) {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_compiled_code_ is a uint64_t
                entry_point_from_compiled_code_.SetAs<uint64_t>(this,
                                                                 reinterpret_cast<uint64_t>(entry));
                return;
            }
            entry_point_from_compiled_code_.Set(this, entry);
        }

        bool IsThumb() {
            return (reinterpret_cast<uintptr_t> (GetEntryPointFromCompiledCode()) & 1) == 1;
        }

        void *GetCompiledCodeAddr() {
            void *addr = GetEntryPointFromCompiledCode();
#ifdef __arm__
            addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t> (addr) & ~1);
#endif
        return addr;
        }

        uint32_t GetCompiledCodeSize() {
            //  class OatQuickMethodHeader {
            //    ...
            //    uint32_t code_size_ = 0u;
            //    uint8_t code_[0];
            //  }
            uint32_t code_size = *reinterpret_cast<uint32_t *>(
                    reinterpret_cast<size_t> (GetCompiledCodeAddr()) - sizeof(uint32_t));
            if (Android::version >= Android::VERSION_O) {
                // On Android 8+, The highest bit is used to signify if the compiled
                // code with the method header has should_deoptimize flag.
                uint32_t kShouldDeoptimizeMask = 0x80000000;
                code_size &= ~kShouldDeoptimizeMask;
            }
            return code_size;
        }

        void BackupFrom(ArtMethod *source, void *entry);

        void AfterHook(bool is_inline_hook, bool debuggable);

    private:
        static int32_t GetDefaultAccessFlagsOffset() {
            switch (Android::version) {
                case Android::VERSION_Q :
                case Android::VERSION_P :
                case Android::VERSION_O_MR1 :
                case Android::VERSION_O :
                case Android::VERSION_N_MR1 :
                case Android::VERSION_N :
                    return 4;
                case Android::VERSION_M :
                    return 12;
                case Android::VERSION_L_MR1 :
                    return 20;
                case Android::VERSION_L :
                    return 56;
                default:
                    // Android Kitkat doesn't use this function.
                    FATAL("Unexpected android version %d", Android::version);
            }
        }

        static int32_t GetDefaultEntryPointFromQuickCompiledCodeOffset() {
            switch (Android::version) {
                case Android::VERSION_Q :
                case Android::VERSION_P :
                    return Android::Is64Bit() ? 32 : 24;
                case Android::VERSION_O_MR1 :
                case Android::VERSION_O :
                    return Android::Is64Bit() ? 40 : 28;
                case Android::VERSION_N_MR1 :
                case Android::VERSION_N :
                    return Android::Is64Bit() ? 48 : 32;
                case Android::VERSION_M :
                    return Android::Is64Bit() ? 48 : 36;
                case Android::VERSION_L_MR1 :
                    return Android::Is64Bit() ? 52 : 44;
                case Android::VERSION_L :
                    return 40;
                default:
                    // Android Kitkat doesn't use this function.
                    FATAL("Unexpected android version %d", Android::version);
            }
        }

        void *GetInterpreterBridge() {
            return UNLIKELY(IsNative()) ? art_quick_generic_jni_trampoline
                                        : art_quick_to_interpreter_bridge;
        }

        static uint32_t kAccCompileDontBother;

        static size_t size;
        static void *art_quick_to_interpreter_bridge;
        static void *art_quick_generic_jni_trampoline;
        static void (*copy_from)(ArtMethod *, ArtMethod *, size_t);
        static Member<ArtMethod, uint32_t> access_flags_;
        static Member<ArtMethod, void *> entry_point_from_compiled_code_;
        static Member<ArtMethod, uint32_t> *declaring_class; // GCRoot is uint32_t
        DISALLOW_IMPLICIT_CONSTRUCTORS(ArtMethod);
    };
}

#endif //PINE_ART_METHOD_H

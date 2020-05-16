//
// Created by canyie on 2020/2/9.
//

#ifndef PINE_ART_METHOD_H
#define PINE_ART_METHOD_H

#include <cstdlib>
#include <jni.h>
#include <atomic>
#include "access_flags.h"
#include "../android.h"
#include "../utils/macros.h"
#include "../utils/elf_img.h"
#include "../utils/member.h"
#include "../utils/log.h"
#include "../utils/well_known_classes.h"
#include "jit.h"

namespace pine::art {
    class ArtMethod final {
    public:
        static void Init(const ElfImg *handle);

        static void InitMembers(ArtMethod *m1, ArtMethod *m2, uint32_t access_flags);

        static ArtMethod *FromReflectedMethod(JNIEnv *env, jobject javaMethod);

        static ArtMethod *Require(JNIEnv *env, jclass c, const char *name,
                const char *signature, bool is_static);

        static ArtMethod *GetArtMethodForR(JNIEnv *env, jobject javaMethod) {
            // We assume that jmethodID is the real ArtMethod pointer, which is no longer correct on Android R.
            // Fortunately, in Java, the Executable object has a member called artMethod,
            // and it still seems to hold the actual ArtMethod pointer.
            jlong artMethod = env->GetLongField(javaMethod,
                                                WellKnownClasses::java_lang_reflect_Executable_artMethod);
            return reinterpret_cast<ArtMethod *>(artMethod);
        }

        static ArtMethod *New() {
            return static_cast<ArtMethod *>(malloc(size));
        }

        static void *GetQuickToInterpreterBridge() {
            return art_quick_to_interpreter_bridge;
        }

        static void SetQuickToInterpreterBridge(void *entry) {
            art_quick_to_interpreter_bridge = entry;
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

        bool Decompile(bool disableJit) {
            void *interpreter_bridge = GetInterpreterBridge();
            if (LIKELY(interpreter_bridge)) {
                if (disableJit) {
                    SetNonCompilable();
                }

                SetEntryPointFromCompiledCode(interpreter_bridge);

                if (art_interpreter_to_interpreter_bridge) {
                    SetEntryPointFromInterpreter(art_interpreter_to_interpreter_bridge);
                }

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
            AddAccessFlags(AccessFlags::kFastNative);
        }

        bool IsStatic() {
            return HasAccessFlags(AccessFlags::kStatic);
        }

        bool IsNative() {
            return HasAccessFlags(AccessFlags::kNative);
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
                return reinterpret_cast<void *> (
                        entry_point_from_compiled_code_.GetAs<uint64_t>(this));
            }
            return entry_point_from_compiled_code_.Get(this);
        }

        void SetEntryPointFromCompiledCode(void *entry) {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_compiled_code_ is a uint64_t
                entry_point_from_compiled_code_.SetAs<uint64_t>(
                        this, reinterpret_cast<uint64_t>(entry));
                return;
            }
            entry_point_from_compiled_code_.Set(this, entry);
        }

        void *GetEntryPointFromJni() {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_jni_ is a uint64_t
                return reinterpret_cast<void *>(
                        entry_point_from_jni_.GetAs<uint64_t>(this));
            }
            return entry_point_from_jni_.Get(this);
        }

        void SetEntryPointFromJni(void *entry) {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_jni_ is a uint64_t
                entry_point_from_jni_.SetAs<uint64_t>(
                        this, reinterpret_cast<uint64_t>(entry));
                return;
            }
            entry_point_from_jni_.Set(this, entry);
        }

        void *GetEntryPointFromInterpreter() {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_interpreter_ is a uint64_t
                return reinterpret_cast<void *>(
                        entry_point_from_interpreter_->GetAs<uint64_t>(this));
            }
            return entry_point_from_interpreter_->Get(this);
        }

        void SetEntryPointFromInterpreter(void *entry) {
            if (Android::version == Android::VERSION_L) {
                // Android 5.0, entry_point_from_interpreter_ is a uint64_t
                entry_point_from_interpreter_->SetAs<uint64_t>(
                        this, reinterpret_cast<uint64_t>(entry));
                return;
            }
            entry_point_from_interpreter_->Set(this, entry);
        }

        bool IsThumb() {
#ifdef __aarch64__
            return false;
#else
            return (reinterpret_cast<uintptr_t>(GetEntryPointFromCompiledCode()) & 1) == 1;
#endif
        }

        void *GetCompiledCodeAddr() {
            void *addr = GetEntryPointFromCompiledCode();
#ifdef __arm__
            addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(addr) & ~1);
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
                    reinterpret_cast<uintptr_t>(GetCompiledCodeAddr()) - sizeof(uint32_t));
            if (Android::version >= Android::VERSION_O) {
                // On Android 8+, The highest bit is used to signify if the compiled
                // code with the method header has should_deoptimize flag.
                uint32_t kShouldDeoptimizeMask = 0x80000000;
                code_size &= ~kShouldDeoptimizeMask;
            }
            return code_size;
        }

        void BackupFrom(ArtMethod *source, void *entry, bool is_inline_hook, bool is_native_or_proxy);

        void AfterHook(bool is_inline_hook, bool debuggable, bool is_native_or_proxy);

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

        static int32_t GetDefaultEntryPointFromJniOffset() {
            switch (Android::version) {
                case Android::VERSION_Q :
                case Android::VERSION_P :
                    return Android::Is64Bit() ? 24 : 20;
                case Android::VERSION_O_MR1 :
                case Android::VERSION_O :
                    return Android::Is64Bit() ? 32 : 24;
                case Android::VERSION_N_MR1 :
                case Android::VERSION_N :
                    return Android::Is64Bit() ? 40 : 28;
                case Android::VERSION_L_MR1 :
                    return Android::Is64Bit() ? 44 : 40;
                case Android::VERSION_L :
                    return 32;
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
        static void *art_interpreter_to_interpreter_bridge;
        static void *art_interpreter_to_compiled_code_bridge;
        static void (*copy_from)(ArtMethod *, ArtMethod *, size_t);

        static Member<ArtMethod, uint32_t> access_flags_;
        static Member<ArtMethod, void *> entry_point_from_compiled_code_;

        // In Android 8.0+, it is actually called data_.
        static Member<ArtMethod, void *> entry_point_from_jni_;

        static Member<ArtMethod, void *> *entry_point_from_interpreter_;
        static Member<ArtMethod, uint32_t> *declaring_class; // GCRoot is uint32_t
        DISALLOW_IMPLICIT_CONSTRUCTORS(ArtMethod);
    };
}

#endif //PINE_ART_METHOD_H

//
// Created by canyie on 2020/3/15.
//

#ifndef PINE_ANDROID_H
#define PINE_ANDROID_H

#include <vector>
#include <jni.h>
#include <android/api-level.h>
#include "art/gc_defs.h"
#include "utils/log.h"
#include "utils/macros.h"
#include "utils/elf_image.h"

namespace pine {
    class ScopedGCCriticalSection {
    public:
        ALWAYS_INLINE ScopedGCCriticalSection(void* self, art::GcCause cause, art::CollectorType collector);
        ALWAYS_INLINE ~ScopedGCCriticalSection();
    private:
        [[maybe_unused]] art::GCCriticalSection critical_section_;
        [[maybe_unused]] const char* old_no_suspend_reason_;
    };

    class Android final {
    public:
        static inline constexpr bool Is64Bit() {
            return sizeof(void*) == 8;
        }

        static void Init(JNIEnv* env, int sdk_version, bool disable_hiddenapi_policy, bool disable_hiddenapi_policy_for_platform);
        static void DisableHiddenApiPolicy(bool application, bool platform) {
            ElfImage handle("libart.so");
            DisableHiddenApiPolicy(&handle, application, platform);
        }
        static bool DisableProfileSaver();
        static void SetClassLinker(void* class_linker) {
            class_linker_ = class_linker;
        }
        static void* GetClassLinker() {
            return class_linker_;
        }

        static void MakeInitializedClassesVisiblyInitialized(void* thread, bool wait) {
            // If symbol MakeInitializedClassesVisiblyInitialized not found,
            // class_linker_ won't be initialized.
            if (!class_linker_) {
                return;
            }
            make_visibly_initialized_(class_linker_, thread, wait);
        }

        static bool MoveJitInfo(void* from, void* to) {
            if (LIKELY(jit_code_cache_ && move_obsolete_method_)) {
                move_obsolete_method_(jit_code_cache_, from, to);
                return true;
            }
            return false;
        }

        static int version;
        static JavaVM* jvm_;

        static void StartGCCriticalSection(void* cookie, void* self, art::GcCause cause, art::CollectorType collector) {
            if (start_gc_critical_section) {
                start_gc_critical_section(cookie, self, cause, collector);
            }
        }

        static void EndGCCriticalSection(void* cookie) {
            if (end_gc_critical_section) {
                end_gc_critical_section(cookie);
            }
        }

        static void SuspendVM(void* cookie, void* self, const char* cause) {
            if (suspend_vm) {
                suspend_vm();
            } else if (suspend_all) {
                // Avoid a deadlock between GC and debugger where GC gets suspended during GC. b/25800335.
                ScopedGCCriticalSection gcs(self, art::GcCause::kGcCauseDebugger, art::CollectorType::kCollectorTypeDebugger);
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
        static constexpr int kS = 31;
        static constexpr int kSL = 32;
        static constexpr int kT = 33;
        static constexpr int kU = 34;
        static constexpr int kV = 35;
    private:
        static void DisableHiddenApiPolicy(const ElfImage* handle, bool application, bool platform);
        static void InitMembersFromRuntime(JavaVM* jvm, const ElfImage* handle);
        static void InitClassLinker(void* runtime, size_t java_vm_offset, const ElfImage* handle, bool has_small_irt);
        static void InitJitCodeCache(void* runtime, size_t java_vm_offset, const ElfImage* handle);

        static std::vector<size_t> OffsetOfJavaVm(bool has_small_irt) {
            std::vector<size_t> offsets;
            // This function will only be called on Android 10+ where ART is always an apex module.
            // Since APEX module can be upgraded through Google Play update without the need to
            // update Android major version, hardcode offset will be meaningless on old Android
            // major versions with new ART. We list all offsets we known.
            if (LIKELY(has_small_irt)) {
                offsets.emplace_back(Is64Bit() ? 632 : 356); // ART 14, 15
                if (version < kU)
                    offsets.emplace_back(Is64Bit() ? 624 : 352); // ART 13
                if (UNLIKELY(version < kT))
                    offsets.emplace_back(Is64Bit() ? 528 : 304); // ART 12
            } else {
                offsets.emplace_back(Is64Bit() ? 520 : 300); // ART 12
                if (UNLIKELY(version < kS))
                    offsets.emplace_back(Is64Bit() ? 496 : 288); // ART 10-11
            }
            return offsets;
        }

        static void (*suspend_vm)();
        static void (*resume_vm)();
        static void (*suspend_all)(void*, const char*, bool);
        static void (*resume_all)(void*);
        static void (*start_gc_critical_section)(void*, void*, art::GcCause, art::CollectorType);
        static void (*end_gc_critical_section)(void*);

        static void* class_linker_;
        static void (*make_visibly_initialized_)(void*, void*, bool);

        static void* jit_code_cache_;
        static void (*move_obsolete_method_)(void*, void*, void*);
        DISALLOW_IMPLICIT_CONSTRUCTORS(Android);
    };

    class ScopedSuspendVM {
    public:
        ScopedSuspendVM(void* self) {
            Android::SuspendVM(this, self, "pine hook method");
        }

        ~ScopedSuspendVM() {
            Android::ResumeVM(this);
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(ScopedSuspendVM);
    };
}

#endif //PINE_ANDROID_H

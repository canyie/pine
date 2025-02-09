//
// Created by canyie on 2020/3/15.
//

#include <unistd.h>
#include <string>
#include <dlfcn.h>
#include <mutex>
#include "android.h"
#include "utils/well_known_classes.h"
#include "art/art_method.h"
#include "art/jit.h"
#include "trampoline/trampoline_installer.h"
#include "utils/memory.h"

using namespace pine;

int Android::version = -1;
JavaVM* Android::jvm_ = nullptr;

void (*Android::suspend_vm)() = nullptr;
void (*Android::resume_vm)() = nullptr;

void (*Android::suspend_all)(void*, const char*, bool) = nullptr;
void (*Android::resume_all)(void*) = nullptr;
void (*Android::start_gc_critical_section)(void*, void*, art::GcCause, art::CollectorType);
void (*Android::end_gc_critical_section)(void*) = nullptr;

void* Android::class_linker_ = nullptr;
void (*Android::make_visibly_initialized_)(void*, void*, bool) = nullptr;

void* Android::jit_code_cache_ = nullptr;
void (*Android::move_obsolete_method_)(void*, void*, void*) = nullptr;

void Android::Init(JNIEnv* env, int sdk_version, bool disable_hiddenapi_policy, bool disable_hiddenapi_policy_for_platform) {
    Android::version = sdk_version;
    if (UNLIKELY(env->GetJavaVM(&jvm_) != JNI_OK)) {
        LOGF("Cannot get java vm");
        env->FatalError("Cannot get java vm");
        abort();
    }

    {
        bool eng_build = false;
        ElfImage art_lib_handle("libart.so");
        if (UNLIKELY(!art_lib_handle.IsOpened())) {
            // Running on eng build ROMs?
            art_lib_handle.RelativeOpen("libartd.so", true, true);
            if (LIKELY(art_lib_handle.IsOpened())) {
                eng_build = true;
            } else {
                // Alibaba YunOS AOC runtime?
                constexpr const char* kLibAocPath = "/system/lib"
#ifdef __LP64__
                                                    "64"
#endif
                                                    "/libaoc.so";
                if (access(kLibAocPath, R_OK) == 0) {
                    art_lib_handle.Open(kLibAocPath, true, true);
                }
            }
        }

        if (Android::version >= Android::kR) {
            suspend_all = reinterpret_cast<void (*)(void*, const char*, bool)>(art_lib_handle.GetSymbolAddress(
                    "_ZN3art16ScopedSuspendAllC1EPKcb"));
            resume_all = reinterpret_cast<void (*)(void*)>(art_lib_handle.GetSymbolAddress(
                    "_ZN3art16ScopedSuspendAllD1Ev"));
            if (UNLIKELY(!suspend_all || !resume_all)) {
                LOGE("SuspendAll API is unavailable.");
                suspend_all = nullptr;
                resume_all = nullptr;
            } else {
                start_gc_critical_section = reinterpret_cast<void (*)(void*, void*, art::GcCause,
                        art::CollectorType)>(art_lib_handle.GetSymbolAddress(
                        "_ZN3art2gc23ScopedGCCriticalSectionC2EPNS_6ThreadENS0_7GcCauseENS0_13CollectorTypeE"));
                end_gc_critical_section = reinterpret_cast<void (*)(void*)>(art_lib_handle.GetSymbolAddress(
                        "_ZN3art2gc23ScopedGCCriticalSectionD2Ev"));
                if (UNLIKELY(!start_gc_critical_section || !end_gc_critical_section)) {
                    LOGE("GC critical section API is unavailable.");
                    start_gc_critical_section = nullptr;
                    end_gc_critical_section = nullptr;
                }
            }
        } else {
            suspend_vm = reinterpret_cast<void (*)()>(art_lib_handle.GetSymbolAddress(
                    "_ZN3art3Dbg9SuspendVMEv")); // art::Dbg::SuspendVM()
            resume_vm = reinterpret_cast<void (*)()>(art_lib_handle.GetSymbolAddress(
                    "_ZN3art3Dbg8ResumeVMEv")); // art::Dbg::ResumeVM()
            if (UNLIKELY(!suspend_vm || !resume_vm)) {
                LOGE("Suspend VM API is unavailable.");
                suspend_vm = nullptr;
                resume_vm = nullptr;
            }
        }

        if (Android::version >= Android::kP)
            DisableHiddenApiPolicy(&art_lib_handle, disable_hiddenapi_policy, disable_hiddenapi_policy_for_platform);

        art::Thread::Init(&art_lib_handle);
        art::ArtMethod::Init(&art_lib_handle);
        // JIT API is not supported yet in Android R+
        if (UNLIKELY(sdk_version >= kN && sdk_version < kR)) {
            ElfImage jit_lib_handle(eng_build ? "libartd-compiler.so" : "libart-compiler.so", true, false);
            art::Jit::Init(&art_lib_handle, &jit_lib_handle);
        }

        InitMembersFromRuntime(jvm_, &art_lib_handle);
    }

    WellKnownClasses::Init(env);
}

static int FakeHandleHiddenApi() {
    return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-macro-usage"

void Android::DisableHiddenApiPolicy(const ElfImage* handle, bool application, bool platform) {
    auto trampoline_installer = TrampolineInstaller::GetDefault();
    void* replace = reinterpret_cast<void*>(FakeHandleHiddenApi);
    bool failed = false;

#define HOOK_SYMBOL(symbol, warn_if_missing) do { \
void *target = handle->GetSymbolAddress(symbol, warn_if_missing); \
if (LIKELY(target))  \
    trampoline_installer->NativeHookNoBackup(target, replace); \
else  \
    failed = true; \
} while(false)

    if (Android::version >= Android::kQ) {
        if (LIKELY(application)) {
            // Android Q, for Domain::kApplication
            HOOK_SYMBOL("_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_8ArtFieldEEEbPT_NS0_7ApiListENS0_12AccessMethodE", false);
            HOOK_SYMBOL("_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_9ArtMethodEEEbPT_NS0_7ApiListENS0_12AccessMethodE", false);
        }

        if (LIKELY(platform)) {
            // For Domain::kPlatform
            HOOK_SYMBOL("_ZN3art9hiddenapi6detail30HandleCorePlatformApiViolationINS_8ArtFieldEEEbPT_RKNS0_13AccessContextENS0_12AccessMethodENS0_17EnforcementPolicyE", false);
            HOOK_SYMBOL("_ZN3art9hiddenapi6detail30HandleCorePlatformApiViolationINS_9ArtMethodEEEbPT_RKNS0_13AccessContextENS0_12AccessMethodENS0_17EnforcementPolicyE", false);
        }

        if (UNLIKELY(failed)) {
            // These functions are inlined for arm32 on Android 15, but not for arm64
            // If any symbol cannot be found, fallback to hook ShouldDenyAccessToMember
            // The flag will only be set if we need the feature, so we don't need to check it

            HOOK_SYMBOL("_ZN3art9hiddenapi24ShouldDenyAccessToMemberINS_8ArtFieldEEEbPT_RKNSt3__18functionIFNS0_13AccessContextEvEEENS0_12AccessMethodE", true);
            HOOK_SYMBOL("_ZN3art9hiddenapi24ShouldDenyAccessToMemberINS_9ArtMethodEEEbPT_RKNSt3__18functionIFNS0_13AccessContextEvEEENS0_12AccessMethodE", true);
        }
    } else {
        // Android P, all accesses from platform domain will be allowed
        if (LIKELY(application)) {
            HOOK_SYMBOL("_ZN3art9hiddenapi6detail19GetMemberActionImplINS_8ArtFieldEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE", true);
            HOOK_SYMBOL("_ZN3art9hiddenapi6detail19GetMemberActionImplINS_9ArtMethodEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE", true);
        }
    }

#undef HOOK_SYMBOL
}

#pragma clang diagnostic pop

static bool FakeProcessProfilingInfo() {
    LOGI("Skipped ProcessProfilingInfo.");
    return true;
}

bool Android::DisableProfileSaver() {
    // I think most users don't need this feature, so I don't get the symbol during initialization...
    void* process_profiling_info;
    {
        ElfImage handle("libart.so");

        // MIUI added, size of the original function is smaller than size of a direct jump trampoline
        // and cannot be hooked, else we will write overflow and corrupt the next function
        // https://github.com/canyie/pine/issues/18
        process_profiling_info = handle.GetSymbolAddress("_ZN3art12ProfileSaver20ProcessProfilingInfoEbPtb", false);
        if (LIKELY(!process_profiling_info)) {
            const char* symbol = version < kO ? "_ZN3art12ProfileSaver20ProcessProfilingInfoEPt"
                                              : version < kS ? "_ZN3art12ProfileSaver20ProcessProfilingInfoEbPt"
                                              : "_ZN3art12ProfileSaver20ProcessProfilingInfoEbbPt";
            process_profiling_info = handle.GetSymbolAddress(symbol);

            // Android 15 QPR1
            if (!process_profiling_info) {
                process_profiling_info = handle.GetSymbolAddress("_ZN3art12ProfileSaver20ProcessProfilingInfoEbPt");
            }
        }
    }

    if (UNLIKELY(!process_profiling_info)) {
        LOGE("Failed to disable ProfileSaver: art::ProfileSaver::ProcessProfilingInfo not found");
        return false;
    }

    TrampolineInstaller::GetDefault()->NativeHookNoBackup(process_profiling_info,
            reinterpret_cast<void*>(FakeProcessProfilingInfo));
    return true;
}

void Android::InitMembersFromRuntime(JavaVM* jvm, const ElfImage* handle) {
    if (version < kQ) {
        // ClassLinker is unnecessary before R.
        // JIT was added in Android N but MoveObsoleteMethod was added in Android O
        // and I didn't find a stable way to retrieve jit code cache until Q
        // from Runtime object, so try to retrieve from ProfileSaver.
        // TODO: Still clearing jit info on Android N but only for jit-compiled methods.
        if (version >= kO) {
            InitJitCodeCache(nullptr, 0, handle);
        }
        return;
    }
    void** instance_ptr = static_cast<void**>(handle->GetSymbolAddress("_ZN3art7Runtime9instance_E"));
    void* runtime;
    if (UNLIKELY(!instance_ptr || !(runtime = *instance_ptr))) {
        LOGE("Unable to retrieve Runtime.");
        return;
    }

    // If SmallIrtAllocator symbols can be found, then the ROM has merged commit "Initially allocate smaller local IRT"
    // This commit added a pointer member between `class_linker_` and `java_vm_`. Need to calibrate offset here.
    // https://android.googlesource.com/platform/art/+/4dcac3629ea5925e47b522073f3c49420e998911
    // https://github.com/crdroidandroid/android_art/commit/aa7999027fa830d0419c9518ab56ceb7fcf6f7f1
    // https://android.googlesource.com/platform/art/+/849d09a81907f16d8ccc6019b8baf86a304b730c
    bool has_smaller_irt = version >= kT
            || handle->HasSymbol("_ZN3art17SmallIrtAllocator10DeallocateEPNS_8IrtEntryE")
            || handle->HasSymbol("_ZN3art3jni17SmallLrtAllocatorC2Ev");

    std::vector<size_t> known_offsets = OffsetOfJavaVm(has_smaller_irt);
    size_t jvm_offset = 0;
    for (size_t offset : known_offsets) {
        auto val = reinterpret_cast<std::unique_ptr<JavaVM>*>(
                reinterpret_cast<uintptr_t>(runtime) + offset)->get();
        if (val == jvm) {
            jvm_offset = offset;
            break;
        }
    }
    if (UNLIKELY(!jvm_offset)) {
        LOGW("JavaVM offset mismatches default offsets, trying a linear search");
        int offset = Memory::FindOffset(runtime, jvm, 1024, 4);
        if (UNLIKELY(offset == -1)) {
            LOGE("Failed to find java vm from Runtime");
            return;
        }
        jvm_offset = offset;
        LOGW("Found JavaVM in Runtime at %zu", jvm_offset);
    }
    InitClassLinker(runtime, jvm_offset, handle, has_smaller_irt);
    InitJitCodeCache(runtime, jvm_offset, handle);
}

void Android::InitClassLinker(void* runtime, size_t java_vm_offset, const ElfImage* handle, bool has_small_irt) {
    // ClassStatus::kVisiblyInitialized is not implemented in official Android Q
    // but some weird ROMs cherry-pick this commit to these Q ROMs
    // https://github.com/crdroidandroid/android_art/commit/ef76ced9d2856ac988377ad99288a357697c4fa2
    if (version < kQ) return;
    bool required = version > kQ;
    make_visibly_initialized_ = reinterpret_cast<void (*)(void*, void*, bool)>(handle->GetSymbolAddress(
            "_ZN3art11ClassLinker40MakeInitializedClassesVisiblyInitializedEPNS_6ThreadEb", required));
    if (!make_visibly_initialized_) {
        if (UNLIKELY(required)) LOGE("ClassLinker::MakeInitializedClassesVisiblyInitialized not found");
        return;
    }

    const size_t kDifference = UNLIKELY(has_small_irt)
            ? sizeof(std::unique_ptr<void>) + sizeof(void*) * 3
            : UNLIKELY(version == kQ)
            ? sizeof(void*) * 2
            : sizeof(std::unique_ptr<void>) + sizeof(void*) * 2;

    void* class_linker = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(runtime) + java_vm_offset - kDifference);
    SetClassLinker(class_linker);
}

void Android::InitJitCodeCache(void *runtime, size_t java_vm_offset, const ElfImage *handle) {
    move_obsolete_method_ = reinterpret_cast<void (*)(void*, void*, void*)>(handle->GetSymbolAddress(
            "_ZN3art3jit12JitCodeCache18MoveObsoleteMethodEPNS_9ArtMethodES3_"));
    if (UNLIKELY(!move_obsolete_method_)) {
        LOGW("JitCodeCache::MoveObsoleteMethod not found. Fallback to clearing jit info.");
        return;
    }
    if (UNLIKELY(!runtime)) {
        // We are not safe to get jit code cache from Runtime... then try ProfileSaver.
        // ProfileSaver is not available before app starts, so we first try Runtime.

        // class ProfileSaver {
        //   static ProfileSaver* instance_;
        //   jit::JitCodeCache* jit_code_cache_;
        // }
        void*** symbol = reinterpret_cast<void***>(handle->GetSymbolAddress(
                "_ZN3art12ProfileSaver9instance_E"));
        if (UNLIKELY(symbol == nullptr)) {
            LOGW("ProfileSaver::instance_ not found. Fallback to clearing jit info.");
            return;
        }
        void** profile_saver = *symbol;
        if (UNLIKELY(profile_saver == nullptr)) {
            LOGW("ProfileSaver is not initialized, cannot get jit code cache. Fallback to clearing jit info.");
            return;
        }
        if (UNLIKELY((jit_code_cache_ = *profile_saver) == nullptr)) {
            LOGE("ProfileSaver is initialized but no jit code cache??? Fallback to clearing jit info.");
        }
        return;
    }
    constexpr size_t kDifference = sizeof(std::unique_ptr<void>) * 2;
    jit_code_cache_ = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(runtime) + java_vm_offset + kDifference);
}

ALWAYS_INLINE ScopedGCCriticalSection::ScopedGCCriticalSection(void* self, art::GcCause cause,
                                                               art::CollectorType collector) {
    Android::StartGCCriticalSection(this, self, cause, collector);
}

ALWAYS_INLINE ScopedGCCriticalSection::~ScopedGCCriticalSection() {
    Android::EndGCCriticalSection(this);
}

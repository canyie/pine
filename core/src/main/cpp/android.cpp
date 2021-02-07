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
#include "external/dobby.h"

using namespace pine;

int Android::version = -1;
JavaVM* Android::jvm = nullptr;

void (*Android::suspend_vm)() = nullptr;
void (*Android::resume_vm)() = nullptr;

void* Android::class_linker_ = nullptr;
void (*Android::make_visibly_initialized_)(void*, void*, bool) = nullptr;

static void* class_linker_target_ = nullptr;
static volatile void(*class_linker_backup_)(void*, void*) = nullptr;
static std::mutex class_linker_hook_mutex_;

static bool (*orig_ShouldUseInterpreterEntrypoint)(art::ArtMethod*, const void*);

static bool hook_ShouldUseInterpreterEntrypoint(art::ArtMethod* method, const void* quick_code) {
    if (UNLIKELY(method->IsHooked() && quick_code != nullptr)) return false;
    return orig_ShouldUseInterpreterEntrypoint(method, quick_code);
}

void Android::Init(JNIEnv* env, int sdk_version, bool disable_hiddenapi_policy, bool disable_hiddenapi_policy_for_platform) {
    Android::version = sdk_version;
    if (UNLIKELY(env->GetJavaVM(&jvm) != JNI_OK)) {
        LOGF("Cannot get java vm");
        env->FatalError("Cannot get java vm");
        abort();
    }

    {
        ElfImg art_lib_handle("libart.so");
        suspend_vm = reinterpret_cast<void (*)()>(art_lib_handle.GetSymbolAddress(
                "_ZN3art3Dbg9SuspendVMEv")); // art::Dbg::SuspendVM()
        resume_vm = reinterpret_cast<void (*)()>(art_lib_handle.GetSymbolAddress(
                "_ZN3art3Dbg8ResumeVMEv")); // art::Dbg::ResumeVM()

        if (Android::version >= Android::kP)
            DisableHiddenApiPolicy(&art_lib_handle, disable_hiddenapi_policy, disable_hiddenapi_policy_for_platform);

        art::Thread::Init(&art_lib_handle);
        art::ArtMethod::Init(&art_lib_handle);
        if (sdk_version >= kN) {
            ElfImg jit_lib_handle("libart-compiler.so", false);
            art::Jit::Init(&art_lib_handle, &jit_lib_handle);
        }

        if (UNLIKELY(sdk_version >= kR)) {
            HookClassLinkerForR(&art_lib_handle);
            if (UNLIKELY(PineConfig::debuggable)) {
                // We cannot set kAccNative for hooked methods because that may cause crash
                DisableInterpreterForHookedMethods(&art_lib_handle);
            }
        }
    }

    WellKnownClasses::Init(env);
}

static int FakeHandleHiddenApi() {
    return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-macro-usage"

void Android::DisableHiddenApiPolicy(const ElfImg* handle, bool application, bool platform) {
    TrampolineInstaller* trampoline_installer = TrampolineInstaller::GetDefault();
    void* replace = reinterpret_cast<void*>(FakeHandleHiddenApi);

#define HOOK(symbol) do { \
void *target = handle->GetSymbolAddress(symbol); \
if (LIKELY(target))  \
    trampoline_installer->NativeHookNoBackup(target, replace); \
else  \
    LOGE("DisableHiddenApiPolicy: symbol %s not found", symbol); \
} while(false)

    if (Android::version >= Android::kQ) {
        if (application) {
            // Android Q, for Domain::kApplication
            HOOK("_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_8ArtFieldEEEbPT_NS0_7ApiListENS0_12AccessMethodE");
            HOOK("_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_9ArtMethodEEEbPT_NS0_7ApiListENS0_12AccessMethodE");
        }

        if (platform) {
            // For Domain::kPlatform
            HOOK("_ZN3art9hiddenapi6detail30HandleCorePlatformApiViolationINS_8ArtFieldEEEbPT_RKNS0_13AccessContextENS0_12AccessMethodENS0_17EnforcementPolicyE");
            HOOK("_ZN3art9hiddenapi6detail30HandleCorePlatformApiViolationINS_9ArtMethodEEEbPT_RKNS0_13AccessContextENS0_12AccessMethodENS0_17EnforcementPolicyE");
        }
    } else {
        // Android P, all accesses from platform domain will be allowed
        if (application) {
            HOOK("_ZN3art9hiddenapi6detail19GetMemberActionImplINS_8ArtFieldEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE");
            HOOK("_ZN3art9hiddenapi6detail19GetMemberActionImplINS_9ArtMethodEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE");
        }
    }

#undef HOOK
}

#pragma clang diagnostic pop

static bool FakeProcessProfilingInfo() {
    LOGI("Skipped ProcessProfilingInfo.");
    return true;
}

bool Android::DisableProfileSaver() {
    // If the user needs this feature very much,
    // we may find these symbols during initialization in the future to reduce time consumption.
    void* process_profiling_info;
    {
        ElfImg handle("libart.so");
        const char* symbol = version < kO ? "_ZN3art12ProfileSaver20ProcessProfilingInfoEPt"
                                          : "_ZN3art12ProfileSaver20ProcessProfilingInfoEbPt";
        process_profiling_info = handle.GetSymbolAddress(symbol);
    }

    if (UNLIKELY(!process_profiling_info)) {
        LOGE("Failed to disable ProfileSaver: art::ProfileSaver::ProcessProfilingInfo not found");
        return false;
    }
    TrampolineInstaller::GetDefault()->NativeHookNoBackup(process_profiling_info,
            reinterpret_cast<void*>(FakeProcessProfilingInfo));
    return true;
}

static void replace_FixupStaticTrampolines(void* class_linker, void* clz) {
    {
        ScopedLock lock(class_linker_hook_mutex_);
        if (LIKELY(class_linker_backup_)) {
            Android::SetClassLinker(class_linker);
            class_linker_backup_(class_linker, clz);
            DobbyDestroy(class_linker_target_);
            class_linker_backup_ = nullptr;
            return;
        }
    }
    void(*origin)(void*, void*) = reinterpret_cast<void (*)(void*, void*)>(class_linker_target_);
    origin(class_linker, clz);
}

void Android::HookClassLinkerForR(const ElfImg* handle) {
    make_visibly_initialized_ = reinterpret_cast<void (*)(void*, void*, bool)>(handle->GetSymbolAddress(
            "_ZN3art11ClassLinker40MakeInitializedClassesVisiblyInitializedEPNS_6ThreadEb"));
    if (UNLIKELY(!make_visibly_initialized_)) {
        LOGE("ClassLinker::MakeInitializedClassesVisiblyInitialized not found");
        return;
    }

    // Hook class linker to get its instance
    class_linker_target_ = handle->GetSymbolAddress(
            "_ZN3art11ClassLinker22FixupStaticTrampolinesENS_6ObjPtrINS_6mirror5ClassEEE");
    if (UNLIKELY(!class_linker_target_)) {
        LOGE("ClassLinker::FixupStaticTrampolines not found.");
        return;
    }
    void* replace = reinterpret_cast<void*>(replace_FixupStaticTrampolines);
    void** backup = reinterpret_cast<void**>(&class_linker_backup_);
    DobbyHook(class_linker_target_, replace, backup);
}

void Android::DisableInterpreterForHookedMethods(const ElfImg* handle) {
    void* target = handle->GetSymbolAddress(
            "_ZN3art11ClassLinker30ShouldUseInterpreterEntrypointEPNS_9ArtMethodEPKv");
    void* replace = reinterpret_cast<void*>(hook_ShouldUseInterpreterEntrypoint);
    if (LIKELY(target)) {
        DobbyHook(target, replace, reinterpret_cast<void**>(&orig_ShouldUseInterpreterEntrypoint));
    } else {
        LOGE("Can't find ClassLinker::ShouldUseInterpreterEntrypoint. Hook may not work.");
    }
}

//
// Created by canyie on 2020/3/15.
//

#include <unistd.h>
#include <string>
#include "android.h"
#include "utils/well_known_classes.h"
#include "art/art_method.h"
#include "art/jit.h"
#include "trampoline/trampoline_installer.h"

using namespace pine;

int Android::version = -1;
JavaVM* Android::jvm = nullptr;

void (*Android::suspend_vm)() = nullptr;

void (*Android::resume_vm)() = nullptr;

static bool hook_ShouldUseInterpreterEntrypoint(art::ArtMethod* method, const void* quick_code) {
    if (UNLIKELY(method->IsNative() || method->IsProxy())) return false;
    if (!quick_code) return true;
    // We cannot call the original function now, and current code will cause crash
    return !method->IsHooked();
}

void Android::Init(JNIEnv* env, int sdk_version) {
    Android::version = sdk_version;
    if (UNLIKELY(env->GetJavaVM(&jvm) != JNI_OK)) {
        LOGF("Cannot get java vm");
        env->FatalError("Cannot get java vm");
        abort();
    }

    {
        ElfImg art_lib_handle("libart.so");
        suspend_vm = reinterpret_cast<void (*)()>(
                art_lib_handle.GetSymbolAddress(
                        "_ZN3art3Dbg9SuspendVMEv")); // art::Dbg::SuspendVM()
        resume_vm = reinterpret_cast<void (*)()>(
                art_lib_handle.GetSymbolAddress("_ZN3art3Dbg8ResumeVMEv")); // art::Dbg::ResumeVM()

        if (Android::version >= Android::kP)
            DisableHiddenApiPolicy(&art_lib_handle);

        art::Thread::Init(&art_lib_handle);
        art::ArtMethod::Init(&art_lib_handle);
        if (sdk_version >= kN) {
            ElfImg jit_lib_handle("libart-compiler.so", false);
            art::Jit::Init(&art_lib_handle, &jit_lib_handle);
        }

        if (UNLIKELY(PineConfig::debuggable && sdk_version >= kR)) {
            // We cannot set kAccNative for hooked methods because that will cause crash,
            // TODO hook ClassLinker::ShouldUseInterpreterEntrypoint

            /*void* should_use_interpreter = art_lib_handle.GetSymbolAddress(
                    "_ZN3art11ClassLinker30ShouldUseInterpreterEntrypointEPNS_9ArtMethodEPKv");
            if (LIKELY(should_use_interpreter)) {
                TrampolineInstaller::GetDefault()->NativeHookNoBackup(should_use_interpreter,
                        reinterpret_cast<void*>(hook_ShouldUseInterpreterEntrypoint));
            }*/
        }
    }

    WellKnownClasses::Init(env);
}

static int FakeHandleHiddenApi() {
    return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-macro-usage"

void Android::DisableHiddenApiPolicy(const ElfImg* handle) {
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
        // Android Q, for Domain::kApplication
        HOOK("_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_8ArtFieldEEEbPT_NS0_7ApiListENS0_12AccessMethodE");
        HOOK("_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_9ArtMethodEEEbPT_NS0_7ApiListENS0_12AccessMethodE");

        // For Domain::kPlatform
        HOOK("_ZN3art9hiddenapi6detail30HandleCorePlatformApiViolationINS_8ArtFieldEEEbPT_RKNS0_13AccessContextENS0_12AccessMethodENS0_17EnforcementPolicyE");
        HOOK("_ZN3art9hiddenapi6detail30HandleCorePlatformApiViolationINS_9ArtMethodEEEbPT_RKNS0_13AccessContextENS0_12AccessMethodENS0_17EnforcementPolicyE");
    } else {
        // Android P
        HOOK("_ZN3art9hiddenapi6detail19GetMemberActionImplINS_8ArtFieldEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE");
        HOOK("_ZN3art9hiddenapi6detail19GetMemberActionImplINS_9ArtMethodEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE");
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

JNIEnv* Android::GetEnvOrAttach() {
    JNIEnv* env;
    int status = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (UNLIKELY(status == JNI_EDETACHED)) {
        LOGW("The current thread are not attached to the java vm. Attaching...");
        status = jvm->AttachCurrentThread(&env, nullptr);
    }
    CHECK_EQ(status, JNI_OK, "JNI GetEnv returned error %d", status);
    return env;
}

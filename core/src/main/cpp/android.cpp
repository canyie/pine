//
// Created by canyie on 2020/3/15.
//

#include <unistd.h>
#include <string>
#include "android.h"
#include "utils/elf_img.h"
#include "utils/well_known_classes.h"
#include "art/art_method.h"
#include "art/jit.h"

using namespace pine;

int Android::version = -1;
JavaVM *Android::jvm = nullptr;
void (*Android::suspend_vm)() = nullptr;
void (*Android::resume_vm)() = nullptr;

inline bool CanRead(const char *file) {
    return access(file, R_OK) == 0;
}

void Android::Init(JNIEnv *env, int sdk_version) {
    Android::version = sdk_version;
    if (UNLIKELY(env->GetJavaVM(&jvm) != JNI_OK)) {
        LOGF("Cannot get java vm");
        env->FatalError("Cannot get java vm");
        abort();
    }

    WellKnownClasses::Init(env);

    const char *art_lib_path;
    const char *jit_lib_path;

    if (Is64Bit()) {
        if (sdk_version >= VERSION_Q && CanRead("/apex/com.android.runtime/lib64/libart.so")) {
            art_lib_path = "/apex/com.android.runtime/lib64/libart.so";;
            jit_lib_path = "/apex/com.android.runtime/lib64/libart-compiler.so";
        } else {
            art_lib_path = "/system/lib64/libart.so";
            jit_lib_path = "/system/lib64/libart-compiler.so";
        }
    } else {
        if (sdk_version >= VERSION_Q && CanRead("/apex/com.android.runtime/lib/libart.so")) {
            art_lib_path = "/apex/com.android.runtime/lib/libart.so";
            jit_lib_path = "/apex/com.android.runtime/lib/libart-compiler.so";
        } else {
            art_lib_path = "/system/lib/libart.so";
            jit_lib_path = "/system/lib/libart-compiler.so";
        }
    }

    {
        ElfImg art_lib_handle(art_lib_path);
        suspend_vm = reinterpret_cast<void (*)()>(
                art_lib_handle.GetSymbolAddress("_ZN3art3Dbg9SuspendVMEv")); // art::Dbg::SuspendVM()
        resume_vm = reinterpret_cast<void (*)()>(
                art_lib_handle.GetSymbolAddress("_ZN3art3Dbg8ResumeVMEv")); // art::Dbg::ResumeVM()
        art::Thread::Init(&art_lib_handle);
        art::ArtMethod::Init(&art_lib_handle);
        if (sdk_version >= VERSION_N) {
            ElfImg jit_lib_handle(jit_lib_path);
            art::Jit::Init(&art_lib_handle, &jit_lib_handle);
        }
    }
}





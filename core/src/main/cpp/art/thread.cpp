//
// Created by canyie on 2020/3/31.
//

#include "thread.h"

using namespace pine::art;

jobject (*Thread::new_local_ref)(JNIEnv *, void *) = nullptr;

jweak (*Thread::add_weak_global_ref)(JavaVM *, Thread *, void *) = nullptr;

void *(*Thread::decode_jobject)(Thread *, jobject) = nullptr;

void Thread::Init(ElfImg *art_lib_handle) {
    new_local_ref = reinterpret_cast<jobject (*)(JNIEnv *, void *)>(art_lib_handle->GetSymbolAddress(
            "_ZN3art9JNIEnvExt11NewLocalRefEPNS_6mirror6ObjectE")); // art::JNIEnvExt::NewLocalRef(art::mirror::Object *)

    if (UNLIKELY(!new_local_ref)) {
        LOGW("JNIEnvExt::NewLocalRef is unavailable, try JavaVMExt::AddWeakGlobalReference");
        const char *add_global_weak_ref_symbol;
        if (Android::version < Android::VERSION_M) {
            // art::JavaVMExt::AddWeakGlobalReference(art::Thread *, art::mirror::Object *)
            add_global_weak_ref_symbol = "_ZN3art9JavaVMExt22AddWeakGlobalReferenceEPNS_6ThreadEPNS_6mirror6ObjectE";
        } else if (Android::version < Android::VERSION_O) {
            // art::JavaVMExt::AddWeakGlobalRef(art::Thread *, art::mirror::Object *)
            add_global_weak_ref_symbol = "_ZN3art9JavaVMExt16AddWeakGlobalRefEPNS_6ThreadEPNS_6mirror6ObjectE";
        } else {
            // art::JavaVMExt::AddWeakGlobalRef(art::Thread *, art::ObjPtr<art::mirror::Object>)
            add_global_weak_ref_symbol = "_ZN3art9JavaVMExt16AddWeakGlobalRefEPNS_6ThreadENS_6ObjPtrINS_6mirror6ObjectEEE";
        }
        add_weak_global_ref = reinterpret_cast<jweak (*)(JavaVM *, Thread *, void *)>(
                art_lib_handle->GetSymbolAddress(add_global_weak_ref_symbol));
    }

    decode_jobject = reinterpret_cast<void *(*)(Thread *, jobject)>(art_lib_handle->GetSymbolAddress(
            "_ZNK3art6Thread13DecodeJObjectEP8_jobject")); // art::Thread::DecodeJObject(_jobject *)
}

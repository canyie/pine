//
// Created by canyie on 2021/3/13.
//

#include <cstring>
#include <cerrno>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <jni.h>
#include <android/log.h>
#include <bits/sysconf.h>
#include <sys/mman.h>
#include <dobby.h>

#define LOG_TAG "PineEnhances"
#define EXPORT JNIEXPORT extern "C"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

// Special flag, means "delaying, not yet actually hooked"
#define DELAYING (nullptr)
// Special flag, means "redirects farther entry update, but backup is not available yet".
#define REDIRECT_ENTRY_UPDATE (reinterpret_cast<void* const>(0x1))

static JavaVM* jvm_;
static jclass PineEnhances_;
static jmethodID onClassInit_;
static void*(*GetClassDef)(void* cls) = nullptr;
static size_t page_size_ = static_cast<const size_t>(sysconf(_SC_PAGESIZE));
static std::unordered_set<void*> cared_classes_;
static std::unordered_map<void*, void*> hooked_methods_;
static std::unordered_map<void*, const void*> pending_entries_;
static std::mutex cared_classes_mutex_;
static std::shared_mutex hooked_methods_mutex_;
static std::mutex pending_entries_mutex_;
static bool care_no_class_def_ = false;

static void* instrumentation_ = nullptr;

static void* (*FindElfSymbol)(void*, const char*, bool);

class ScopedLock {
public:
    inline explicit ScopedLock(std::mutex& mutex) : mLock(mutex)  { mLock.lock(); }
    inline explicit ScopedLock(std::mutex* mutex) : mLock(*mutex) { mLock.lock(); }
    inline ~ScopedLock() { mLock.unlock(); }
private:
    std::mutex& mLock;

    ScopedLock(const ScopedLock&) = delete;
    void operator=(const ScopedLock&) = delete;
};

JNIEnv* CurrentEnv() {
    JNIEnv* env;
    if (jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        jvm_->AttachCurrentThread(&env, nullptr);
    }
    return env;
}

static bool Unprotect(void* addr) {
    size_t alignment = (uintptr_t) addr % page_size_;
    void *aligned_ptr = (void*) ((uintptr_t) addr - alignment);
    int result = mprotect(aligned_ptr, page_size_, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (result == -1) {
        LOGE("mprotect failed for %p: %s (%d)", addr, strerror(errno), errno);
        return false;
    }
    return true;
}

bool AcquireClassCareFlag(void* ptr) {
    if (ptr == nullptr) return false;
    void* class_def = GetClassDef(ptr);
    if (class_def == nullptr) {
        return care_no_class_def_;
    }
    ScopedLock lk(cared_classes_mutex_);
    return cared_classes_.erase(class_def) != 0;
}

bool IsMethodHooked(void* method, bool redirect_entry_update) {
    std::shared_lock<std::shared_mutex> lk(hooked_methods_mutex_);
    auto i = hooked_methods_.find(method);
    if (i == hooked_methods_.end()) return false;
    if (redirect_entry_update) return i->second != DELAYING;
    return true;
}

void* GetMethodBackup(void* method) {
    std::shared_lock<std::shared_mutex> lk(hooked_methods_mutex_);
    auto i = hooked_methods_.find(method);
    if (i == hooked_methods_.end()) return nullptr;
    auto second = i->second;
    return second == REDIRECT_ENTRY_UPDATE ? nullptr : second;
}

void MaybeCallClassInitMonitor(void* ptr) {
    if (!AcquireClassCareFlag(ptr)) return;
    JNIEnv* env = CurrentEnv();
    env->CallStaticVoidMethod(PineEnhances_, onClassInit_, reinterpret_cast<jlong>(ptr));
    if (env->ExceptionCheck()) {
        LOGE("Unexpected exception threw in onClassInit");
        env->ExceptionClear();
    }
}

static bool HookFunc(void* target, void* replace, void** backup) {
    // Dobby does not unprotect the memory before reading it
    if (!Unprotect(target)) return false;
    return DobbyHook(target, replace, backup) == RS_SUCCESS;
}

static bool HookSymbol(void* handle, const char* symbol, void* replace, void** backup) {
    void* target = FindElfSymbol(handle, symbol, true);
    if (!target) return false;
    return HookFunc(target, replace, backup);
}

#define HOOK_ENTRY(name, return_type, ...) \
static return_type (*backup_##name)(__VA_ARGS__) = nullptr;\
return_type replace_##name (__VA_ARGS__)

HOOK_ENTRY(ShouldUseInterpreterEntrypoint, bool, void* method, const void* quick_code) {
    if (quick_code != nullptr && IsMethodHooked(method, false)) return false;
    return backup_ShouldUseInterpreterEntrypoint(method, quick_code);
}

HOOK_ENTRY(ShouldStayInSwitchInterpreter, bool, void* method) {
    if (IsMethodHooked(method, false)) return false;
    return backup_ShouldStayInSwitchInterpreter(method);
}

HOOK_ENTRY(FixupStaticTrampolines, void, void* thiz, void* cls) {
    backup_FixupStaticTrampolines(thiz, cls);
    MaybeCallClassInitMonitor(cls);
}

HOOK_ENTRY(FixupStaticTrampolinesWithThread, void, void* thiz, void* self, void* cls) {
    backup_FixupStaticTrampolinesWithThread(thiz, self, cls);
    MaybeCallClassInitMonitor(cls);
}

HOOK_ENTRY(MarkClassInitialized, void*, void* thiz, void* self, uint32_t* cls_ptr) {
    void* result = backup_MarkClassInitialized(thiz, self, cls_ptr);
    if (cls_ptr) MaybeCallClassInitMonitor(reinterpret_cast<void*>(*cls_ptr));
    return result;
}

HOOK_ENTRY(UpdateMethodsCode, void, void* thiz, void* method, const void* quick_code) {
    instrumentation_ = thiz;
    if (IsMethodHooked(method, true)) {
        auto backup = GetMethodBackup(method);
        if (backup) {
            // Redirect entry update to backup
            method = backup;
        } else {
            ScopedLock lk(pending_entries_mutex_);
            pending_entries_[method] = quick_code;
            return;
        }
    }
    backup_UpdateMethodsCode(thiz, method, quick_code);
}

void PineEnhances_careClassInit(JNIEnv*, jclass, jlong address) {
    void* ptr = reinterpret_cast<void*>(address);
    void* class_def = GetClassDef(ptr);
    if (class_def == nullptr) {
        // This class have no class def. That's mostly impossible, these classes (like proxy classes)
        // should be initialized before. But if it happens...
        LOGW("Class %p have no class def, this should not happen, please check the root cause", ptr);
        care_no_class_def_ = true;
        return;
    }
    ScopedLock lk(cared_classes_mutex_);
    cared_classes_.insert(class_def);
}

void PineEnhances_recordMethodHooked(JNIEnv*, jclass, jlong method, jlong backup) {
    auto o = reinterpret_cast<void*>(method);
    auto b = reinterpret_cast<void*>(backup);
    {
        std::unique_lock<std::shared_mutex> lk(hooked_methods_mutex_);
        hooked_methods_[o] = b;
    }
    if (!(instrumentation_ && backup_UpdateMethodsCode)) return;
    const void* saved_entry;
    {
        ScopedLock lk(pending_entries_mutex_);
        auto i = pending_entries_.find(o);
        if (i == pending_entries_.end()) return;
        saved_entry = i->second;
        pending_entries_.erase(i);
    }
    backup_UpdateMethodsCode(instrumentation_, b, saved_entry);
}

jboolean PineEnhances_initClassInitMonitor(JNIEnv* env, jclass PineEnhances, jint sdk_level,
                                           jlong openElf, jlong findElfSymbol, jlong closeElf) {
     onClassInit_ = env->GetStaticMethodID(PineEnhances, "onClassInit", "(J)V");
     if (!onClassInit_) {
         LOGE("Unable to find onClassInit");
         return JNI_FALSE;
     }
     PineEnhances_ = static_cast<jclass>(env->NewGlobalRef(PineEnhances));
     if (!PineEnhances_) {
         LOGE("Unable to make new global ref");
         return JNI_FALSE;
     }
     auto OpenElf = reinterpret_cast<void* (*)(const char*)>(openElf);
     FindElfSymbol = reinterpret_cast<void* (*)(void*, const char*, bool)>(findElfSymbol);
     auto CloseElf = reinterpret_cast<void (*)(void*)>(closeElf);

     void* handle = OpenElf("libart.so");

     GetClassDef = reinterpret_cast<void* (*)(void*)>(FindElfSymbol(handle,
             "_ZN3art6mirror5Class11GetClassDefEv", true));
     if (!GetClassDef) {
         LOGE("Cannot find symbol art::Class::GetClassDef");
         return JNI_FALSE;
     }

     bool hooked = false;
#define HOOK_FUNC(name) hooked |= HookFunc(name, (void*) replace_##name , (void**) &backup_##name)
#define HOOK_SYMBOL(name, symbol) hooked |= HookSymbol(handle, symbol, (void*) replace_##name , (void**) &backup_##name )

     // Before 7.0, it is NeedsInterpreter which is inlined so cannot be hooked
     // But the logic which forces code to be executed by interpreter is added in Android 8.0
     // And we have hooked UpdateMethodsCode to avoid entry updating, so it should be safe to skip
     if (sdk_level >= __ANDROID_API_N__) {
         HOOK_SYMBOL(ShouldUseInterpreterEntrypoint, "_ZN3art11ClassLinker30ShouldUseInterpreterEntrypointEPNS_9ArtMethodEPKv");
         if (!hooked) {
             // Android Tiramisu?
             HOOK_SYMBOL(ShouldStayInSwitchInterpreter, "_ZN3art11interpreter29ShouldStayInSwitchInterpreterEPNS_9ArtMethodE");
         }
         if (!hooked) {
             LOGE("Failed to hook ShouldUseInterpreterEntrypoint/ShouldStayInSwitchInterpreter. Hook may not work.");
         }
         hooked = false;
     }

     if (sdk_level >= __ANDROID_API_Q__) {
         // Note: kVisiblyInitialized is not implemented in Android Q,
         // but we found some ROMs "indicates that is Q", but uses R's art (has "visibly initialized" state)
         // https://github.com/crdroidandroid/android_art/commit/ef76ced9d2856ac988377ad99288a357697c4fa2
         void* MarkClassInitialized = FindElfSymbol(handle, "_ZN3art11ClassLinker20MarkClassInitializedEPNS_6ThreadENS_6HandleINS_6mirror5ClassEEE", sdk_level >= __ANDROID_API_R__);
         if (MarkClassInitialized) {
             HOOK_FUNC(MarkClassInitialized);
             HOOK_SYMBOL(FixupStaticTrampolinesWithThread, "_ZN3art11ClassLinker22FixupStaticTrampolinesEPNS_6ThreadENS_6ObjPtrINS_6mirror5ClassEEE");

             bool orig_hooked = hooked;
             hooked = false;
             HOOK_SYMBOL(UpdateMethodsCode, "_ZN3art15instrumentation15Instrumentation21UpdateMethodsCodeImplEPNS_9ArtMethodEPKv");
             if (!hooked) {
                 LOGE("Failed to hook UpdateMethodsCode, something may not work!");
             }
             hooked = orig_hooked;
         }
     }
     HOOK_SYMBOL(FixupStaticTrampolines, "_ZN3art11ClassLinker22FixupStaticTrampolinesENS_6ObjPtrINS_6mirror5ClassEEE");
     if (!hooked) {
         // Before Android 8.0, it uses raw mirror::Class* without ObjPtr<>
         HOOK_SYMBOL(FixupStaticTrampolines, "_ZN3art11ClassLinker22FixupStaticTrampolinesEPNS_6mirror5ClassE");
     }
#undef HOOK_SYMBOL

     CloseElf(handle);

     if (!hooked) {
         LOGE("Cannot hook MarkClassInitialized or FixupStaticTrampolines");
         return JNI_FALSE;
     }
    return JNI_TRUE;
}

 JNINativeMethod JNI_METHODS[] = {
         {"initClassInitMonitor", "(IJJJ)Z", (void*) PineEnhances_initClassInitMonitor},
         {"careClassInit", "(J)V", (void*) PineEnhances_careClassInit},
         {"recordMethodHooked", "(JJ)V", (void*) PineEnhances_recordMethodHooked}
};

EXPORT bool init_PineEnhances(JavaVM* jvm, JNIEnv* env, jclass cls) {
    jvm_ = jvm;
    return env->RegisterNatives(cls, JNI_METHODS, NELEM(JNI_METHODS)) == JNI_OK;
}

EXPORT jint JNI_OnLoad(JavaVM* jvm, void*) {
    JNIEnv* env;
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Cannot get jni env");
        return JNI_ERR;
    }
    jclass PineEnhances = env->FindClass("top/canyie/pine/enhances/PineEnhances");
    if (PineEnhances == nullptr) {
        LOGE("Cannot find class top/canyie/pine/enhances/PineEnhances");
        return JNI_ERR;
    }
    if (!init_PineEnhances(jvm, env, PineEnhances)) {
        LOGE("Cannot register native methods of class PineEnhances");
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

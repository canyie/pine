//
// Created by canyie on 2021/3/13.
//

#include <cstring>
#include <cerrno>
#include <map>
#include <set>
#include <list>
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

typedef void const* ClassDef;
typedef void const* ArtMethod;
typedef struct {
    ArtMethod target;
    ArtMethod backup;
    const void* entrypoint;
} HookRecord;

// Special flag, means "redirects farther entry update, but backup is not available yet".
#define REDIRECT_ENTRY_UPDATE (nullptr)

static JavaVM* jvm_;
static jclass PineEnhances_;
static jmethodID onClassInit_;
static ClassDef(*GetClassDef)(void* cls) = nullptr;
static size_t page_size_ = static_cast<const size_t>(sysconf(_SC_PAGESIZE));

// Legacy ClassInitMonitor implementation.
static std::set<ClassDef> cared_classes_;
static std::mutex cared_classes_mutex_;
static bool care_no_class_def_ = false;

// target -> backup if hooked, REDIRECT_ENTRY_UPDATE if not hooked but need to prevent entry update
static std::map<ArtMethod, ArtMethod> hooked_methods_;
static std::shared_mutex hooked_methods_mutex_;

// target -> entry point (pending)
static std::map<ArtMethod, const void*> pending_entries_;
static std::mutex pending_entries_mutex_;

// declaring class -> hook record list
static std::map<ClassDef, std::list<const HookRecord>> hook_records_;
static std::shared_mutex hook_records_mutex_;

static void* instrumentation_ = nullptr;

static void* (*FindElfSymbol)(void*, const char*, bool);
static void* (*GetMethodDeclaringClass)(ArtMethod);
static void (*SyncMethodEntry)(ArtMethod target, ArtMethod backup, const void* entry);

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

bool IsMethodHooked(ArtMethod method) {
    std::shared_lock<std::shared_mutex> lk(hooked_methods_mutex_);
    auto i = hooked_methods_.find(method);
    return i != hooked_methods_.end();
}

ArtMethod GetMethodBackup(ArtMethod method) {
    std::shared_lock<std::shared_mutex> lk(hooked_methods_mutex_);
    auto i = hooked_methods_.find(method);
    if (i == hooked_methods_.end()) return nullptr;
    auto second = i->second;
    return second == REDIRECT_ENTRY_UPDATE ? nullptr : second;
}

void MaybeClassInit(void* ptr) {
    if (ptr == nullptr) return;
    auto class_def = GetClassDef(ptr);
    bool call_monitor;
    if (class_def) {
        {
            std::shared_lock<std::shared_mutex> lk(hook_records_mutex_);
            auto i = hook_records_.find(class_def);
            if (i != hook_records_.end()) {
                for (const HookRecord& record : i->second) {
                    LOGE("Restore %p to %p", record.target, record.entrypoint);
                    SyncMethodEntry(record.target, record.backup, record.entrypoint);
                }
            }
        }
        ScopedLock lk(cared_classes_mutex_);
        call_monitor = cared_classes_.erase(class_def) != 0;
    } else {
        call_monitor = care_no_class_def_;
    }
    if (!call_monitor) return;
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

static bool HookSymbol(void* handle, const char* symbol, void* replace, void** backup, bool required) {
    void* target = FindElfSymbol(handle, symbol, required);
    if (!target) return false;
    return HookFunc(target, replace, backup);
}

#define HOOK_ENTRY(name, return_type, ...) \
static return_type (*backup_##name)(__VA_ARGS__) = nullptr;\
return_type replace_##name (__VA_ARGS__)

HOOK_ENTRY(ShouldUseInterpreterEntrypoint, bool, void* method, const void* quick_code) {
    if (quick_code != nullptr && IsMethodHooked(method)) return false;
    return backup_ShouldUseInterpreterEntrypoint(method, quick_code);
}

HOOK_ENTRY(ShouldStayInSwitchInterpreter, bool, void* method) {
    if (IsMethodHooked(method)) return false;
    return backup_ShouldStayInSwitchInterpreter(method);
}

HOOK_ENTRY(FixupStaticTrampolines, void, void* thiz, void* cls) {
    backup_FixupStaticTrampolines(thiz, cls);
    MaybeClassInit(cls);
}

HOOK_ENTRY(FixupStaticTrampolinesWithThread, void, void* thiz, void* self, void* cls) {
    backup_FixupStaticTrampolinesWithThread(thiz, self, cls);
    MaybeClassInit(cls);
}

HOOK_ENTRY(MarkClassInitialized, void*, void* thiz, void* self, uint32_t* cls_ptr) {
    void* result = backup_MarkClassInitialized(thiz, self, cls_ptr);
    if (cls_ptr) MaybeClassInit(reinterpret_cast<void*>(*cls_ptr));
    return result;
}

bool PreUpdateMethodsCode(void* thiz, ArtMethod& method, const void*& quick_code) {
    instrumentation_ = thiz;
    if (IsMethodHooked(method)) {
        auto backup = GetMethodBackup(method);
        if (backup) {
            // Redirect entry update to backup
            method = backup;
        } else {
            ScopedLock lk(pending_entries_mutex_);
            pending_entries_[method] = quick_code;
            return true;
        }
    }
    return false;
}

HOOK_ENTRY(UpdateMethodsCode, void, void* thiz, ArtMethod method, const void* quick_code) {
    if (PreUpdateMethodsCode(thiz, method, quick_code)) return;
    backup_UpdateMethodsCode(thiz, method, quick_code);
}

HOOK_ENTRY(UpdateMethodsCodeImpl, void, void* thiz, ArtMethod method, const void* quick_code) {
    if (PreUpdateMethodsCode(thiz, method, quick_code)) return;
    backup_UpdateMethodsCodeImpl(thiz, method, quick_code);
}

HOOK_ENTRY(InitializeMethodsCode, void, void* thiz, ArtMethod method, const void* aot_code) {
    if (PreUpdateMethodsCode(thiz, method, aot_code)) return;
    backup_InitializeMethodsCode(thiz, method, aot_code);
}

void PineEnhances_careClassInit(JNIEnv*, jclass, jlong address) {
    void* ptr = reinterpret_cast<void*>(address);
    auto class_def = GetClassDef(ptr);
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

void PineEnhances_recordMethodHooked(JNIEnv*, jclass, jlong target, jlong entry, jlong backup) {
    auto o = reinterpret_cast<ArtMethod>(target);
    auto b = reinterpret_cast<ArtMethod>(backup);
    {
        std::unique_lock<std::shared_mutex> lk(hooked_methods_mutex_);
        hooked_methods_[o] = b;
    }
    if (b == REDIRECT_ENTRY_UPDATE) return;
    // Record hooked methods using declaring class def.
    {
        std::unique_lock<std::shared_mutex> lk(hook_records_mutex_);
        const HookRecord record {
            .target = o,
            .backup = b,
            .entrypoint = reinterpret_cast<const void*>(entry)
        };
        // null class def means it's a runtime class (e.g. proxy class) which is already
        // visibly initialized so just skip
        // Shall we do STW here to prevent it from being moved by GC?
        auto class_def = GetClassDef( GetMethodDeclaringClass(o));
        if (class_def) hook_records_[class_def].emplace_back(record);
    }

    // Sync pending entry point to backup if needed
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

std::string GetRuntimeLibraryName(JNIEnv* env) {
    // initClassInitMonitor will always be called after Pine core library is initialized
    // At this time we can directly access hidden APIs
    jclass VMRuntime = env->FindClass("dalvik/system/VMRuntime");
    jmethodID getRuntime = VMRuntime
            ? env->GetStaticMethodID(VMRuntime, "getRuntime", "()Ldalvik/system/VMRuntime;")
            : nullptr;
    jmethodID getVMLibrary = getRuntime
            ? env->GetMethodID(VMRuntime, "vmLibrary", "()Ljava/lang/String;")
            : nullptr;
    if (getVMLibrary) {
        jobject vmRuntime = env->CallStaticObjectMethod(VMRuntime, getRuntime);
        if (vmRuntime) {
            jstring vmLibrary = static_cast<jstring>(env->CallObjectMethod(vmRuntime, getVMLibrary));
            env->DeleteLocalRef(vmRuntime);
            if (vmLibrary) {
                env->DeleteLocalRef(VMRuntime);
                const char* vm_library_cstr = env->GetStringUTFChars(vmLibrary, nullptr);
                std::string vm_library(vm_library_cstr);
                env->ReleaseStringUTFChars(vmLibrary, vm_library_cstr);
                env->DeleteLocalRef(vmLibrary);
                return vm_library;
            }
        }
    }
    LOGE("Failed to get VM library name");
    env->ExceptionDescribe();
    env->ExceptionClear();
    if (VMRuntime) env->DeleteLocalRef(VMRuntime);
    return "libart.so";
}

jboolean PineEnhances_initClassInitMonitor(JNIEnv* env, jclass PineEnhances, jint sdk_level,
                                           jlong openElf, jlong findElfSymbol, jlong closeElf,
                                           jlong getMethodDeclaringClass, jlong syncMethodEntry) {
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
     GetMethodDeclaringClass = reinterpret_cast<void* (*)(ArtMethod)>(getMethodDeclaringClass);
     SyncMethodEntry = reinterpret_cast<void (*)(ArtMethod, ArtMethod, const void*)>(syncMethodEntry);

     auto vm_library = GetRuntimeLibraryName(env);
     void* handle = OpenElf(vm_library.data());

     GetClassDef = reinterpret_cast<ClassDef (*)(void*)>(FindElfSymbol(handle,
             "_ZN3art6mirror5Class11GetClassDefEv", true));
     if (!GetClassDef) {
         LOGE("Cannot find symbol art::Class::GetClassDef");
         return JNI_FALSE;
     }

     bool hooked = false;
#define HOOK_FUNC(name) hooked |= HookFunc(name, (void*) replace_##name , (void**) &backup_##name)
#define HOOK_SYMBOL(name, symbol, required) hooked |= HookSymbol(handle, symbol, (void*) replace_##name , (void**) &backup_##name , required)

     // Before 7.0, it is NeedsInterpreter which is inlined so cannot be hooked
     // But the logic which forces code to be executed by interpreter is added in Android 8.0
     // And we have hooked UpdateMethodsCode to avoid entry updating, so it should be safe to skip
     if (sdk_level >= __ANDROID_API_N__) {
         HOOK_SYMBOL(ShouldUseInterpreterEntrypoint, "_ZN3art11ClassLinker30ShouldUseInterpreterEntrypointEPNS_9ArtMethodEPKv", false);
         if (!hooked) {
             // Android Tiramisu?
             HOOK_SYMBOL(ShouldStayInSwitchInterpreter, "_ZN3art11interpreter29ShouldStayInSwitchInterpreterEPNS_9ArtMethodE", true);
         }
         if (!hooked) {
             LOGE("Failed to hook ShouldUseInterpreterEntrypoint/ShouldStayInSwitchInterpreter. Hook may not work.");
         }
         hooked = false;

         HOOK_SYMBOL(UpdateMethodsCodeImpl, "_ZN3art15instrumentation15Instrumentation21UpdateMethodsCodeImplEPNS_9ArtMethodEPKv", true);
     }
     else
         HOOK_SYMBOL(UpdateMethodsCode, "_ZN3art15instrumentation15Instrumentation17UpdateMethodsCodeEPNS_9ArtMethodEPKv", true);
     if (sdk_level >= __ANDROID_API_T__) {
         HOOK_SYMBOL(InitializeMethodsCode, "_ZN3art15instrumentation15Instrumentation21InitializeMethodsCodeEPNS_9ArtMethodEPKv", true);
     }

     if (sdk_level >= __ANDROID_API_Q__) {
         // Note: kVisiblyInitialized is not implemented in Android Q,
         // but we found some ROMs "indicates that is Q", but uses R's art (has "visibly initialized" state)
         // https://github.com/crdroidandroid/android_art/commit/ef76ced9d2856ac988377ad99288a357697c4fa2
         void* MarkClassInitialized = FindElfSymbol(handle, "_ZN3art11ClassLinker20MarkClassInitializedEPNS_6ThreadENS_6HandleINS_6mirror5ClassEEE", sdk_level >= __ANDROID_API_R__);
         if (MarkClassInitialized) {
             HOOK_FUNC(MarkClassInitialized);
             HOOK_SYMBOL(FixupStaticTrampolinesWithThread, "_ZN3art11ClassLinker22FixupStaticTrampolinesEPNS_6ThreadENS_6ObjPtrINS_6mirror5ClassEEE", false);
         }
     }

     HOOK_SYMBOL(FixupStaticTrampolines, "_ZN3art11ClassLinker22FixupStaticTrampolinesENS_6ObjPtrINS_6mirror5ClassEEE", false);
     if (!hooked) {
         // Before Android 8.0, it uses raw mirror::Class* without ObjPtr<>
         HOOK_SYMBOL(FixupStaticTrampolines, "_ZN3art11ClassLinker22FixupStaticTrampolinesEPNS_6mirror5ClassE", true);
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
         {"initClassInitMonitor", "(IJJJJJ)Z", (void*) PineEnhances_initClassInitMonitor},
         {"careClassInit", "(J)V", (void*) PineEnhances_careClassInit},
         {"recordMethodHooked", "(JJJ)V", (void*) PineEnhances_recordMethodHooked}
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

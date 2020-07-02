//
// Created by canyie on 2020/2/9.
//

#include <jni.h>
#include "art_method.h"
#include "../jni_bridge.h"
#include "../utils/elf_img.h"
#include "../utils/well_known_classes.h"
#include "../utils/scoped_local_ref.h"
#include "../utils/memory.h"

using namespace pine::art;

uint32_t ArtMethod::kAccCompileDontBother = 0;

size_t ArtMethod::size = 0;
void* ArtMethod::art_quick_to_interpreter_bridge = nullptr;
void* ArtMethod::art_quick_generic_jni_trampoline = nullptr;
void* ArtMethod::art_interpreter_to_compiled_code_bridge = nullptr;
void* ArtMethod::art_interpreter_to_interpreter_bridge = nullptr;

void (*ArtMethod::copy_from)(ArtMethod*, ArtMethod*, size_t) = nullptr;

Member<ArtMethod, uint32_t> ArtMethod::access_flags_;
Member<ArtMethod, void*> ArtMethod::entry_point_from_jni_;
Member<ArtMethod, void*> ArtMethod::entry_point_from_compiled_code_;
Member<ArtMethod, void*>* ArtMethod::entry_point_from_interpreter_;
Member<ArtMethod, uint32_t>* ArtMethod::declaring_class = nullptr;

void ArtMethod::Init(const ElfImg* handle) {
    art_quick_to_interpreter_bridge = handle->GetSymbolAddress("art_quick_to_interpreter_bridge");
    art_quick_generic_jni_trampoline = handle->GetSymbolAddress("art_quick_generic_jni_trampoline");

    if (Android::version < Android::VERSION_N) {
        art_interpreter_to_compiled_code_bridge = handle->GetSymbolAddress(
                "artInterpreterToCompiledCodeBridge");
        art_interpreter_to_interpreter_bridge = handle->GetSymbolAddress(
                "artInterpreterToInterpreterBridge");
    }

    const char* symbol_copy_from = nullptr;
    if (Android::version >= Android::VERSION_O) {
        // art::ArtMethod::CopyFrom(art::ArtMethod *, art::PointerSize)
        symbol_copy_from = "_ZN3art9ArtMethod8CopyFromEPS0_NS_11PointerSizeE";
    } else if (Android::version >= Android::VERSION_N) {
#ifdef __LP64__
        // art::ArtMethod::CopyFrom(art::ArtMethod *, unsigned long)
        symbol_copy_from = "_ZN3art9ArtMethod8CopyFromEPS0_m";
#else
        // art::ArtMethod::CopyFrom(art::ArtMethod *, unsigned int)
        symbol_copy_from = "_ZN3art9ArtMethod8CopyFromEPS0_j";
#endif
    } else if (Android::version >= Android::VERSION_M) {
#ifdef __LP64__
        // art::ArtMethod::CopyFrom(art::ArtMethod const *, unsigned long)
        symbol_copy_from = "_ZN3art9ArtMethod8CopyFromEPKS0_m";
#else
        // art::ArtMethod::CopyFrom(art::ArtMethod const *, unsigned int)
        symbol_copy_from = "_ZN3art9ArtMethod8CopyFromEPKS0_j";
#endif
    }

    if (symbol_copy_from)
        copy_from = reinterpret_cast<void (*)(ArtMethod*, ArtMethod*, size_t)>(
                handle->GetSymbolAddress(symbol_copy_from));
}

ArtMethod* ArtMethod::FromReflectedMethod(JNIEnv* env, jobject javaMethod) {
    jmethodID m = env->FromReflectedMethod(javaMethod);
    if (Android::version >= Android::VERSION_R) {
        if (reinterpret_cast<uintptr_t>(m) & 1) {
            return GetArtMethodForR(env, javaMethod);
        }
    }
    return reinterpret_cast<ArtMethod*>(m);
}

ArtMethod*
ArtMethod::Require(JNIEnv* env, jclass c, const char* name, const char* signature, bool is_static) {
    jmethodID m = is_static ? env->GetStaticMethodID(c, name, signature)
                            : env->GetMethodID(c, name, signature);
    if (Android::version >= Android::VERSION_R) {
        if (reinterpret_cast<uintptr_t>(m) & 1) {
            ScopedLocalRef javaMethod(env, env->ToReflectedMethod(c, m, static_cast<jboolean>(is_static)));
            return GetArtMethodForR(env, javaMethod.Get());
        }
    }
    return reinterpret_cast<ArtMethod*>(m);
}

static inline size_t Difference(intptr_t a, intptr_t b) {
    intptr_t size = b - a;
    if (size < 0) size = -size;
    return static_cast<size_t>(size);
}

static inline uint32_t Align(uint32_t offset, uint32_t align_with) {
    uint32_t alignment = offset % align_with;
    if (alignment) {
        offset += (align_with - alignment);
    }
    return offset;
}

void ArtMethod::InitMembers(ArtMethod* m1, ArtMethod* m2, uint32_t access_flags) {
    if (Android::version >= Android::VERSION_N) {
        kAccCompileDontBother = (Android::version >= Android::VERSION_O_MR1)
                                ? AccessFlags::kCompileDontBother_O_MR1
                                : AccessFlags::kCompileDontBother_N;
    }

    size = Difference(reinterpret_cast<intptr_t>(m1), reinterpret_cast<intptr_t>(m2));
    int android_version = Android::version;
    if (LIKELY(android_version >= Android::VERSION_L)) {
        for (uint32_t offset = 0; offset < size; offset += 2) {
            void* ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m1) + offset);
            if ((*static_cast<uint32_t*>(ptr)) == access_flags) {
                access_flags_.SetOffset(offset);
            } else if (UNLIKELY(android_version == Android::VERSION_L)) {
                // On Android 5.0, type of entry_point_from_jni_ is uint64_t
                if ((*static_cast<uint64_t*>(ptr)) == reinterpret_cast<uint64_t>(Ruler_m1))
                    entry_point_from_jni_.SetOffset(offset);
            } else if ((*static_cast<void**>(ptr)) == Ruler_m1) {
                entry_point_from_jni_.SetOffset(offset);
            }

            bool done = access_flags_.IsValid() && entry_point_from_jni_.IsValid();
            if (UNLIKELY(done)) break;
        }

        if (UNLIKELY(!access_flags_.IsValid())) {
            do {
                if (LIKELY(Android::version >= Android::VERSION_N)) {
                    // TODO: Is this really possible?
                    LOGW("failed to find access_flags_ with default access flags, try again with kAccCompileDontBother");
                    access_flags |= kAccCompileDontBother;
                    int offset = Memory::FindOffset(m1, access_flags, size, 2);
                    if (LIKELY(offset >= 0)) {
                        LOGW("Found access_flags_ with kAccCompileDontBother, offset %d", offset);
                        access_flags_.SetOffset(offset);
                        break;
                    }

                    if (LIKELY(Android::version >= Android::VERSION_R)) {
                        // Android R has a new access flags: kAccPreCompiled
                        // TODO: Is this really possible?
                        LOGW("failed to find access_flags_ with default access flags, try again with kAccPreCompiled");
                        access_flags |= AccessFlags::kPreCompiled;
                        // Don't clear kAccCompileDontBother.
                        offset = Memory::FindOffset(m1, access_flags, size, 2);
                        if (LIKELY(offset >= 0)) {
                            LOGW("Found access_flags_ with kAccPreCompiled, offset %d", offset);
                            access_flags_.SetOffset(offset);
                            break;
                        }
                    }
                }
                LOGE("Member access_flags_ not found in ArtMethod, use default.");
                access_flags_.SetOffset(GetDefaultAccessFlagsOffset());
            } while (false);
        }

        uint32_t entry_point_member_size = Android::version == Android::VERSION_L
                                           ? sizeof(uint64_t) : sizeof(void*);

        if (LIKELY(entry_point_from_jni_.IsValid())) {
            uint32_t compiled_code_entry_offset = entry_point_from_jni_.GetOffset()
                                                  + entry_point_member_size;

            if (Android::version >= Android::VERSION_O) {
                // Only align offset on Android O+ (PtrSizedFields is PACKED(4) in Android N or lower.)
                compiled_code_entry_offset = Align(compiled_code_entry_offset,
                                                   entry_point_member_size);
            }

            entry_point_from_compiled_code_.SetOffset(compiled_code_entry_offset);

        } else {
            LOGE("Member entry_point_from_jni_ not found in ArtMethod, use default.");
            entry_point_from_jni_.SetOffset(GetDefaultEntryPointFromJniOffset());
            entry_point_from_compiled_code_.SetOffset(
                    GetDefaultEntryPointFromQuickCompiledCodeOffset());
        }

        if (Android::version < Android::VERSION_N) {
            // Not align: PtrSizedFields is PACKED(4) in the android version.
            entry_point_from_interpreter_ = new Member<ArtMethod, void*>(
                    entry_point_from_jni_.GetOffset() - entry_point_member_size);
        } else {
            // On Android 7.0+, the declaring_class may be moved by the GC,
            // so we check and update it when invoke backup method.
            declaring_class = new Member<ArtMethod, uint32_t>(0);
        }
    } else {
        // Hardcode members offset for Kitkat :(
        LOGW("Android Kitkat, hardcode offset only...");
        access_flags_.SetOffset(28);
        entry_point_from_compiled_code_.SetOffset(32);

        // FIXME This offset has not been verified, so it may be wrong
        entry_point_from_interpreter_ = new Member<ArtMethod, void*>(36);
    }
}

void ArtMethod::BackupFrom(ArtMethod* source, void* entry, bool is_inline_hook,
        bool is_native_or_proxy) {
    if (LIKELY(copy_from)) {
        copy_from(this, source, sizeof(void*));
    } else {
        memcpy(this, source, size);
    }

    uint32_t access_flags = source->GetAccessFlags();
    if (Android::version >= Android::VERSION_N) {
        access_flags |= kAccCompileDontBother;
    }
    if ((access_flags & AccessFlags::kStatic) == 0) {
        // Non-static method, set kAccPrivate to ensure it is a direct method.
        access_flags &= ~(AccessFlags::kPublic | AccessFlags::kProtected);
        access_flags |= AccessFlags::kPrivate;
    }
    access_flags &= ~AccessFlags::kConstructor;
    SetAccessFlags(access_flags);

    if (UNLIKELY(Android::version >= Android::VERSION_N
                 && !is_inline_hook
                 && !is_native_or_proxy
                 && art_quick_to_interpreter_bridge)) {
        // On Android N+, the method may compiled by JIT, and unknown problem occurs when calling
        // the backup method if we use entry replacement mode. Just use the interpreter to execute.
        // Possible reason: compiled code is recycled in JIT garbage collection.
        SetEntryPointFromCompiledCode(art_quick_to_interpreter_bridge);

        // For non-native and non-proxy methods, the entry_point_from_jni_ member is used to save
        // ProfilingInfo, and the ProfilingInfo may saved original compiled code entry, the interpreter
        // will jump directly to the saved_code_entry_ for execution. Clear entry_point_from_jni_ to avoid it.
        entry_point_from_jni_.Set(this, nullptr);
    } else {
        SetEntryPointFromCompiledCode(entry);

        // ArtMethod::CopyFrom() will clear data_ member, the member is used to save
        // the original interface method for proxy method. Restore it to avoid errors.
        if (UNLIKELY(is_native_or_proxy && Android::version >= Android::VERSION_O))
            SetEntryPointFromJni(source->GetEntryPointFromJni());
    }
}

void ArtMethod::AfterHook(bool is_inline_hook, bool debuggable, bool is_native_or_proxy) {
    uint32_t access_flags = GetAccessFlags();

    if (Android::version >= Android::VERSION_N) {
        access_flags |= kAccCompileDontBother;
    }

    if (Android::version >= Android::VERSION_O && !is_inline_hook) {
        if (UNLIKELY(debuggable && !is_native_or_proxy)) {
            // Android 8.0+ and debug mode, ART may force the use of interpreter mode,
            // and entry_point_from_compiled_code_ will be ignored. Set kAccNative to avoid it.
            // See ClassLinker::ShouldUseInterpreterEntrypoint(ArtMethod*, const void*)
            access_flags |= AccessFlags::kNative;
        }
    }

    if (Android::version >= Android::VERSION_Q) {
        // On Android 10+, a method can be execute with fast interpreter is cached in access flags,
        // and we may need to disable fast interpreter for a hooked method.
        // Clear the cached flag(kAccFastInterpreterToInterpreterInvoke) to refresh the state.
        access_flags &= ~AccessFlags::kFastInterpreterToInterpreterInvoke;
    }

    bool is_native = (access_flags & AccessFlags::kNative) != 0;
    if (UNLIKELY(is_native && Android::version >= Android::VERSION_L)) {
        // GC is disabled when executing FastNative and CriticalNative methods
        // and may cause deadlocks. This is not applicable for hooked methods.
        access_flags &= ~AccessFlags::kFastNative;
        if (Android::version >= Android::VERSION_P) {
            access_flags &= ~AccessFlags::kCriticalNative;
        }
    }

    SetAccessFlags(access_flags);

    if (art_interpreter_to_compiled_code_bridge)
        SetEntryPointFromInterpreter(art_interpreter_to_compiled_code_bridge);
}


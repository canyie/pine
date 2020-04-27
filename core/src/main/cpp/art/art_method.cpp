//
// Created by canyie on 2020/2/9.
//

#include <jni.h>
#include "art_method.h"
#include "../utils/elf_img.h"
#include "../jni_bridge.h"

using namespace pine::art;

uint32_t ArtMethod::kAccCompileDontBother = 0;

size_t ArtMethod::size = 0;
void *ArtMethod::art_quick_to_interpreter_bridge = nullptr;
void *ArtMethod::art_quick_generic_jni_trampoline = nullptr;
void *ArtMethod::art_interpreter_to_compiled_code_bridge = nullptr;
void *ArtMethod::art_interpreter_to_interpreter_bridge = nullptr;
void (*ArtMethod::copy_from)(ArtMethod *, ArtMethod *, size_t) = nullptr;

Member<ArtMethod, uint32_t> ArtMethod::access_flags_;
Member<ArtMethod, void *> ArtMethod::entry_point_from_jni_;
Member<ArtMethod, void *> ArtMethod::entry_point_from_compiled_code_;
Member<ArtMethod, void *> *ArtMethod::entry_point_from_interpreter_;
Member<ArtMethod, uint32_t> *ArtMethod::declaring_class = nullptr;

void ArtMethod::Init(ElfImg *handle) {
    art_quick_to_interpreter_bridge = handle->GetSymbolAddress("art_quick_to_interpreter_bridge");
    art_quick_generic_jni_trampoline = handle->GetSymbolAddress("art_quick_generic_jni_trampoline");

    if (Android::version < Android::VERSION_N) {
        art_interpreter_to_compiled_code_bridge = handle->GetSymbolAddress(
                "artInterpreterToCompiledCodeBridge");
        art_interpreter_to_interpreter_bridge = handle->GetSymbolAddress(
                "artInterpreterToInterpreterBridge");
    }

    const char *symbol_copy_from = nullptr;
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
        copy_from = reinterpret_cast<void (*)(ArtMethod *, ArtMethod *,size_t)>(
                handle->GetSymbolAddress(symbol_copy_from));
}

inline uint32_t Align(uint32_t offset, uint32_t align_with) {
    uint32_t alignment = offset % align_with;
    if (alignment) {
        offset += (align_with - alignment);
    }
    return offset;
}

void ArtMethod::InitMembers(ArtMethod *m1, ArtMethod *m2, uint32_t access_flags) {
    if (Android::version >= Android::VERSION_N) {
        kAccCompileDontBother = (Android::version >= Android::VERSION_O_MR1)
                                ? AccessFlags::kAccCompileDontBother_O_MR1
                                : AccessFlags::kAccCompileDontBother_N;
    }

    size = static_cast<size_t>(reinterpret_cast<uintptr_t>(m2) - reinterpret_cast<uintptr_t>(m1));

    if (LIKELY(Android::version >= Android::VERSION_L)) {
        for (uint32_t offset = 0; offset < size; offset += 2) {
            void *ptr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m1) + offset);
            if ((*static_cast<uint32_t *>(ptr)) == access_flags) {
                access_flags_.SetOffset(offset);
            } else if ((*static_cast<void **>(ptr)) == Ruler_m1) {
                entry_point_from_jni_.SetOffset(offset);
            }

            bool done = access_flags_.IsValid() && entry_point_from_jni_.IsValid();
            if (UNLIKELY(done)) break;
        }

        if (UNLIKELY(!access_flags_.IsValid())) {
            LOGW("Member access_flags_ not found in ArtMethod, use default.");
            access_flags_.SetOffset(GetDefaultAccessFlagsOffset());
        }

        if (LIKELY(entry_point_from_jni_.IsValid())) {
            uint32_t entry_point_from_jni_size = Android::version == Android::VERSION_L
                                                 ? sizeof(uint64_t) : sizeof(void *);
            uint32_t compiled_code_entry_offset = entry_point_from_jni_.GetOffset()
                    + entry_point_from_jni_size;

            if (Android::version >= Android::VERSION_O) {
                // Only align offset on Android O+ (PtrSizedFields is PACKED(4) in Android N or lower.)
                compiled_code_entry_offset = Align(compiled_code_entry_offset,
                                                   entry_point_from_jni_size);
            }

            entry_point_from_compiled_code_.SetOffset(compiled_code_entry_offset);

        } else {
            LOGW("Member entry_point_from_jni_ not found in ArtMethod, use default.");
            entry_point_from_jni_.SetOffset(GetDefaultEntryPointFromJniOffset());
            entry_point_from_compiled_code_.SetOffset(
                    GetDefaultEntryPointFromQuickCompiledCodeOffset());
        }

        if (Android::version < Android::VERSION_N) {
            uint32_t entry_point_from_interpreter_size = Android::version == Android::VERSION_L
                    ? sizeof(uint64_t) : sizeof(void *);

            // Not align: PtrSizedFields is PACKED(4) in the android version.
            entry_point_from_interpreter_ = new Member<ArtMethod, void *>(
                    entry_point_from_jni_.GetOffset() - entry_point_from_interpreter_size);
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
        entry_point_from_interpreter_ = new Member<ArtMethod, void *>(36);
    }
}

void ArtMethod::BackupFrom(ArtMethod *source, void *entry) {
    if (copy_from) {
        copy_from(this, source, sizeof(void *));
    } else {
        memcpy(this, source, size);
    }

    uint32_t access_flags = source->GetAccessFlags();
    if (Android::version >= Android::VERSION_N) {
        access_flags |= kAccCompileDontBother;
    }
    if ((access_flags & AccessFlags::kAccStatic) == 0) {
        // Non-static method, set kAccPrivate to ensure it is a direct method.
        access_flags &= ~(AccessFlags::kAccPublic | AccessFlags::kAccProtected);
        access_flags |= AccessFlags::kAccPrivate;
    }
    access_flags &= ~AccessFlags::kAccConstructor;
    SetAccessFlags(access_flags);
    SetEntryPointFromCompiledCode(entry);
}

void ArtMethod::AfterHook(bool is_inline_hook, bool debuggable) {
    uint32_t access_flags = GetAccessFlags();
    access_flags &= ~(AccessFlags::kAccSynchronized | AccessFlags::kAccDeclaredSynchronized);

    if (Android::version >= Android::VERSION_N) {
        access_flags |= kAccCompileDontBother;
    }

    if (Android::version >= Android::VERSION_O && !is_inline_hook) {
        if (Android::version >= Android::VERSION_Q)
            access_flags &= ~AccessFlags::kAccFastInterpreterToInterpreterInvoke;

        if (UNLIKELY(debuggable)) {
            // Android 8.0+ and debug mode, ART may force the use of interpreter mode,
            // and entry_point_from_compiled_code_ will be ignored. Set kAccNative to avoid it.
            // See ClassLinker::ShouldUseInterpreterEntrypoint(ArtMethod*, const void*)
            access_flags |= AccessFlags::kAccNative;
        }
    }

    bool is_native = (access_flags & AccessFlags::kAccNative) != 0;
    if (UNLIKELY(is_native && Android::version >= Android::VERSION_L)) {
        // GC is disabled when executing FastNative and CriticalNative methods
        // and may cause deadlocks. This is not applicable for hooked methods.
        access_flags &= ~AccessFlags::kAccFastNative;
        if (Android::version >= Android::VERSION_P) {
            access_flags &= ~AccessFlags::kAccCriticalNative;
        }
    }

    SetAccessFlags(access_flags);

    if (art_interpreter_to_compiled_code_bridge)
        SetEntryPointFromInterpreter(art_interpreter_to_compiled_code_bridge);
}

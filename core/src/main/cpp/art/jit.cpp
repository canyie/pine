//
// Created by canyie on 2020/3/15.
//

#include "jit.h"
#include "../android.h"

using namespace pine::art;

JitCompiler *Jit::self_compiler = nullptr;

bool (*Jit::jit_compile_method)(void *, void *, void *,
                                bool) = nullptr;

bool (*Jit::jit_compile_method_q)(void *, void *, void *, bool,
                                  bool) = nullptr;

void **Jit::jit_update_options_ptr = nullptr;

JitCompiler **Jit::global_compiler_ptr = nullptr;

Member<void, size_t> *Jit::CompilerOptions_inline_max_code_units = nullptr;

void Jit::Init(const ElfImg *art_lib_handle, const ElfImg *jit_lib_handle) {
    global_compiler_ptr = static_cast<JitCompiler **>(art_lib_handle->GetSymbolAddress(
            "_ZN3art3jit3Jit20jit_compiler_handle_E"));

    auto jit_load = reinterpret_cast<JitCompiler *(*)(bool *)>(jit_lib_handle->GetSymbolAddress(
            "jit_load"));

    if (LIKELY(jit_load)) {
        bool generate_debug_info = false;
        self_compiler = jit_load(&generate_debug_info);
    } else {
        LOGW("Failed to create new JitCompiler: jit_load not found");
    }

    void *jit_compile_method = jit_lib_handle->GetSymbolAddress("jit_compile_method");

    if (Android::version >= Android::VERSION_Q) {
        Jit::jit_compile_method_q = reinterpret_cast<bool (*)(void *, void *, void *, bool,
                                                         bool)>(jit_compile_method);
        // Android Q, ART may update CompilerOptions and the value we set will be overwritten.
        // the function pointer saved in art::jit::Jit::jit_update_options_ .
        Jit::jit_update_options_ptr = static_cast<void **>(art_lib_handle->GetSymbolAddress(
                "_ZN3art3jit3Jit20jit_update_options_E"));
    } else {
        Jit::jit_compile_method = reinterpret_cast<bool (*)(void *, void *, void *,
                                                       bool)>(jit_compile_method);
    }

    // fields count from compiler_filter_ (not included) to inline_max_code_units_ (not included)
    unsigned thresholds_count = Android::version >= Android::VERSION_O ? 5 : 6;

    CompilerOptions_inline_max_code_units = new Member<void, size_t> (sizeof(void *) + thresholds_count * sizeof(size_t));
}

bool Jit::CompileMethod(Thread *thread, void *method) {
    void *compiler = GetCompiler();
    if (UNLIKELY(!compiler)) {
        LOGE("Cannot get art::jit::JitCompiler!");
        return false;
    }

    bool result;

    // JIT compilation will modify the state of the thread, so we backup and restore it after compilation.
    int32_t origin_state_and_flags = thread->GetStateAndFlags();

    if (jit_compile_method) {
        result = jit_compile_method(compiler, method, thread, false/*osr*/);
    } else if (jit_compile_method_q) {
        result = jit_compile_method_q(compiler, method, thread, false/*baseline*/, false/*osr*/);
    } else {
        LOGE("Compile method failed: jit_compile_method not found");
        return false;
    }

    thread->SetStateAndFlags(origin_state_and_flags);
    return result;
}

void fake_jit_update_options(void *handle) {
    LOGI("Ignoring request to update CompilerOptions from ART.");
}

bool Jit::DisableInline() {
    JitCompiler *compiler = GetGlobalCompiler();
    if (UNLIKELY(!compiler)) {
        LOGE("Disable JIT inline failed: JitCompiler is not available now!");
        return false;
    }
    void *compiler_options = compiler->compiler_options_.get();
    if (UNLIKELY(!compiler_options)) {
        LOGE("Disable JIT inline failed: JIT CompilerOptions is null");
        return false;
    }
    size_t inline_max_code_units = CompilerOptions_inline_max_code_units->Get(compiler_options);
    if (LIKELY(inline_max_code_units >= 0 && inline_max_code_units <= 1024)) {
        if (jit_update_options_ptr) {
            // Android Q, hook art::jit::Jit::jit_update_options_ to avoid update CompilerOptions.
            if (LIKELY(*jit_update_options_ptr))
                *jit_update_options_ptr = reinterpret_cast<void *>(fake_jit_update_options);
            else
                LOGW("Not hooking jit_update_options: symbol found but the function it points to is invalid.");
        }
        CompilerOptions_inline_max_code_units->Set(compiler_options, 0);
        return true;
    } else {
        // It is not a normal inline_max_code_units. It may be that the offset is changed
        // due to the source code modified by the manufacturer of this device.
        LOGE("Unexpected inline_max_code_units value %u (offset %d).", inline_max_code_units,
                CompilerOptions_inline_max_code_units->GetOffset());
        return false;
    }
}


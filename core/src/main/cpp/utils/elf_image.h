//
// From https://github.com/ganyao114/SandHook/blob/master/hooklib/src/main/cpp/includes/elf_util.h
// Original work Copyright (c) Swift Gan (github user ganyao114)
// Modified work Copyright (c) canyie (github user canyie)
// License: Anti 996 License Version 1.0
// Created by Swift Gan on 2019/3/14.
//

#ifndef PINE_ELF_IMAGE_H
#define PINE_ELF_IMAGE_H

#include <vector>
#include <stdio.h>
#include <linux/elf.h>
#include "macros.h"

#if defined(__LP64__)
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Dyn Elf_Dyn;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Sym Elf_Sym;
typedef Elf64_Off Elf_Off;

#define ELF_R_SYM(i) ELF64_R_SYM(i)
#else
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Dyn Elf_Dyn;
typedef Elf32_Rel Elf_Rela;
typedef Elf32_Sym Elf_Sym;
typedef Elf32_Off Elf_Off;

#define ELF_R_SYM(i) ELF32_R_SYM(i)
#endif

namespace pine {
    class ElfImage {
    public:
        ElfImage(const char* elf, bool warn_if_nonexist = true, bool warn_if_symtab_not_found = true) {
            this->elf_name_ = elf;
            if (elf[0] == '/') {
                Open(elf, warn_if_nonexist, warn_if_symtab_not_found);
            } else {
                // Relative path
                RelativeOpen(elf, warn_if_nonexist, warn_if_symtab_not_found);
            }
        }

        Elf_Addr LinearLookup(const char* name) const;
        void* GetSymbolAddress(const char* name, bool warn_if_missing = true) const;
        bool HasSymbol(const char* name) const;

        void Open(const char* path, bool warn_if_nonexist, bool warn_if_symtab_not_found);
        void RelativeOpen(const char* elf, bool warn_if_nonexist, bool warn_if_symtab_not_found);
        bool IsOpened() const {
            return header_ != nullptr;
        }

        ~ElfImage();

    private:
        void ParseMemory(Elf_Ehdr* header, bool is_debugdata);
        void ParseDebugdata(uint8_t* debugdata, size_t size);
        // Pine changed: GetModuleBase is private
        void* GetModuleBase(const char* name);

#ifdef __LP64__
        static constexpr const char* kSystemLibDir = "/system/lib64/";
        static constexpr const char* kApexRuntimeLibDir = "/apex/com.android.runtime/lib64/";
        static constexpr const char* kApexArtLibDir = "/apex/com.android.art/lib64/";
#else
        static constexpr const char* kSystemLibDir = "/system/lib/";
        static constexpr const char* kApexRuntimeLibDir = "/apex/com.android.runtime/lib/";
        static constexpr const char* kApexArtLibDir = "/apex/com.android.art/lib/";
#endif

        const char* elf_name_ = nullptr;
        void* base_ = nullptr;
        off_t size_ = 0;
        off_t bias_ = -4396;

        Elf_Ehdr* header_ = nullptr;

        // .dynsym
        Elf_Sym* dynsym_ = nullptr;
        Elf_Off dynsym_count_ = 0;

        // .symtab
        Elf_Sym* symtab_ = nullptr;
        Elf_Off symtab_count_ = 0;

        // .dynstr which is guaranteed to be the first STRTAB
        const char* dynstr_ = nullptr;

        // .strtab
        Elf_Addr strtab_ = 0;

        // .gnu_debugdata
        // Decompressed debugdata.xz
        std::vector<uint8_t> debugdata_;
        Elf_Sym* symtab_from_debugdata_ = 0;
        Elf_Off symtab_count_from_debugdata_ = 0;
        Elf_Addr strtab_from_debugdata_ = 0;
    };
}

#endif //PINE_ELF_IMAGE_H

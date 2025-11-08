//
// From https://github.com/ganyao114/SandHook/blob/master/hooklib/src/main/cpp/includes/elf_util.h
// Original work Copyright (c) Swift Gan (github user ganyao114)
// Modified work Copyright (c) canyie (github user canyie)
// License: Anti 996 License Version 1.0
// Created by Swift Gan on 2019/3/14.
//

#include "elf_image.h"

#include <cstring>
#include <malloc.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xz.h>
#include "io_wrapper.h"
#include "log.h"
#include "macros.h"
#include "../android.h"

using namespace pine;

void ElfImage::Open(const char* path, bool warn_if_nonexist, bool warn_if_symtab_not_found) {
    int fd = WrappedOpen(path, O_RDONLY | O_CLOEXEC);
    if (UNLIKELY(fd == -1)) {
        if (UNLIKELY(errno != ENOENT || warn_if_nonexist))
            LOGE("Failed to open %s: %s", path, strerror(errno));
        return;
    }

    size_ = lseek(fd, 0, SEEK_END);
    if (UNLIKELY(size_ <= 0)) {
        LOGE("lseek() failed for %s: %s", path, strerror(errno));
    }

    // Get the base load address of the specified elf
    // This should happen before calling mmap to avoid conflicts
    base_ = GetModuleBase(path);
    if (UNLIKELY(!base_)) {
        LOGE("Cannot find the base loaded address of %s in memory!", path);
        close(fd);
        return;
    }

    header_ = reinterpret_cast<Elf_Ehdr*>(mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd, 0));
    if (UNLIKELY(header_ == MAP_FAILED)) {
        LOGE("mmap() failed for %s: %s", path, strerror(errno));
        close(fd);
        header_ = nullptr;
        return;
    }

    close(fd);

    ParseMemory(header_, false);

    if (UNLIKELY(!symtab_ && !symtab_from_debugdata_ && warn_if_symtab_not_found)) {
        LOGW("can't find symtab from sections in %s\n", path);
    }
}

void ElfImage::ParseMemory(Elf_Ehdr* header, bool is_debugdata) {
    auto header_address = reinterpret_cast<uintptr_t>(header);
    auto section_headers = reinterpret_cast<Elf_Shdr*>(header_address + header->e_shoff);
    auto shoff = reinterpret_cast<uintptr_t>(section_headers);
    auto section_str = reinterpret_cast<char*>(header_address + section_headers[header->e_shstrndx].sh_offset);

    for (int i = 0; i < header->e_shnum; i++, shoff += header->e_shentsize) {
        Elf_Shdr* section_h = reinterpret_cast<Elf_Shdr*>(shoff);
        char* sname = section_h->sh_name + section_str;
        Elf_Off entsize = section_h->sh_entsize;
        switch (section_h->sh_type) {
            case SHT_DYNSYM:
                // Shall we reject ELFs with duplicated dynsym sections?
                if (!is_debugdata && bias_ == -4396) {
                    dynsym_ = reinterpret_cast<Elf_Sym*>(header_address + section_h->sh_offset);
                    dynsym_count_ = section_h->sh_size / entsize;
                }
                break;
            case SHT_SYMTAB:
                if (strcmp(sname, ".symtab") == 0) {
                    if (is_debugdata) {
                        symtab_from_debugdata_ = reinterpret_cast<Elf_Sym*>(header_address + section_h->sh_offset);
                        symtab_count_from_debugdata_ = section_h->sh_size / entsize;
                    } else {
                        symtab_ = reinterpret_cast<Elf_Sym*>(header_address + section_h->sh_offset);
                        symtab_count_ = section_h->sh_size / entsize;
                    }
                }
                break;
            case SHT_STRTAB:
                // .dynstr is guaranteed to be the first STRTAB, but just in case, let's check the name
                if (!is_debugdata && strcmp(sname, ".dynstr") == 0) {
                    dynstr_ = reinterpret_cast<const char*>(header_address + section_h->sh_offset);
                } else if (strcmp(sname, ".strtab") == 0) {
                    if (is_debugdata) {
                        strtab_from_debugdata_ = header_address + section_h->sh_offset;
                    } else {
                        strtab_ = header_address + section_h->sh_offset;
                    }
                }
                break;
            case SHT_PROGBITS:
                if (dynstr_ == nullptr || dynsym_ == nullptr || is_debugdata) break;
                if (bias_ == -4396) {
                    bias_ = (off_t) section_h->sh_addr - (off_t) section_h->sh_offset;
                }
                if (strcmp(".gnu_debugdata", sname) == 0) {
                    auto debugdata = reinterpret_cast<uint8_t*>(header_address + section_h->sh_offset);
                    ParseDebugdata(debugdata, section_h->sh_size);
                }
                break;
        }
    }
}

void ElfImage::ParseDebugdata(uint8_t* debugdata, size_t size) {
    uint8_t buf[8192];
    xz_crc32_init();
    struct xz_dec *dec = xz_dec_init(XZ_DYNALLOC, 1 << 20);
    if (!dec) [[unlikely]] {
        LOGE("Failed to initialize xz decoder!");
        return;
    }
    struct xz_buf b = {
            .in = debugdata,
            .in_pos = 0,
            .in_size = size,
            .out = buf,
            .out_pos = 0,
            .out_size = sizeof(buf)
    };
    do {
        enum xz_ret ret = xz_dec_run(dec, &b);
        if (ret != XZ_OK && ret != XZ_STREAM_END) [[unlikely]] {
            // Hmmm... Not a serious error, just report it
            LOGE("Failed to decompress debugdata with error code %d!", ret);
            xz_dec_end(dec);
            debugdata_.clear();
            return;
        }
        debugdata_.insert(debugdata_.end(), buf, buf + b.out_pos);
        b.out_pos = 0;
    } while (b.in_pos != size);
    xz_dec_end(dec);
    ParseMemory(reinterpret_cast<Elf_Ehdr*>(debugdata_.data()), true);
}

void ElfImage::RelativeOpen(const char* elf, bool warn_if_nonexist, bool warn_if_symtab_not_found) {
    char buffer[128] = {0}; // We assume that the path length doesn't exceed 128 bytes.
    if (Android::version >= Android::kQ) {
        // Android R: com.android.art
        strcpy(buffer, kApexArtLibDir);
        strcat(buffer, elf);
        Open(buffer, false, warn_if_symtab_not_found);
        if (LIKELY(IsOpened())) return;

        // Android Q: com.android.runtime
        strcpy(buffer, kApexRuntimeLibDir);
        strcat(buffer, elf);
        Open(buffer, false, warn_if_symtab_not_found);
        if (LIKELY(IsOpened())) return;
    }
    strcpy(buffer, kSystemLibDir);
    strcat(buffer, elf);
    Open(buffer, warn_if_nonexist, warn_if_symtab_not_found);
}

ElfImage::~ElfImage() {
    if (header_) [[likely]] {
        munmap(header_, size_);
    }
}

Elf_Addr ElfImage::LinearLookup(const char* name) const {
    // search dynsym
    if (dynsym_ != nullptr && dynstr_ != nullptr) [[likely]] {
        Elf_Sym* sym = dynsym_;
        for (Elf_Off i = 0; i < dynsym_count_; i++, sym++)
            if (strcmp(dynstr_ + sym->st_name, name) == 0)
                return sym->st_value;
    }

    // search .symtab
    if (symtab_ != nullptr && strtab_ != 0) [[likely]] {
        for (Elf_Off i = 0; i < symtab_count_; i++) {
            unsigned int st_type = ELF_ST_TYPE(symtab_[i].st_info);
            auto st_name = reinterpret_cast<const char*>(strtab_ + symtab_[i].st_name);
            if (st_type == STT_FUNC && symtab_[i].st_size && strcmp(st_name, name) == 0) {
                return symtab_[i].st_value;
            }
        }
    }

    // search symtab from debugdata
    if (symtab_from_debugdata_ && strtab_from_debugdata_) [[likely]] {
        for (Elf_Off i = 0; i < symtab_count_from_debugdata_; i++) {
            unsigned int st_type = ELF_ST_TYPE(symtab_from_debugdata_[i].st_info);
            auto st_name = reinterpret_cast<const char*>(strtab_from_debugdata_ + symtab_from_debugdata_[i].st_name);
            if (st_type == STT_FUNC && symtab_from_debugdata_[i].st_size && strcmp(st_name, name) == 0) {
                return symtab_from_debugdata_[i].st_value;
            }
        }
    }

    return 0;
}

void* ElfImage::GetSymbolAddress(const char* name, bool warn_if_missing) const {
    if (UNLIKELY(!base_))
        return nullptr;
    // TODO: hash based lookup
    Elf_Addr offset = LinearLookup(name);
    if (UNLIKELY(!offset)) {
        if (warn_if_missing) LOGE("Symbol %s not found in elf %s", name, elf_name_);
        return nullptr;
    }
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base_) + offset - bias_);
}

bool ElfImage::HasSymbol(const char* name) const {
    return GetSymbolAddress(name, false) != nullptr;
}

void* ElfImage::GetModuleBase(const char* name) {
    FILE* maps;
    char buff[256];
    off_t load_addr;
    bool found = false;
    maps = WrappedFOpen("/proc/self/maps", "re");
    while (fgets(buff, sizeof(buff), maps)) {
        if (strstr(buff, name) && (strstr(buff, "r-xp") || strstr(buff, "r--p"))) {
            found = true;
            break;
        }
    }

    if (UNLIKELY(!found)) {
        LOGE("Failed to read load address for %s", name);
        fclose(maps);
        return nullptr;
    }

    if (UNLIKELY(sscanf(buff, "%lx", &load_addr) != 1))
        LOGE("Failed to read load address for %s", name);

    fclose(maps);

    return reinterpret_cast<void*>(load_addr);
}

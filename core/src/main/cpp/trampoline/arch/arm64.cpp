//
// Created by canyie on 2020/4/7.
//

#include "arm64.h"

using namespace pine;

void Arm64TrampolineInstaller::InitTrampolines() {
    TrampolineInstaller::InitTrampolines();
    kDirectJumpTrampolineSize = 16;
}

bool Arm64TrampolineInstaller::IsPCRelatedInst(uint32_t inst) {
    INST_CASE(0xff000010, 0x54000000); // b <label>
    INST_CASE(0x7c000000, 0x14000000); // bl <label>
    INST_CASE(0x7e000000, 0x34000000); // cb{n}z Rn, <label>
    INST_CASE(0x7e000000, 0x36000000); // tb{n}z Rt, #<imm>, <label>
    INST_CASE(0x3b000000, 0x18000000); // ldr
    INST_CASE(0x1f000000, 0x10000000); // adr/adrp Rd, <label>
    return false;
}

bool Arm64TrampolineInstaller::CannotBackup(art::ArtMethod *target) {
    uintptr_t entry = reinterpret_cast<uintptr_t>(target->GetEntryPointFromCompiledCode());
    for (uint32_t index = 0;index < kDirectJumpTrampolineSize;index += 4) {
        uint32_t *p = reinterpret_cast<uint32_t *> (entry + index);
        if (UNLIKELY(IsPCRelatedInst(*p))) {
            return true;
        }
    }
    return false;
}
